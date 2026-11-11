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

		adjncy.resize(entries);
		for (int_t i = 0; i < entries; ++i) {
			adjncy[i] = static_cast<int_t>(matrix.Col[i]);
		}

		xadj.resize(n + 1);
		for (int_t i = 0; i < n + 1; ++i) {
			xadj[i] = static_cast<int_t>(matrix.Rst[i]);
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

		vector<bool> is_exist(n, false);

		for (int_t curr_V: sub_vertices) {
			is_exist[curr_V] = true;
		}

		int_t entries = 0;
		for (int_t curr_V: sub_vertices) {
			for (int_t i = xadj[curr_V]; i < xadj[curr_V + 1]; ++i) {
				if (is_exist[adjncy[i]]) {
					++entries;
				}
			}
		}
		subgraph.m = entries / 2;

		subgraph.adjncy.resize(entries);
		subgraph.edge_weights.resize(entries);

		subgraph.xadj.resize(subgraph.n + 1);
		subgraph.xadj[0] = 0;

		int_t edge_pos = 0;

		for (int_t i = 0; i < (int_t)sub_vertices.size(); ++i) {
			int_t curr_V         = sub_vertices[i];
			subgraph.xadj[i + 1] = subgraph.xadj[i];
			for (int_t k = xadj[curr_V]; k < xadj[curr_V + 1]; ++k) {
				int_t next_V = adjncy[k];

				if (is_exist[next_V]) {

					int_t j = original_to_sub[next_V];

					int_t weight = edge_weights[k];

					++subgraph.xadj[i + 1];
					subgraph.adjncy[edge_pos]       = j;
					subgraph.edge_weights[edge_pos] = weight;

					++edge_pos;
				}
			}
		}

		return subgraph;
	}

} // namespace yagkp