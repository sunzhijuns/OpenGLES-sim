/*
 * Copyright (c) 2013, Liou Jhe-Yu <lioujheyu@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 *	@file common.h
 *  @brief Data structures and functions for common use.
 *  @author Liou Jhe-Yu(lioujheyu@gmail.com)
 */
#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include "GPU/gpu_config.h"
#include "GPU/instruction_def.h"

#ifdef USE_SSE
#	include <x86intrin.h>
#	include <malloc.h>
#	define _MM_ALIGN16 __attribute__((aligned (16)))
#endif // USE_SSE

#define VERTEX_SHADER 0
#define FRAGMENT_SHADER 1

/**
 *	@brief Vector class with 4 floating component
 *
 *	Vector component-wised operation instruction(using SSE).
 *
 *	This SSE vector class's implementation is referred to this site:
 *	http://fastcpp.blogspot.tw/2011/12/simple-vector3-class-with-sse-support.html
 *	It works like a charm.
 */
#ifdef USE_SSE
struct _MM_ALIGN16 floatVec4
{
#	ifdef __WIN32__
	inline void* operator new[](size_t x) { return _aligned_malloc(x, 16); }
	inline void  operator delete[](void* x) { if (x) _aligned_free(x); }
#	else
	inline void* operator new[](size_t x) { return memalign(16, x); }
	inline void  operator delete[](void* x) { if(x) free(x); }
#	endif //WIN32

	union {
		__m128 sse;
		struct { float x, y, z, w; };
		struct { int ix, iy, iz, iw;};
		struct { float r, g, b, a; };
		struct { float s, t, p, q; };
	};

	inline floatVec4() {}

	inline floatVec4(float v) :
		sse(_mm_set1_ps(v)) {}

	inline floatVec4(float xv, float yv, float zv, float wv) :
		sse(_mm_setr_ps(xv, yv, zv, wv)) {}

	inline floatVec4(__m128 m) : sse(m) {}

    inline const floatVec4 operator+(const floatVec4 &other) const
    {
		return _mm_add_ps(sse, other.sse);
    }

    inline const floatVec4 operator+(const float other) const
    {
        return _mm_add_ps(sse, _mm_set1_ps(other));
    }

    inline const floatVec4 operator-(const floatVec4 &other) const
    {
		return _mm_sub_ps(sse, other.sse);
    }

    inline const floatVec4 operator-(const float other) const
    {
    	return _mm_sub_ps(sse, _mm_set1_ps(other));
    }

    inline const floatVec4 operator*(const floatVec4 &other) const
	{
		return _mm_mul_ps(sse, other.sse);
	}

	inline const floatVec4 operator*(const float other) const
    {
    	return _mm_mul_ps(sse, _mm_set1_ps(other));
    }

    inline const floatVec4 operator/(const floatVec4 &other) const
	{
		return _mm_div_ps(sse, other.sse);
	}

    inline const floatVec4 operator/(const float other) const
    {
		return _mm_div_ps(sse, _mm_set1_ps(other));
    }
#else
struct floatVec4
{
	union
	{
		struct { float x, y, z, w; };
		struct { int ix, iy, iz, iw;};
		struct { float r, g, b, a; };
		struct { float s, t, p, q; };
	};

	inline floatVec4() {}

	inline floatVec4(float v) :
		x(v), y(v), z(v), w(v) {}

	inline floatVec4(float xv, float yv, float zv, float wv) :
		x(xv), y(yv), z(zv), w(wv) {}

	inline const floatVec4 operator+(const floatVec4 &other) const
	{
		floatVec4 tmp;
		tmp.x = x + other.x;
		tmp.y = y + other.y;
		tmp.z = z + other.z;
		tmp.w = w + other.w;
		return tmp;
	}

	inline const floatVec4 operator+(const float other) const
	{
		floatVec4 tmp;
		tmp.x = x+other;
		tmp.y = y+other;
		tmp.z = z+other;
		tmp.w = w+other;
		return tmp;
	}

	inline const floatVec4 operator-(const floatVec4 &other) const
	{
		floatVec4 tmp;
		tmp.x = x - other.x;
		tmp.y = y - other.y;
		tmp.z = z - other.z;
		tmp.w = w - other.w;
		return tmp;
	}

	inline const floatVec4 operator-(const float other) const
	{
		floatVec4 tmp;
		tmp.x = x - other;
		tmp.y = y - other;
		tmp.z = z - other;
		tmp.w = w - other;
		return tmp;
	}

	inline const floatVec4 operator*(const floatVec4 &other) const
	{
		floatVec4 tmp;
		tmp.x = x * other.x;
		tmp.y = y * other.y;
		tmp.z = z * other.z;
		tmp.w = w * other.w;
		return tmp;
	}

	inline const floatVec4 operator*(const float other) const
	{
		floatVec4 tmp;
		tmp.x = x * other;
		tmp.y = y * other;
		tmp.z = z * other;
		tmp.w = w * other;
		return tmp;
	}

	inline const floatVec4 operator/(const floatVec4 &other) const
	{
		floatVec4 tmp;
		tmp.x = x / other.x;
		tmp.y = y / other.y;
		tmp.z = z / other.z;
		tmp.w = w / other.w;
		return tmp;
	}

	inline const floatVec4 operator/(const float other) const
	{
		floatVec4 tmp;
		tmp.x = x / other;
		tmp.y = y / other;
		tmp.z = z / other;
		tmp.w = w / other;
		return tmp;
	}
#endif //USE_SSE

	inline const floatVec4 operator>(const floatVec4 &other) const
    {
        floatVec4 tmp;
        tmp.x = (x > other.x)?1.0 : 0.0;
        tmp.y = (y > other.y)?1.0 : 0.0;
        tmp.z = (z > other.z)?1.0 : 0.0;
        tmp.w = (w > other.w)?1.0 : 0.0;
        return tmp;
    }

    inline const floatVec4 operator>=(const floatVec4 &other) const
    {
        floatVec4 tmp;
 		tmp.x = (x >= other.x)?1.0 : 0.0;
        tmp.y = (y >= other.y)?1.0 : 0.0;
        tmp.z = (z >= other.z)?1.0 : 0.0;
        tmp.w = (w >= other.w)?1.0 : 0.0;
        return tmp;
    }

    inline const floatVec4 operator<(const floatVec4 &other) const
    {
        floatVec4 tmp;
        tmp.x = (x < other.x)?1.0 : 0.0;
        tmp.y = (y < other.y)?1.0 : 0.0;
        tmp.z = (z < other.z)?1.0 : 0.0;
        tmp.w = (w < other.w)?1.0 : 0.0;
        return tmp;
    }

    inline const floatVec4 operator<=(const floatVec4 &other) const
    {
        floatVec4 tmp;
        tmp.x = (x <= other.x)?1.0 : 0.0;
        tmp.y = (y <= other.y)?1.0 : 0.0;
        tmp.z = (z <= other.z)?1.0 : 0.0;
        tmp.w = (w <= other.w)?1.0 : 0.0;
        return tmp;
    }

    inline const floatVec4 operator==(const floatVec4 &other) const
    {
        floatVec4 tmp;
        tmp.x = (x == other.x)?1.0 : 0.0;
        tmp.y = (y == other.y)?1.0 : 0.0;
        tmp.z = (z == other.z)?1.0 : 0.0;
        tmp.w = (w == other.w)?1.0 : 0.0;
        return tmp;
    }

	inline const floatVec4 operator!=(const floatVec4 &other) const
    {
        floatVec4 tmp;
        tmp.x = (x != other.x)?1.0 : 0.0;
        tmp.y = (y != other.y)?1.0 : 0.0;
        tmp.z = (z != other.z)?1.0 : 0.0;
        tmp.w = (w != other.w)?1.0 : 0.0;
        return tmp;
    }
};

//component-wise maximums
inline const floatVec4 fvmax(const floatVec4& x, const floatVec4& y)
{
	floatVec4 tmp;
#ifdef USE_SSE
	tmp.sse = _mm_max_ps(x.sse, y.sse);
#else
	tmp.x = std::max(x.x, y.x);
	tmp.y = std::max(x.y, y.y);
	tmp.z = std::max(x.z, y.z);
	tmp.w = std::max(x.w, y.w);
#endif //USE_SSE
	return tmp;
}

//component-wise minimums
inline const floatVec4 fvmin(const floatVec4& x, const floatVec4& y)
{
	floatVec4 tmp;
#ifdef USE_SSE
	tmp.sse = _mm_min_ps(x.sse, y.sse);
#else
	tmp.x = std::min(x.x, y.x);
	tmp.y = std::min(x.y, y.y);
	tmp.z = std::min(x.z, y.z);
	tmp.w = std::min(x.w, y.w);
#endif //USE_SSE
	return tmp;
}

#ifdef USE_SSE
inline const __m128 sse_dot4_ps(__m128 a, __m128 b)
{
#	if defined(__SSE4_1__)//not yet verified
		return _mm_dp_ps(a, b, 0xff);
#	elif defined(__SSSE3__)
		__m128 t1 = _mm_mul_ps(a, b);
		__m128 t2 = _mm_hadd_ps(t1, t1);
		__m128 dp = _mm_hadd_ps(t2, t2);
		return dp;
#	else   //SSE2
		__m128 t1 = _mm_mul_ps(a, b);
		__m128 t2 = _mm_shuffle_ps(t1, t1, 0x93);
		__m128 t3 = _mm_add_ps(t1, t2);
		__m128 t4 = _mm_shuffle_ps(t3, t3, 0x4e);
		__m128 dp = _mm_add_ps(t3, t4);
		return dp;
#	endif
}
#endif //USE_SSE

// dot product with another vector
inline float dot(const floatVec4 &x, const floatVec4& other)
{
#ifdef USE_SSE
	return _mm_cvtss_f32(sse_dot4_ps(x.sse, other.sse));
#else
	floatVec4 tmp;
	tmp = x * other;
	return (tmp.x + tmp.y + tmp.z + tmp.w);
#endif //USE_SSE
}

inline const floatVec4 fvabs(const floatVec4 &x)
{
	floatVec4 tmp;
	tmp.x = fabs(x.x);
	tmp.y = fabs(x.y);
	tmp.z = fabs(x.z);
	tmp.w = fabs(x.w);
	return tmp;
}

inline const floatVec4 fvceil(const floatVec4 &x)
{
	floatVec4 tmp;
	tmp.x = ceil(x.x);
	tmp.y = ceil(x.y);
	tmp.z = ceil(x.z);
	tmp.w = ceil(x.w);
	return tmp;
}

inline const floatVec4 fvfloor(const floatVec4 &x)
{
	floatVec4 tmp;
	tmp.x = floor(x.x);
	tmp.y = floor(x.y);
	tmp.z = floor(x.z);
	tmp.w = floor(x.w);
	return tmp;
}

inline const floatVec4 fvround(const floatVec4 &x)
{
	floatVec4 tmp;
	tmp.x = round(x.x);
	tmp.y = round(x.y);
	tmp.z = round(x.z);
	tmp.w = round(x.w);
	return tmp;
}

inline const floatVec4 fvtrunc(const floatVec4 &x)
{
	floatVec4 tmp;
	tmp.x = trunc(x.x);
	tmp.y = trunc(x.y);
	tmp.z = trunc(x.z);
	tmp.w = trunc(x.w);
	return tmp;
}

//Returns the fractional portion of each input component
inline const floatVec4 fvfrc(const floatVec4 &x)
{
	floatVec4 tmp;
	tmp.x = x.x - floor(x.x);
	tmp.y = x.y - floor(x.y);
	tmp.z = x.z - floor(x.z);
	tmp.w = x.w - floor(x.w);
	return tmp;
}

//Converts input integers to float
inline const floatVec4 fvInt2Float(const floatVec4 &x)
{
	floatVec4 tmp;
	tmp.x = (float)x.x;
	tmp.y = (float)x.y;
	tmp.z = (float)x.z;
	tmp.w = (float)x.w;
	return tmp;
}

inline const floatVec4 fvrcp(const floatVec4& x)
{
#ifdef USE_SSE
	return _mm_rcp_ps(x.sse);
#else
	return floatVec4(1.0/x.x, 1.0/x.y, 1.0/x.z, 1.0/x.w);
#endif
}

/**
 *	@brief Fast inverse square root
 *	Reference: http://en.wikipedia.org/wiki/Fast_inverse_square_root
 *  Attention!! the variable "i" needs to be declared as integer which is
 *	defined as long integer originally. Some compiler has different definition
 *	in long integer type and make the result compromised.
 */
inline const float Q_rsqrt(float number)
{
	int32_t i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( int32_t * ) &y;                    // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
	//y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

struct fixColor4
{
    unsigned char r,g,b,a;

    inline fixColor4() {}

    inline fixColor4(unsigned char rv, unsigned char gv, unsigned char bv, unsigned char av)
    {
    	r = rv;
    	g = gv;
    	b = bv;
    	a = av;
    }

    inline fixColor4& operator=(const fixColor4 &rhs)
    {
        if (this == &rhs)
            return *this;
        r = rhs.r;
        g = rhs.g;
        b = rhs.b;
        a = rhs.a;
        return *this;
    }

    inline const fixColor4 operator*(const float other) const
    {
        fixColor4 tmp;
        tmp.r = r*other;
        tmp.g = g*other;
        tmp.b = b*other;
        tmp.a = a*other;
        return tmp;
    }

    inline const fixColor4 operator/(const float other) const
    {
        fixColor4 tmp;
        tmp.r = r/other;
        tmp.g = g/other;
        tmp.b = b/other;
        tmp.a = a/other;
        return tmp;
    }

    inline const fixColor4 operator+(const fixColor4 &other) const
    {
        fixColor4 tmp;
        tmp.r = r + other.r;
        tmp.g = g + other.g;
        tmp.b = b + other.b;
        tmp.a = a + other.a;
        return tmp;
    }
};

inline const fixColor4 fv2bv(const floatVec4 &fv)
{
	fixColor4 bvtmp;

	bvtmp.r = floor(fv.x == 1.0 ? 255 : fv.x * 256.0);
	bvtmp.g = floor(fv.y == 1.0 ? 255 : fv.y * 256.0);
	bvtmp.b = floor(fv.z == 1.0 ? 255 : fv.z * 256.0);
	bvtmp.a = floor(fv.w == 1.0 ? 255 : fv.w * 256.0);
	return bvtmp;
}

struct textureImage
{
	inline textureImage():maxLevel(-1),border(0),data{NULL}{}

    int				maxLevel;
    unsigned int	border;
	unsigned int	widthLevel[13];
	unsigned int	heightLevel[13];

    unsigned char	*data[13];

    inline textureImage& operator=(const textureImage &rhs)
    {
    	if (this == &rhs)
            return *this;
        maxLevel = rhs.maxLevel;
        border = rhs.border;

        for (int i=0;i<13;i++) {
			data[i] = rhs.data[i];
			widthLevel[i] = rhs.widthLevel[i];
			heightLevel[i] = rhs.heightLevel[i];
		}

        return *this;
    }
};

struct operand
{
	int id;
	int type;
	int ccMask;
	int ccModifier;
	bool abs;
	bool inverse;
	int modifier;
	floatVec4 val;

	inline operand() { Init(); }

	inline void Init()
	{
		id = 0;
		type = 0;
		ccMask = 0;
		ccModifier = 0;
		abs = false;
		inverse = false;
		modifier = 0;
		val.x = val.y = val.z = val.w = 0;
	}

	void Print()
	{
		if (type != INST_NO_TYPE) {
			if (type == INST_CONSTANT) {
				printf(" (%f %f %f %f)",val.x,val.y,val.z,val.w);
			}
			else {
				if (inverse)
					printf(" -%d[%d].%x", type, id, modifier);
				else
					printf(" %d[%d].%x", type, id, modifier);
			}
		}
	}
};

struct scalarOperand
{
	int id;
	int type;
	int ccMask;
	int ccModifier;
	bool abs;
	bool inverse;
	float val;

	inline scalarOperand() { Init(); }

	inline void Init()
	{
		id = 0;
		type = 0;
		ccMask = 0;
		ccModifier = 0;
		abs = false;
		inverse = false;
		val = 0;
	}

	void Print()
	{
		if (type != INST_NO_TYPE) {
			if (type == INST_CONSTANT) {
				printf(" (%f)",val);
			}
			else {
				if (inverse)
					printf(" -%d[%d]", type, id);
				else
					printf(" %d[%d]", type, id);
			}
		}
	}
};

struct instruction
{
	int op;
	bool opModifiers[12];
	operand dst;
	operand src[3];
	int tid, tType;

	inline instruction() { Init(); }

	inline void Init()
	{
		op = 0;
		for (unsigned int i=0; i<12; i++)
			opModifiers[i] = false;
		tid = -1;
		tType = 0;
		dst.Init();
		src[0].Init();
		src[1].Init();
		src[2].Init();
	}

	void Print()
	{
		printf("%d.",op);
		for (unsigned int i=0; i<12; i++)
			printf("%d",(opModifiers[i])?1:0);
		dst.Print();
		src[0].Print();
		src[1].Print();
		src[2].Print();
		if (tid != -1)
			printf(" Tex%d.%d", tid, tType);
		printf("\n");
	}
};

struct scalarInstruction
{
	int op;
	bool opModifiers[12];
	scalarOperand dst;
	scalarOperand src[3];
	int tid, tType;

	inline scalarInstruction() { Init(); }

	inline void Init()
	{
		op = 0;
		for (unsigned int i=0; i<12; i++)
			opModifiers[i] = false;
		tid = -1;
		tType = 0;
		dst.Init();
		src[0].Init();
		src[1].Init();
		src[2].Init();
	}

	void Print()
	{
		printf("%d.",op);
		for (unsigned int i=0; i<12; i++)
			printf("%d",(opModifiers[i])?1:0);
		dst.Print();
		src[0].Print();
		src[1].Print();
		src[2].Print();
		if (tid != -1)
			printf(" Tex%d.%d", tid, tType);
		printf("\n");
	}
};

template <typename T> const T& MIN3(const T& a, const T& b, const T& c)
{
  return (b<a)?((c<b)?c:b):((c<a)?c:a);
}

template <typename T> const T& MAX3(const T& a, const T& b, const T& c)
{
  return (b>a)?((c>b)?c:b):((c>a)?c:a);
}

template <typename T> T CLAMP(const T& V, const T& L, const T& H)
{
  return V < L ? L : (V > H ? H : V);
}
#endif // COMMON_H_INCLUDED
