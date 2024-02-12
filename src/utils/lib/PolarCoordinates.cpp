#include "PolarCoordinates.h"

PolarCoord GetOriginPerpendicular(const PolarCoord& a, const PolarCoord& b, bool& isInnerPerpendicular)
{
	PolarCoord h;
	isInnerPerpendicular = false;

	float gamma = PolarCoord::SubtractAngle(a.fi, b.fi);
	if (EqualFloat(gamma, 0.f) || EqualFloat(gamma, c_fpi))
	{
		return h;
	}

	float a_sqr = a.r * a.r;
	float b_sqr = b.r * b.r;
	float c_sqr = a_sqr + b_sqr - 2.f * a.r * b.r * cosf(gamma);
	float c = sqrtf(c_sqr);

	float cos_alpha = (c_sqr + a_sqr - b_sqr) / (2.f * a.r * c);
	//h.r = a.r * b.r * sinf(fabsf(gamma)) / c;
	h.r = a.r * sqrtf(1 - cos_alpha * cos_alpha);

	float alpha = acosf(cos_alpha);
	float delta = c_fpi_2 - alpha;

	isInnerPerpendicular = std::max(a_sqr, b_sqr) - h.r * h.r < c_sqr;

	h.SetFi(a.fi - delta * Sgn(gamma));

	return h;
}

PolarCoord GetIntersection(const PolarCoord& a, const PolarCoord& b, float r)
{
	PolarCoord res(r, 0.f);
	bool isInnerPerpendicular = false;
	PolarCoord h = GetOriginPerpendicular(a, b, isInnerPerpendicular);
	
	if (EqualFloat(h.r, 0.f))
	{
		res.SetFi(a.fi);
		return res;
	}

	float alfa = acosf(h.r / r);
	res.SetFi(h.fi + alfa);
	if (!PolarCoord::IsAngleBetween(a.fi, b.fi, res.fi))
	{
		res.SetFi(h.fi - alfa);
	}
	
	return res;
}

std::ostream& operator << (std::ostream& os, PolarCoord a)
{
	return os << "(" << a.r << ", " << a.fi << ")";
}
