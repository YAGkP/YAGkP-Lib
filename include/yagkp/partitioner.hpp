#pragma once

#include <vector>

#include "yagkp/graph.hpp"
#include "yagkp/types.hpp"

namespace yagkp::Partitioner {
	// graph: input graph to partition          | graph
	// k: number of parts (k >= 1)              | 3
	// returns: vector of part indices (0..k-1) | {0, 0, 1, 1, 2}
	Partition GetGraphKPartition(const UndirectedGraph& graph, const int_t k);
} // namespace yagkp::Partitioner