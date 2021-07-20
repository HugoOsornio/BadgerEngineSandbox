#include "Vector4D.hpp"
#include <cmath>

namespace BadgerSandbox
{
  Vector4D::Vector4D(float a, float b, float c, float d)
  : x(a)
  , y(b)
  , z(c)
  , w(d)
  {
    if(d != 0.0f)
		w = 1.0f;
  }
  
  float& Vector4D::operator[](int i)
  {
	  return ((&x)[i]);
  }
  
  const float& Vector4D::operator[](int i) const
  {
	  return ((&x)[i]);
  }
  
  Vector4D& Vector4D::operator *= (float s)
  {
	  x *= s;
	  y *= s;
	  z *= s;
	  return (*this);
  }
  
  Vector4D& Vector4D::operator /= (float s)
  {
	  s = 1.0F / s;
	  x *= s;
	  y *= s;
	  z *= s;
	  return (*this);
  }

  Vector4D& Vector4D::operator += (const Vector4D& v)
  {
	  x += v.x;
	  y += v.y;
	  z += v.z;
	  return (*this);
  }

  Vector4D& Vector4D::operator -= (const Vector4D& v)
  {
	  x -= v.x;
	  y -= v.y;
	  z -= v.z;
	  return (*this);
  }
  
  inline Vector4D operator * (const Vector4D& v, float s)
  {
	  return (Vector4D(v.x * s, v.y *s, v.z * s, v.w));
  }
  
  inline Vector4D operator / (const Vector4D& v, float s)
  {
	  s = 1.0F / s;
	  return (Vector4D(v.x * s, v.y * s, v.z * s, v.w));
  }
  
  inline Vector4D operator - (const Vector4D& v)
  {
	  return (Vector4D(-v.x, -v.y, -v.z, v.w));
  }
  
  inline float Magnitude(const Vector4D& v)
  {
	  return (sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w));
  }
  
  inline Vector4D Normalize(const Vector4D& v)
  {
	  return (v / Magnitude(v));
  }

  inline Vector4D operator + (const Vector4D& a, const Vector4D& b)
  {
	  return (Vector4D(a.x + b.x, a.y + b.y, a.z + b.z, a.w));
  }

  inline Vector4D operator - (const Vector4D& a, const Vector4D& b)
  {
	  return (Vector4D(a.x - b.x, a.y - b.y, a.z - b.z, a.w));
  }

  inline float Dot(const Vector4D& a, const Vector4D& b)
  {
	  return (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
  }

  inline Vector4D Project(const Vector4D& a, const Vector4D& b)
  {
	  return (b * (Dot(a, b) / Dot(b, b)));
  }

  inline Vector4D Reject(const Vector4D& a, const Vector4D& b)
  {
	  return (a - b * (Dot(a, b) / Dot(b, b)));
  }
}
