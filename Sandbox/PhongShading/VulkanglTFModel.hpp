/*
 * Vulkan glTF model and texture loading class based on tinyglTF (https://github.com/syoyo/tinygltf)
 *
 * Copyright (C) 2018 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <stdlib.h>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>

#include "vulkan/vulkan.h"


#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>


#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "tiny_gltf.h"
#include <RapidVulkan/Check.hpp>

// Changing this value here also requires changing it in the vertex shader
constexpr uint32_t MAX_NUM_JOINTS = 512u;

namespace vkglTF
{
    VkPhysicalDevice gltfPhysicalDevice;
    VkDevice gltfLogicalDevice;
    VkPhysicalDeviceMemoryProperties glTFMemoryProperties;
    VkCommandPool gltfCommandPool;
	  struct Node;

  struct BoundingBox
  {
    glm::vec3 min;
    glm::vec3 max;
    bool valid = false;
    BoundingBox(){};
    BoundingBox(glm::vec3 min, glm::vec3 max)
      : min(min)
      , max(max)
    {
    }
    BoundingBox getAABB(glm::mat4 m)
    {
      glm::vec3 min = glm::vec3(m[3]);
      glm::vec3 max = min;
      glm::vec3 v0, v1;

      glm::vec3 right = glm::vec3(m[0]);
      v0 = right * this->min.x;
      v1 = right * this->max.x;
      min += glm::min(v0, v1);
      max += glm::max(v0, v1);

      glm::vec3 up = glm::vec3(m[1]);
      v0 = up * this->min.y;
      v1 = up * this->max.y;
      min += glm::min(v0, v1);
      max += glm::max(v0, v1);

      glm::vec3 back = glm::vec3(m[2]);
      v0 = back * this->min.z;
      v1 = back * this->max.z;
      min += glm::min(v0, v1);
      max += glm::max(v0, v1);

      return BoundingBox(min, max);
    }
  };

    uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound = nullptr)
    {
        for (uint32_t i = 0; i < glTFMemoryProperties.memoryTypeCount; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((glTFMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    if (memTypeFound)
                    {
                        *memTypeFound = true;
                    }
                    return i;
                }
            }
            typeBits >>= 1;
        }

        if (memTypeFound)
        {
            *memTypeFound = false;
            return 0;
        }
        else
        {
            throw std::runtime_error("Could not find a matching memory type");
        }
    }

    VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer,
        VkDeviceMemory* memory, void* data = nullptr)
    {
        // Create the buffer handle
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.usage = usageFlags;
        bufferCreateInfo.size = size;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        RapidVulkan::CheckError(vkCreateBuffer(gltfLogicalDevice, &bufferCreateInfo, nullptr, buffer));

        // Create the memory backing up the buffer handle
        VkMemoryRequirements memReqs;
        VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkGetBufferMemoryRequirements(gltfLogicalDevice, *buffer, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        // Find a memory type index that fits the properties of the buffer
        memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
        RapidVulkan::CheckError(vkAllocateMemory(gltfLogicalDevice, &memAlloc, nullptr, memory));

        // If a pointer to the buffer data has been passed, map the buffer and copy over the data
        if (data != nullptr)
        {
            void* mapped;
            RapidVulkan::CheckError(vkMapMemory(gltfLogicalDevice, *memory, 0, size, 0, &mapped));
            memcpy(mapped, data, size);
            // If host coherency hasn't been requested, do a manual flush to make writes visible
            if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange mappedRange{};
                mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                mappedRange.memory = *memory;
                mappedRange.offset = 0;
                mappedRange.size = size;
                vkFlushMappedMemoryRanges(gltfLogicalDevice, 1, &mappedRange);
            }
            vkUnmapMemory(gltfLogicalDevice, *memory);
        }

        // Attach the memory to the buffer object
        RapidVulkan::CheckError(vkBindBufferMemory(gltfLogicalDevice, *buffer, *memory, 0));

        return VK_SUCCESS;
    }

    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool = gltfCommandPool;
        cmdBufAllocateInfo.level = level;
        cmdBufAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer cmdBuffer;
        RapidVulkan::CheckError(vkAllocateCommandBuffers(gltfLogicalDevice, &cmdBufAllocateInfo, &cmdBuffer));

        // If requested, also start recording for the new command buffer
        if (begin)
        {
            VkCommandBufferBeginInfo commandBufferBI{};
            commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            RapidVulkan::CheckError(vkBeginCommandBuffer(cmdBuffer, &commandBufferBI));
        }

        return cmdBuffer;
    }

    void beginCommandBuffer(VkCommandBuffer commandBuffer)
    {
        VkCommandBufferBeginInfo commandBufferBI{};
        commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        RapidVulkan::CheckError(vkBeginCommandBuffer(commandBuffer, &commandBufferBI));
    }

    void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true)
    {
        RapidVulkan::CheckError(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence;
        RapidVulkan::CheckError(vkCreateFence(gltfLogicalDevice, &fenceInfo, nullptr, &fence));

        // Submit to the queue
        RapidVulkan::CheckError(vkQueueSubmit(queue, 1, &submitInfo, fence));
        // Wait for the fence to signal that command buffer has finished executing
        RapidVulkan::CheckError(vkWaitForFences(gltfLogicalDevice, 1, &fence, VK_TRUE, 100000000000));

        vkDestroyFence(gltfLogicalDevice, fence, nullptr);

        if (free)
        {
            vkFreeCommandBuffers(gltfLogicalDevice, gltfCommandPool, 1, &commandBuffer);
        }
    }
    
  struct Node;
    /*
    glTF primitive
  */
  struct Primitive
  {
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t vertexCount;
    bool hasIndices;

    BoundingBox bb;

    Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount)
      : firstIndex(firstIndex)
      , indexCount(indexCount)
      , vertexCount(vertexCount)
    {
      hasIndices = indexCount > 0;
    };

    void setBoundingBox(glm::vec3 min, glm::vec3 max)
    {
      bb.min = min;
      bb.max = max;
      bb.valid = true;
    }
  };

  /*
    glTF mesh
  */
  struct Mesh
  {
    std::vector<Primitive*> primitives;

    BoundingBox bb;
    BoundingBox aabb;

    struct UniformBuffer
    {
      VkBuffer buffer;
      VkDeviceMemory memory;
      VkDescriptorBufferInfo descriptor;
      VkDescriptorSet descriptorSet;
      void* mapped;
    } uniformBuffer;

    struct UniformBlock
    {
      glm::mat4 matrix;
      glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
      float jointcount{0};
    } uniformBlock;

    Mesh(glm::mat4 matrix)
    {
      this->uniformBlock.matrix = matrix;
      // TODO ADD A CREATE BUFFER FUNCTION TO THE FILE ITSELF
      RapidVulkan::CheckError(createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uniformBlock),
                                                   &uniformBuffer.buffer, &uniformBuffer.memory, &uniformBlock));
      RapidVulkan::CheckError(vkMapMemory(gltfLogicalDevice, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped));
      uniformBuffer.descriptor = {uniformBuffer.buffer, 0, sizeof(uniformBlock)};
    };

    ~Mesh()
    {
      vkDestroyBuffer(gltfLogicalDevice, uniformBuffer.buffer, nullptr);
      vkFreeMemory(gltfLogicalDevice, uniformBuffer.memory, nullptr);
      for (Primitive* p : primitives)
        delete p;
    }

    void setBoundingBox(glm::vec3 min, glm::vec3 max)
    {
      bb.min = min;
      bb.max = max;
      bb.valid = true;
    }
  };

  /*
    glTF skin
  */
  struct Skin
  {
    std::string name;
    Node* skeletonRoot = nullptr;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<Node*> joints;
  };

  /*
    glTF node
  */
  struct Node
  {
    Node* parent;
    uint32_t index;
    std::vector<Node*> children;
    glm::mat4 matrix;
    std::string name;
    Mesh* mesh;
    Skin* skin;
    int32_t skinIndex = -1;
    glm::vec3 translation{};
    glm::vec3 scale{1.0f};
    glm::quat rotation{};
    BoundingBox bvh;
    BoundingBox aabb;

    glm::mat4 localMatrix()
    {
      return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
    }

    glm::mat4 getMatrix()
    {
      glm::mat4 m = localMatrix();
      vkglTF::Node* p = parent;
      while (p)
      {
        m = p->localMatrix() * m;
        p = p->parent;
      }
      return m;
    }

    void update()
    {
      if (mesh)
      {
        glm::mat4 m = getMatrix();
        if (skin)
        {
          mesh->uniformBlock.matrix = m;
          // Update join matrices
          glm::mat4 inverseTransform = glm::inverse(m);
          size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);
          for (size_t i = 0; i < numJoints; i++)
          {
            vkglTF::Node* jointNode = skin->joints[i];
            glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
            jointMat = inverseTransform * jointMat;
            mesh->uniformBlock.jointMatrix[i] = jointMat;
          }
          mesh->uniformBlock.jointcount = (float)numJoints;
          memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
        }
        else
        {
          memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
        }
      }

      for (auto& child : children)
      {
        child->update();
      }
    }

    ~Node()
    {
      if (mesh)
      {
        delete mesh;
      }
      for (auto& child : children)
      {
        delete child;
      }
    }
  };

  /*
    glTF animation channel
  */
  struct AnimationChannel
  {
    enum PathType
    {
      TRANSLATION,
      ROTATION,
      SCALE
    };
    PathType path;
    Node* node;
    uint32_t samplerIndex;
  };

  /*
    glTF animation sampler
  */
  struct AnimationSampler
  {
    enum InterpolationType
    {
      LINEAR,
      STEP,
      CUBICSPLINE
    };
    InterpolationType interpolation;
    std::vector<float> inputs;
    std::vector<glm::vec4> outputsVec4;
  };

  /*
    glTF animation
  */
  struct Animation
  {
    std::string name;
    std::vector<AnimationSampler> samplers;
    std::vector<AnimationChannel> channels;
    float start = std::numeric_limits<float>::max();
    float end = std::numeric_limits<float>::min();
  };

  /*
    glTF model loading and rendering class
  */
  struct Model
  {
    struct Vertex
    {
      glm::vec3 pos;
      glm::vec3 normal;
      glm::vec2 uv0;
      glm::vec2 uv1;
      glm::vec4 joint0;
      glm::vec4 weight0;
    };

    struct Vertices
    {
      VkBuffer buffer = VK_NULL_HANDLE;
      VkDeviceMemory memory;
    } vertices;
    struct Indices
    {
      int count;
      VkBuffer buffer = VK_NULL_HANDLE;
      VkDeviceMemory memory;
    } indices;

    glm::mat4 aabb;

    std::vector<Node*> nodes;
    std::vector<Node*> linearNodes;

    std::vector<Skin*> skins;

    std::vector<Animation> animations;
    std::vector<std::string> extensions;

    struct Dimensions
    {
      glm::vec3 min = glm::vec3(FLT_MAX);
      glm::vec3 max = glm::vec3(-FLT_MAX);
    } dimensions;

    void destroy(VkDevice device)
    {
      if (vertices.buffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer(device, vertices.buffer, nullptr);
        vkFreeMemory(device, vertices.memory, nullptr);
      }
      if (indices.buffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer(device, indices.buffer, nullptr);
        vkFreeMemory(device, indices.memory, nullptr);
      }
      for (auto node : nodes)
      {
        delete node;
      }

      animations.resize(0);
      nodes.resize(0);
      linearNodes.resize(0);
      extensions.resize(0);
      for (auto skin : skins)
      {
        delete skin;
      }
      skins.resize(0);
    };

    void loadNode(vkglTF::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model,
                  std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale)
    {
      vkglTF::Node* newNode = new Node{};
      newNode->index = nodeIndex;
      newNode->parent = parent;
      newNode->name = node.name;
      newNode->skinIndex = node.skin;
      newNode->matrix = glm::mat4(1.0f);

      // Generate local node matrix
      glm::vec3 translation = glm::vec3(0.0f);
      if (node.translation.size() == 3)
      {
        translation = glm::make_vec3(node.translation.data());
        newNode->translation = translation;
      }
      glm::mat4 rotation = glm::mat4(1.0f);
      if (node.rotation.size() == 4)
      {
        glm::quat q = glm::make_quat(node.rotation.data());
        newNode->rotation = glm::mat4(q);
      }
      glm::vec3 scale = glm::vec3(1.0f);
      if (node.scale.size() == 3)
      {
        scale = glm::make_vec3(node.scale.data());
        newNode->scale = scale;
      }
      if (node.matrix.size() == 16)
      {
        newNode->matrix = glm::make_mat4x4(node.matrix.data());
      };

      // Node with children
      if (node.children.size() > 0)
      {
        for (size_t i = 0; i < node.children.size(); i++)
        {
          loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer, globalscale);
        }
      }

      // Node contains mesh data
      if (node.mesh > -1)
      {
        const tinygltf::Mesh mesh = model.meshes[node.mesh];
        Mesh* newMesh = new Mesh(newNode->matrix);
        for (size_t j = 0; j < mesh.primitives.size(); j++)
        {
          const tinygltf::Primitive& primitive = mesh.primitives[j];
          uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
          uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
          uint32_t indexCount = 0;
          uint32_t vertexCount = 0;
          glm::vec3 posMin{};
          glm::vec3 posMax{};
          bool hasSkin = false;
          bool hasIndices = primitive.indices > -1;
          // Vertices
          {
            const float* bufferPos = nullptr;
            const float* bufferNormals = nullptr;
            const float* bufferTexCoordSet0 = nullptr;
            const float* bufferTexCoordSet1 = nullptr;
            const uint16_t* bufferJoints = nullptr;
            const float* bufferWeights = nullptr;

            int posByteStride;
            int normByteStride;
            int uv0ByteStride;
            int uv1ByteStride;
            int jointByteStride;
            int weightByteStride;

            // Position attribute is required
            assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

            const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
            const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
            bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
            posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
            posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
            vertexCount = static_cast<uint32_t>(posAccessor.count);
            posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float))
                                                            : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

            if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
            {
              const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
              const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
              bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
              normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float))
                                                                 : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
            }

            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
            {
              const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
              const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
              bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
              uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                                                            : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
            }
            if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
            {
              const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
              const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
              bufferTexCoordSet1 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
              uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                                                            : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
            }

            // Skinning
            // Joints
            if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
            {
              const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
              const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
              bufferJoints =
                reinterpret_cast<const uint16_t*>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
              jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / sizeof(bufferJoints[0]))
                                                                    : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
            }

            if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
            {
              const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
              const tinygltf::BufferView& weightView = model.bufferViews[weightAccessor.bufferView];
              bufferWeights =
                reinterpret_cast<const float*>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
              weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float))
                                                                       : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
            }

            hasSkin = (bufferJoints && bufferWeights);

            for (size_t v = 0; v < posAccessor.count; v++)
            {
              Vertex vert{};
              vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
              vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
              vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
              vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);

              vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * jointByteStride])) : glm::vec4(0.0f);
              vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
              // Fix for all zero weights
              if (glm::length(vert.weight0) == 0.0f)
              {
                vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
              }
              vertexBuffer.push_back(vert);
            }
          }
          // Indices
          if (hasIndices)
          {
            const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
            const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

            indexCount = static_cast<uint32_t>(accessor.count);
            const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

            switch (accessor.componentType)
            {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            {
              const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
              for (size_t index = 0; index < accessor.count; index++)
              {
                indexBuffer.push_back(buf[index] + vertexStart);
              }
              break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            {
              const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
              for (size_t index = 0; index < accessor.count; index++)
              {
                indexBuffer.push_back(buf[index] + vertexStart);
              }
              break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            {
              const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
              for (size_t index = 0; index < accessor.count; index++)
              {
                indexBuffer.push_back(buf[index] + vertexStart);
              }
              break;
            }
            default:
                std::cout << "Index component type: " << std::to_string(accessor.componentType) << " not supported" << std::endl;
                return;
            }
          }
          Primitive* newPrimitive = new Primitive(indexStart, indexCount, vertexCount);
          newPrimitive->setBoundingBox(posMin, posMax);
          newMesh->primitives.push_back(newPrimitive);
        }
        // Mesh BB from BBs of primitives
        for (auto p : newMesh->primitives)
        {
          if (p->bb.valid && !newMesh->bb.valid)
          {
            newMesh->bb = p->bb;
            newMesh->bb.valid = true;
          }
          newMesh->bb.min = glm::min(newMesh->bb.min, p->bb.min);
          newMesh->bb.max = glm::max(newMesh->bb.max, p->bb.max);
        }
        newNode->mesh = newMesh;
      }
      if (parent)
      {
        parent->children.push_back(newNode);
      }
      else
      {
        nodes.push_back(newNode);
      }
      linearNodes.push_back(newNode);
    }

    void loadSkins(tinygltf::Model& gltfModel)
    {
      for (tinygltf::Skin& source : gltfModel.skins)
      {
        Skin* newSkin = new Skin{};
        newSkin->name = source.name;

        // Find skeleton root node
        if (source.skeleton > -1)
        {
          newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
        }

        // Find joint nodes
        for (int jointIndex : source.joints)
        {
          Node* node = nodeFromIndex(jointIndex);
          if (node)
          {
            newSkin->joints.push_back(nodeFromIndex(jointIndex));
          }
        }

        // Get inverse bind matrices from buffer
        if (source.inverseBindMatrices > -1)
        {
          const tinygltf::Accessor& accessor = gltfModel.accessors[source.inverseBindMatrices];
          const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
          const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
          newSkin->inverseBindMatrices.resize(accessor.count);
          memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
        }

        skins.push_back(newSkin);
      }
    }

    void loadAnimations(tinygltf::Model& gltfModel)
    {
      for (tinygltf::Animation& anim : gltfModel.animations)
      {
        vkglTF::Animation animation{};
        animation.name = anim.name;
        if (anim.name.empty())
        {
          animation.name = std::to_string(animations.size());
        }

        // Samplers
        for (auto& samp : anim.samplers)
        {
          vkglTF::AnimationSampler sampler{};

          if (samp.interpolation == "LINEAR")
          {
            sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
          }
          if (samp.interpolation == "STEP")
          {
            sampler.interpolation = AnimationSampler::InterpolationType::STEP;
          }
          if (samp.interpolation == "CUBICSPLINE")
          {
            sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
          }

          // Read sampler input time values
          {
            const tinygltf::Accessor& accessor = gltfModel.accessors[samp.input];
            const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

            const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
            const float* buf = static_cast<const float*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++)
            {
              sampler.inputs.push_back(buf[index]);
            }

            for (auto input : sampler.inputs)
            {
              if (input < animation.start)
              {
                animation.start = input;
              };
              if (input > animation.end)
              {
                animation.end = input;
              }
            }
          }

          // Read sampler output T/R/S values
          {
            const tinygltf::Accessor& accessor = gltfModel.accessors[samp.output];
            const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

            assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

            const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

            switch (accessor.type)
            {
            case TINYGLTF_TYPE_VEC3:
            {
              const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
              for (size_t index = 0; index < accessor.count; index++)
              {
                sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
              }
              break;
            }
            case TINYGLTF_TYPE_VEC4:
            {
              const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
              for (size_t index = 0; index < accessor.count; index++)
              {
                sampler.outputsVec4.push_back(buf[index]);
              }
              break;
            }
            default:
            {
              
              std::cout << "unknown type" << std::endl;
              break;
            }
            }
          }

          animation.samplers.push_back(sampler);
        }

        // Channels
        for (auto& source : anim.channels)
        {
          vkglTF::AnimationChannel channel{};

          if (source.target_path == "rotation")
          {
            channel.path = AnimationChannel::PathType::ROTATION;
          }
          if (source.target_path == "translation")
          {
            channel.path = AnimationChannel::PathType::TRANSLATION;
          }
          if (source.target_path == "scale")
          {
            channel.path = AnimationChannel::PathType::SCALE;
          }
          if (source.target_path == "weights")
          {
              std::cout << "weights not yet supported, skipping channel" << std::endl;
              continue;
          }
          channel.samplerIndex = source.sampler;
          channel.node = nodeFromIndex(source.target_node);
          if (!channel.node)
          {
            continue;
          }

          animation.channels.push_back(channel);
        }

        animations.push_back(animation);
      }
    }

    void loadFromFile(std::string filename, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool commandPool, float scale = 1.0f)
    {
      tinygltf::Model gltfModel;
      tinygltf::TinyGLTF gltfContext;
      std::string error;
      std::string warning;

      gltfPhysicalDevice = physicalDevice;
      gltfLogicalDevice = device;
      gltfCommandPool = commandPool;
      vkGetPhysicalDeviceMemoryProperties(gltfPhysicalDevice, &glTFMemoryProperties);

      bool binary = false;
      size_t extpos = filename.rfind('.', filename.length());
      if (extpos != std::string::npos)
      {
        binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
      }

      bool fileLoaded = binary ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, filename.c_str())
                               : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename.c_str());

      std::vector<uint32_t> indexBuffer;
      std::vector<Vertex> vertexBuffer;

      if (fileLoaded)
      {
        // TODO: scene handling with no default scene
        const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
        for (size_t i = 0; i < scene.nodes.size(); i++)
        {
          const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
          loadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);
        }
        if (gltfModel.animations.size() > 0)
        {
          loadAnimations(gltfModel);
        }
        loadSkins(gltfModel);

        for (auto node : linearNodes)
        {
          // Assign skins
          if (node->skinIndex > -1)
          {
            node->skin = skins[node->skinIndex];
          }
          // Initial pose
          if (node->mesh)
          {
            node->update();
          }
        }
      }
      else
      {
          throw std::runtime_error("COULD NOT LOAD glTF MODEL");
      }

      extensions = gltfModel.extensionsUsed;

      size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
      size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
      indices.count = static_cast<uint32_t>(indexBuffer.size());

      assert(vertexBufferSize > 0);

      struct StagingBuffer
      {
        VkBuffer buffer;
        VkDeviceMemory memory;
      } vertexStaging, indexStaging;

      // Create staging buffers
      // Vertex data
      RapidVulkan::CheckError(createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBufferSize,
                                                   &vertexStaging.buffer, &vertexStaging.memory, vertexBuffer.data()));
      // Index data
      if (indexBufferSize > 0)
      {
        RapidVulkan::CheckError(createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexBufferSize,
                                                     &indexStaging.buffer, &indexStaging.memory, indexBuffer.data()));
      }

      // Create device local buffers
      // Vertex buffer
      RapidVulkan::CheckError(createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBufferSize, &vertices.buffer, &vertices.memory));
      // Index buffer
      if (indexBufferSize > 0)
      {
        RapidVulkan::CheckError(createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBufferSize, &indices.buffer, &indices.memory));
      }

      // Copy from staging buffers
      VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

      VkBufferCopy copyRegion = {};

      copyRegion.size = vertexBufferSize;
      vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

      if (indexBufferSize > 0)
      {
        copyRegion.size = indexBufferSize;
        vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);
      }

      flushCommandBuffer(copyCmd, transferQueue, true);

      vkDestroyBuffer(gltfLogicalDevice, vertexStaging.buffer, nullptr);
      vkFreeMemory(gltfLogicalDevice, vertexStaging.memory, nullptr);
      if (indexBufferSize > 0)
      {
        vkDestroyBuffer(gltfLogicalDevice, indexStaging.buffer, nullptr);
        vkFreeMemory(gltfLogicalDevice, indexStaging.memory, nullptr);
      }

      getSceneDimensions();
    }

    void drawNode(Node* node, VkCommandBuffer commandBuffer)
    {
      if (node->mesh)
      {
        for (Primitive* primitive : node->mesh->primitives)
        {
          vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
        }
      }
      for (auto& child : node->children)
      {
        drawNode(child, commandBuffer);
      }
    }

    void draw(VkCommandBuffer commandBuffer)
    {
      const VkDeviceSize offsets[1] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
      vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
      for (auto& node : nodes)
      {
        drawNode(node, commandBuffer);
      }
    }

    void calculateBoundingBox(Node* node, Node* parent)
    {
      BoundingBox parentBvh = parent ? parent->bvh : BoundingBox(dimensions.min, dimensions.max);

      if (node->mesh)
      {
        if (node->mesh->bb.valid)
        {
          node->aabb = node->mesh->bb.getAABB(node->getMatrix());
          if (node->children.size() == 0)
          {
            node->bvh.min = node->aabb.min;
            node->bvh.max = node->aabb.max;
            node->bvh.valid = true;
          }
        }
      }

      parentBvh.min = glm::min(parentBvh.min, node->bvh.min);
      parentBvh.max = glm::min(parentBvh.max, node->bvh.max);

      for (auto& child : node->children)
      {
        calculateBoundingBox(child, node);
      }
    }

    void getSceneDimensions()
    {
      // Calculate binary volume hierarchy for all nodes in the scene
      for (auto node : linearNodes)
      {
        calculateBoundingBox(node, nullptr);
      }

      dimensions.min = glm::vec3(FLT_MAX);
      dimensions.max = glm::vec3(-FLT_MAX);

      for (auto node : linearNodes)
      {
        if (node->bvh.valid)
        {
          dimensions.min = glm::min(dimensions.min, node->bvh.min);
          dimensions.max = glm::max(dimensions.max, node->bvh.max);
        }
      }

      // Calculate scene aabb
      aabb = glm::scale(glm::mat4(1.0f), glm::vec3(dimensions.max[0] - dimensions.min[0], dimensions.max[1] - dimensions.min[1],
                                                   dimensions.max[2] - dimensions.min[2]));
      aabb[3][0] = dimensions.min[0];
      aabb[3][1] = dimensions.min[1];
      aabb[3][2] = dimensions.min[2];
    }

    void updateAnimation(uint32_t index, float time)
    {
      if (animations.empty())
      {
        std::cout << "glTF does not contain animation" << std::endl;
        return;
      }
      if (index > static_cast<uint32_t>(animations.size()) - 1)
      {
        std::cout << "No animation with index" << std::endl;
        return;
      }
      Animation& animation = animations[index];

      bool updated = false;
      for (auto& channel : animation.channels)
      {
        vkglTF::AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
        if (sampler.inputs.size() > sampler.outputsVec4.size())
        {
          continue;
        }

        for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
        {
          if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1]))
          {
            float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
            if (u <= 1.0f)
            {
              switch (channel.path)
              {
              case vkglTF::AnimationChannel::PathType::TRANSLATION:
              {
                glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
                channel.node->translation = glm::vec3(trans);
                break;
              }
              case vkglTF::AnimationChannel::PathType::SCALE:
              {
                glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
                channel.node->scale = glm::vec3(trans);
                break;
              }
              case vkglTF::AnimationChannel::PathType::ROTATION:
              {
                glm::quat q1;
                q1.x = sampler.outputsVec4[i].x;
                q1.y = sampler.outputsVec4[i].y;
                q1.z = sampler.outputsVec4[i].z;
                q1.w = sampler.outputsVec4[i].w;
                glm::quat q2;
                q2.x = sampler.outputsVec4[i + 1].x;
                q2.y = sampler.outputsVec4[i + 1].y;
                q2.z = sampler.outputsVec4[i + 1].z;
                q2.w = sampler.outputsVec4[i + 1].w;
                channel.node->rotation = glm::normalize(glm::slerp(q1, q2, u));
                break;
              }
              }
              updated = true;
            }
          }
        }
      }
      if (updated)
      {
        for (auto& node : nodes)
        {
          node->update();
        }
      }
    }

    /*
      Helper functions
    */
    Node* findNode(Node* parent, uint32_t index)
    {
      Node* nodeFound = nullptr;
      if (parent->index == index)
      {
        return parent;
      }
      for (auto& child : parent->children)
      {
        nodeFound = findNode(child, index);
        if (nodeFound)
        {
          break;
        }
      }
      return nodeFound;
    }

    Node* nodeFromIndex(uint32_t index)
    {
      Node* nodeFound = nullptr;
      for (auto& node : nodes)
      {
        nodeFound = findNode(node, index);
        if (nodeFound)
        {
          break;
        }
      }
      return nodeFound;
    }
  };
}