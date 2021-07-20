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
		Vector3D& operator += (const Vector3D& v);
		Vector3D& operator -= (const Vector3D& v);
	};
  
  extern Vector3D operator * (const Vector3D& v, float s);
  extern Vector3D operator / (const Vector3D& v, float s);
  extern Vector3D operator - (const Vector3D& v);
  extern float Magnitude(const Vector3D& v);
  extern Vector3D Normalize(const Vector3D& v);
  extern Vector3D operator + (const Vector3D& a, const Vector3D& b);
  extern Vector3D operator - (const Vector3D& a, const Vector3D& b);
  extern float Dot(const Vector3D& a, const Vector3D& b);
  extern Vector3D Cross(const Vector3D& a, const Vector3D& b);
  extern Vector3D Project(const Vector3D& a, const Vector3D& b);
  extern Vector3D Reject(const Vector3D& a, const Vector3D& b);
}