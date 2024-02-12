#include "PolygonApplicator.h"

#include <algorithm>
#include <iostream>

using namespace std;

namespace Irrigation {

PolygonApplicator::PolygonApplicator(const vector<HwCoord>& points)
{
	const int n = static_cast<int>(points.size());

	if (n < 3)
	{
		m_completed = true;
		return;
	}

	int minI = static_cast<int>(min_element(begin(points), end(points), HwCoord::Comparer()) - begin(points));

	int l = (minI + n - 1) % n;
	int r = (minI + 1) % n;

	// Determine the direction order of the points
	// 1: clockwise with increasing indexes
	// -1: counter-clockwise
	// This is not true for some corner cases, but they are not likely to appear in real scenarios.
	int dir = HwCoord::IsAngleLess(points[l].fi, points[r].fi) ||
		(HwCoord::IsAngleLess(points[l].fi, points[minI].fi) && HwCoord::IsAngleLess(points[minI].fi, points[r].fi))
		? 1 : -1;

	int prevI = (minI - dir + n) % n;
	for (int i = minI, j = n; j--; i = (i + dir + n) % n)
	{
		PolarCoord a(points[prevI]);
		PolarCoord b(points[i]);

		bool isInnerPerpendicular = false;
		PolarCoord h = GetOriginPerpendicular(a, b, isInnerPerpendicular);

		// Split the edge if the perpendicular intersects with it
		if (isInnerPerpendicular)
		{
			m_vertexes.push_back(h);
		}

		m_vertexes.push_back(b);

		prevI = i;
	}

	m_sortedVertexes.resize(m_vertexes.size());
	for (int i = 0; i < static_cast<int>(m_vertexes.size()); ++i)
	{
		m_sortedVertexes[i] = i;
	}

	sort(begin(m_sortedVertexes), end(m_sortedVertexes), PolarCoordComparer(m_vertexes));

	Begin();
}

void PolygonApplicator::Begin()
{
	if (m_vertexes.size() < 3)
	{
		m_completed = true;
		return;
	}

	m_completed = false;
	m_upperPostponed.clear();

	m_visitedVertexes.clear();
	m_visitedVertexes.resize(m_vertexes.size(), false);

	SwitchToVertex(m_sortedVertexes.front(), m_sortedVertexes.front());
}

void PolygonApplicator::SwitchToVertex(int i, int j)
{
	m_i = i;
	m_j = j;

	m_currentR = RoundUp(max(m_vertexes[m_i].r, m_vertexes[m_j].r), m_rIncrement);
	m_visitedVertexes[m_i] = true;
	m_visitedVertexes[m_j] = true;
}

int PolygonApplicator::VertexesBinarySearch(float rSearch) const
{
	int l = 0, r = static_cast<int>(m_sortedVertexes.size());

	while (l < r)
	{
		int m = (l + r) / 2;
		if (m_vertexes[m_sortedVertexes[m]].r > rSearch)
		{
			r = m;
		}
		else
		{
			l = m + 1;
		}
	}

	return l;
}

HwArc PolygonApplicator::NextArc()
{
	HwArc arc;

	if (m_completed)
	{
		return arc;
	}

	int n = static_cast<int>(m_vertexes.size());
	int iNext = (m_i + 1) % n;
	int jNext = (m_j + n - 1) % n;
	m_currentR += m_rIncrement;

	while (m_vertexes[iNext].r < m_currentR &&
		m_vertexes[iNext].r > m_vertexes[m_i].r)
	{
		m_i = iNext;
		m_visitedVertexes[m_i] = true;
		iNext = (m_i + 1) % n;
	}

	while (m_vertexes[jNext].r < m_currentR &&
		m_vertexes[jNext].r > m_vertexes[m_j].r)
	{
		m_j = jNext;
		m_visitedVertexes[m_j] = true;
		jNext = (m_j + n - 1) % n;
	}

	if (((m_vertexes[iNext].r < m_vertexes[m_i].r ||
		m_vertexes[jNext].r < m_vertexes[m_j].r) && m_i == m_j) ||
		(iNext == jNext && m_vertexes[iNext].r == m_currentR))
	{
		if (m_upperPostponed.empty())
		{
			// Polygon completed
			m_completed = true;
			return arc;
		}

		SwitchToVertex(m_upperPostponed.back().first, m_upperPostponed.back().second);
		iNext = (m_i + 1) % n;
		jNext = (m_j + n - 1) % n;
		m_upperPostponed.pop_back();
	}

	// Check if the edge is descending
	if (m_vertexes[iNext].r < m_vertexes[m_i].r)
	{
		while (m_visitedVertexes[iNext] || m_vertexes[iNext].r < m_vertexes[m_i].r)
		{
			m_i = iNext;
			iNext = (m_i + 1) % n;
		}

		if (!m_visitedVertexes[m_i])
		{
			SwitchToVertex(m_i, m_i);
			iNext = (m_i + 1) % n;
			jNext = (m_j + n - 1) % n;
		}
	}

	if (m_vertexes[jNext].r < m_vertexes[m_j].r)
	{
		while (m_visitedVertexes[jNext] || m_vertexes[jNext].r < m_vertexes[m_j].r)
		{
			m_j = jNext;
			jNext = (m_j + n - 1) % n;
		}

		if (!m_visitedVertexes[m_j])
		{
			SwitchToVertex(m_j, m_j);
			iNext = (m_i + 1) % n;
			jNext = (m_j + n - 1) % n;
		}
	}

	if (m_vertexes[jNext].r < m_currentR || m_vertexes[iNext].r < m_currentR ||
		m_vertexes[m_j].r > m_currentR || m_vertexes[m_i].r > m_currentR)
	{
		cerr << "m_currentR should be between the vertexes" << endl;
		return arc;
	}

	PolarCoord leftIntersect = GetIntersection(m_vertexes[iNext], m_vertexes[m_i], m_currentR);
	PolarCoord rightIntersect = GetIntersection(m_vertexes[jNext], m_vertexes[m_j], m_currentR);

	int iSorted = VertexesBinarySearch(m_currentR - m_rIncrement);

	for (; m_vertexes[m_sortedVertexes[iSorted]].r < m_currentR; ++iSorted)
	{
		int t = m_sortedVertexes[iSorted];
		if (!m_visitedVertexes[t] &&
			PolarCoord::IsAngleBetween(leftIntersect.fi, rightIntersect.fi, m_vertexes[t].fi))
		{
			m_upperPostponed.emplace_back(t, m_j);
			m_j = t;
			jNext = (m_j + n - 1) % n;
			rightIntersect = GetIntersection(m_vertexes[jNext], m_vertexes[m_j], m_currentR);
		}
	}

	if (EqualFloat(leftIntersect.fi, rightIntersect.fi))
	{
		return NextArc();
	}

	HwCoord leftCoord = leftIntersect.ToHwCoord();
	HwCoord rightCoord = rightIntersect.ToHwCoord();

	arc.fi0 = leftCoord.fi;
	arc.fi1 = rightCoord.fi;
	arc.r = leftCoord.r;

	return arc.IsValid() ? arc : NextArc();
}

} // namespace Irrigation
