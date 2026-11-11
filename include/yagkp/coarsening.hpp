#pragma once

#include <vector>

#include "yagkp/coarse_level.hpp"
#include "yagkp/graph.hpp"
#include "yagkp/types.hpp"

namespace yagkp::Coarser {
	// Coarse levels from the first contraction of graph to the coarsest graph.
	// graph itself is not a level and is not copied; if it is already small,
	// there are no levels
	std::vector<CoarseLevel> GetCoarseLevels(const Graph& graph, const int_t k);
} // namespace yagkp::Coarser