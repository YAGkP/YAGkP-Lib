#pragma once

#include <utility>
#include <vector>

#include "mmio/matrix.hpp"
#include "yagkp/types.hpp"

namespace yagkp {
	// Undirected graph with weighted vertices and edges, in CSR format.
	//
	// Every edge {u, v} is stored twice: as v in the neighbours of u and as u
	// in the neighbours of v, with the same weight. So adjncy and edge_weights
	// have 2m entries. The neighbours of v are adjncy[xadj[v] .. xadj[v + 1]),
	// and edge_weights holds the weights of these edges at the same positions.
	// An input file must therefore describe a symmetric matrix
	struct UndirectedGraph {
		int_t n = 0; // number of vertices
		int_t m = 0; // number of edges (each stored twice, see above)

		int_t total_vertex_weight = 0; // sum of vertex_weights

		std::vector<int_t> adjncy;
		std::vector<int_t> xadj;
		std::vector<int_t> vertex_weights;
		std::vector<int_t> edge_weights;

		struct AdjacentIterator {
			const UndirectedGraph& g;
			int_t v;

			struct Iterator {
				const UndirectedGraph& g;
				int_t pos;

				Iterator(const UndirectedGraph& g, int_t pos) : g(g), pos(pos) {
				}

				bool operator!=(const Iterator& other) const {
					return pos != other.pos;
				}

				void operator++() {
					++pos;
				}

				std::pair<int_t, int_t> operator*() const {
					return {g.adjncy[pos], g.edge_weights[pos]};
				}
			};

			Iterator begin() const {
				return Iterator(g, g.xadj[v]);
			}
			Iterator end() const {
				return Iterator(g, g.xadj[v + 1]);
			}
		};

		AdjacentIterator operator[](int_t v) const {
			return AdjacentIterator{*this, v};
		}

		UndirectedGraph();
		UndirectedGraph(
		    const std::string& file_name,
		    const std::string& format,
		    bool ignore_eweights = false
		);

		UndirectedGraph selectSubgraph(const std::vector<int_t>& sub_vertices) const;

	  private:
		void buildGraph(const spMtx<double>& matrix, bool ignore_eweights);
	};
} // namespace yagkp