#pragma once

#include <PolarCoordinates.h>

#include <queue>
#include <stack>
#include <vector>

namespace Irrigation {

class PolygonApplicator
{
public:
	PolygonApplicator(const std::vector<HwCoord>& points);

	void Begin();
	void SetRIncrement(float rIncrement) { m_rIncrement = rIncrement; }
	HwArc NextArc();

private:
	void SwitchToVertex(int i, int j);
	int VertexesBinarySearch(float rSearch) const;

	struct PolarCoordComparer
	{
		PolarCoordComparer(const std::vector<PolarCoord>& vertexes) :
			m_vertexes(vertexes)
		{}

		bool operator()(int i, int j) const
		{
			const PolarCoord& a = m_vertexes[i];
			const PolarCoord& b = m_vertexes[j];
			return a.r != b.r ? a.r < b.r : a.fi < b.fi;
		}

	private:
		const std::vector<PolarCoord>& m_vertexes;
	};

	std::vector<PolarCoord> m_vertexes;
	std::vector<int> m_sortedVertexes;
	std::vector<bool> m_visitedVertexes;
	std::deque<std::pair<int, int>> m_upperPostponed;

	float m_currentR = 0.f;
	float m_rIncrement = 10.f;
	int m_i = 0, m_j = 0;
	bool m_completed = false;
};

} // namespace Irrigation
