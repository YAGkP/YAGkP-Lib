#pragma once

#include <vector>

#include "yagkp/coarse_level.hpp"
#include "yagkp/graph.hpp"
#include "yagkp/types.hpp"

namespace yagkp::Uncoarser {
	/**
	 * @brief Projects the partition of the coarsened graph onto the uncoarsened graph.
	 *
	 * @param graph original graph
	 * @param levels coarse levels of the graph, from the first contraction to the coarsest
	 * @param partition partition of the coarsest graph
	 * @param C1 restriction on the 1st part
	 * @param C2 restriction on the 2nd part
	 * @return partition of the original graph
	 */
	Bisection RestorePartition(
	    const UndirectedGraph& graph,
	    const std::vector<CoarseLevel>& levels,
	    Bisection partition,
	    const int_t C1,
	    const int_t C2
	);
} // namespace yagkp::Uncoarser