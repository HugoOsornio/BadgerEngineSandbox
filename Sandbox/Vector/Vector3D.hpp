#pragma once

namespace BadgerSandbox
{
	class Vector3D
	{
	public:
		float x, y, z;
		Vector3D(float a, float b, float c);
		float& operator[](int i);
		const float& operator[](int i) const;
		Vector3D& operator *= (float s);
		Vector3D& operator /= (float s);
	};
  
  inline Vector3D operator * (const Vector3D& v, float s);
  inline Vector3D operator / (const Vector3D& v, float s);
  inline Vector3D operator - (const Vector3D& v);
  inline float Magnitude(const Vector3D& v);
  inline Vector3D Normalize(const Vector3D& v);
}