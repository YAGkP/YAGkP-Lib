#pragma once

#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "yagkp/crs.hpp"
#include "yagkp/types.hpp"

namespace yagkp {
	// Undirected graph with weighted vertices and edges, in CSR format.
	//
	// Every edge {u, v} is stored twice: as v in the neighbours of u and as u
	// in the neighbours of v, with the same weight. Row v of neighbours contains its adjacent
	// vertices. The flat edge_weights array uses the same row boundaries.
	// An input file must therefore describe a symmetric matrix
	struct UndirectedGraph {
		int_t n = 0; // number of vertices
		int_t m = 0; // number of edges (each stored twice, see above)

		int_t total_vertex_weight = 0; // sum of vertex_weights

		CRS<int_t> neighbours;
		vector<int_t> vertex_weights;
		vector<int_t> edge_weights;

		auto operator[](int_t v) const {
			const auto& offsets = neighbours.row_offsets();
			auto positions = std::views::iota(offsets[v], offsets[v + 1]);
			return std::views::transform(positions, [this](int_t pos) {
				return std::pair{neighbours.value_buffer()[pos], edge_weights[pos]};
			});
		}

		UndirectedGraph();
		UndirectedGraph(
		    const string& file_name,
		    const string& format,
		    bool ignore_eweights = false
		);

		UndirectedGraph selectSubgraph(const vector<int_t>& sub_vertices) const;
	};
} // namespace yagkp