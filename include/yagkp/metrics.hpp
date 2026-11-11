#pragma once

#include <vector>

#include "yagkp/graph.hpp"
#include "yagkp/types.hpp"

namespace yagkp::PartitionMetrics {
	// graph: input graph                                                   | graph
	// partition: vector of part indices for each vertex (size = |V|)       | {0, 1, 2}
	// returns: total weight of edges connecting different parts (edge cut) | 4
	int_t GetEdgeCut(const Graph& graph, const Partition& partition);

	// graph: input graph                                                   | graph
	// partition: vector of parts for each vertex (size = |V|)              | {Part::First,
	// Part::Second, Part::First} returns: total weight of edges connecting different parts (edge
	// cut) | 4
	int_t GetEdgeCut(const Graph& graph, const Bisection& partition);

	// graph: input graph                                               | graph (|V| = 6, weight(Ui)
	// = 1) k: number of parts                                               | 4 partition: vector
	// of part indices                                | {0, 1, 0, 2, 3, 1} returns: imbalance
	// measure = k * max(part_weight_fraction - 1/k) | 0.33
	fp_t GetImbalance(const Graph& graph, const int_t k, const Partition& partition);

	// graph: input graph                                   | graph (|V| = 6, weight(Ui) = 1)
	// k: number of parts                                   | 4
	// partition: vector of part indices                    | {0, 1, 0, 2, 3, 1}
	// returns: maximum total vertex weight among all parts | 2
	int_t GetMaxPartWeight(const Graph& graph, const int_t k, const Partition& partition);
} // namespace yagkp::PartitionMetrics
