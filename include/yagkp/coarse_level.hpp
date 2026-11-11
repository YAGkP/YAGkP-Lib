#pragma once

#include <vector>

#include "yagkp/graph.hpp"
#include "yagkp/types.hpp"

namespace yagkp {
	struct CoarseLevel {
		std::vector<int_t> uncoarse_to_coarse;
		std::vector<std::vector<int_t>> coarse_to_uncoarse;
		UndirectedGraph coarsened_graph;
		std::vector<int_t> included_ew_in_vertex;
	};
} // namespace yagkp