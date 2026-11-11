#pragma once

#include <vector>

#include "yagkp/graph.hpp"
#include "yagkp/types.hpp"

namespace yagkp {
	struct CoarseLevel {
		vector<int_t> uncoarse_to_coarse;
		vector<vector<int_t>> coarse_to_uncoarse;
		UndirectedGraph coarsened_graph;
		vector<int_t> included_ew_in_vertex;
	};
} // namespace yagkp