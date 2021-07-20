#pragma once

namespace BadgerSandbox
{
	class Vector4D
	{
	public:
		float x, y, z, w;
		Vector4D(float a, float b, float c, float d);
		float& operator[](int i);
		const float& operator[](int i) const;
		Vector4D& operator *= (float s);
		Vector4D& operator /= (float s);
		Vector4D& operator += (const Vector4D& v);
		Vector4D& operator -= (const Vector4D& v);
	};
  
	extern Vector4D operator * (const Vector4D& v, float s);
	extern Vector4D operator / (const Vector4D& v, float s);
	extern Vector4D operator - (const Vector4D& v);
	extern float Magnitude(const Vector4D& v);
	extern Vector4D Normalize(const Vector4D& v);
	extern Vector4D operator + (const Vector4D& a, const Vector4D& b);
	extern Vector4D operator - (const Vector4D& a, const Vector4D& b);
	extern float Dot(const Vector4D& a, const Vector4D& b);
	extern Vector4D Project(const Vector4D& a, const Vector4D& b);
	extern Vector4D Reject(const Vector4D& a, const Vector4D& b);
}