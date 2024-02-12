#pragma once

#include "MathUtils.h"

#include <fstream>

struct HwCoord
{
	struct Comparer
	{
		bool operator()(HwCoord a, HwCoord b) const
		{
			return a.r != b.r ? a.r < b.r : a.fi < b.fi;
		}
	};

	static constexpr int c_angleRange = 0x1000;

	HwCoord() = default;
	HwCoord(int _r, int _fi) : r(_r), fi(_fi)
	{}

	bool operator==(const HwCoord& other) const
	{
		return other.r == r && other.fi == fi;
	}

	float GetRadians() const
	{
		return static_cast<float>(fi) * c_fpi / static_cast<float>(c_angleRange / 2);
	}

	void SetFromRadians(float ffi)
	{
		fi = static_cast<int>(ffi / c_fpi * static_cast<float>(c_angleRange / 2));
	}

	static bool IsAngleLess(int alpha, int beta)
	{
		int d = (beta - alpha + HwCoord::c_angleRange * 3 / 2) % HwCoord::c_angleRange;
		return d > HwCoord::c_angleRange / 2;
	}

	static int AngleAbsDiff(int alpha, int beta)
	{
		int d = abs(beta - alpha);
		return d > c_angleRange / 2 ? c_angleRange - d : d;
	}

	int r = 0;
	int fi = 0;
};

struct HwArc
{
	bool IsValid() const
	{
		return r > 0 && fi0 != fi1;
	}

	bool operator==(const HwArc& other) const
	{
		return other.r == r && other.fi0 == fi0 && other.fi1 == fi1;
	}

	int r = 0;
	int fi0 = 0;
	int fi1 = 0;
};

struct PolarCoord
{
	PolarCoord() = default;
	PolarCoord(HwCoord coord) :
		r(static_cast<float>(coord.r)),
		fi(coord.GetRadians())
	{}
	PolarCoord(float r0, float fi0) :
		r(r0),
		fi(fi0)
	{}

	HwCoord ToHwCoord() const
	{
		HwCoord res;
		res.SetFromRadians(fi);
		res.r = static_cast<int>(floorf(r + .5f));
		return res;
	}

	void SetFi(float alpha)
	{
		fi = NormalizeAngle(alpha);
	}

	static float SubtractAngle(float alpha, float beta)
	{
		float gamma = alpha - beta;
		if (gamma > M_PI)
		{
			gamma -= c_f2pi;
		}

		if (gamma < -M_PI)
		{
			gamma += c_f2pi;
		}

		return gamma;
	}

	static bool IsAngleBetween(float alpha, float beta, float fi)
	{
		if (alpha >= beta)
		{
			return alpha >= fi && fi >= beta;
		}
		else
		{
			return alpha >= fi || beta >= fi;
		}
	}

	static float NormalizeAngle(float alpha)
	{
		if (alpha >= c_f2pi)
		{
			return alpha - c_f2pi;
		}

		if (std::signbit(alpha))
		{
			return alpha + c_f2pi;
		}

		return alpha;
	}

	float r = 0.f;
	float fi = 0.f;
};

std::ostream& operator << (std::ostream& os, PolarCoord a);
PolarCoord GetOriginPerpendicular(const PolarCoord& a, const PolarCoord& b, bool& isInnerPerpendicular);
PolarCoord GetIntersection(const PolarCoord& a, const PolarCoord& b, float r);
