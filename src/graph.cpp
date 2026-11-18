#include "yagkp/graph.hpp"

#include <numeric>

#include "mmio/matrix.hpp"

namespace yagkp {

	UndirectedGraph::UndirectedGraph() {
	}

	UndirectedGraph::UndirectedGraph(
	    const string& file_name,
	    const string& format,
	    bool ignore_eweights
	) {
		const spMtx<double> matrix(file_name.c_str(), format);

		n = static_cast<int_t>(matrix.m);
		// Every edge is read in both directions, so there are 2m entries
		const int_t entries = static_cast<int_t>(matrix.nz);
		m                   = entries / 2;

		neighbours.reserve(n, entries);
		for (int_t vertex = 0; vertex < n; ++vertex) {
			neighbours.append_row();
			for (int_t pos = matrix.Rst[vertex]; pos < matrix.Rst[vertex + 1]; ++pos) {
				neighbours.push_back(matrix.Col[pos]);
			}
		}

		vertex_weights.resize(n);
		for (int_t i = 0; i < n; ++i) {
			vertex_weights[i] = 1;
		}

		total_vertex_weight =
		    std::accumulate(vertex_weights.begin(), vertex_weights.end(), int_t{0});

		edge_weights.resize(entries);
		if (matrix.Val != nullptr && !ignore_eweights) {
			for (int_t i = 0; i < entries; ++i) {
				edge_weights[i] = static_cast<int_t>(matrix.Val[i]);
			}
		}
		else {
			for (int_t i = 0; i < entries; ++i) {
				edge_weights[i] = 1;
			}
		}
	}

	UndirectedGraph UndirectedGraph::selectSubgraph(const vector<int_t>& sub_vertices) const {
		UndirectedGraph subgraph;

		vector<int_t> original_to_sub(n, -1);
		for (int_t i = 0; i < (int_t)sub_vertices.size(); ++i) {
			original_to_sub[sub_vertices[i]] = i;
		}

		subgraph.n = sub_vertices.size();

		subgraph.vertex_weights.resize(subgraph.n);
		for (int_t i = 0; i < (int_t)sub_vertices.size(); ++i) {
			subgraph.vertex_weights[i] = vertex_weights[sub_vertices[i]];
		}

		subgraph.total_vertex_weight = std::accumulate(
		    subgraph.vertex_weights.begin(),
		    subgraph.vertex_weights.end(),
		    int_t{0}
		);

		for (int_t vertex: sub_vertices) {
			subgraph.neighbours.append_row();
			for (auto [neighbour, weight]: (*this)[vertex]) {
				const int_t sub_vertex = original_to_sub[neighbour];
				if (sub_vertex != -1) {
					subgraph.neighbours.push_back(sub_vertex);
					subgraph.edge_weights.push_back(weight);
				}
			}
		}
		subgraph.m = subgraph.neighbours.value_count() / 2;

		return subgraph;
	}

} // namespace yagkp