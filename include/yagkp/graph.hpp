#pragma once

#include <utility>
#include <vector>

#include "mmio/matrix.hpp"
#include "yagkp/types.hpp"

namespace yagkp {
	struct Graph {

		int_t n = 0;
		int_t m = 0;

		int_t total_vertex_weight = 0;

		std::vector<int_t> adjncy;
		std::vector<int_t> xadj;
		std::vector<int_t> vertex_weights;
		std::vector<int_t> edge_weights;

		struct AdjacentIterator {
			const Graph& g;
			int_t v;

			struct Iterator {
				const Graph& g;
				int_t pos;

				Iterator(const Graph& g, int_t pos) : g(g), pos(pos) {
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

		Graph();
		Graph(
		    const std::string& file_name,
		    const std::string& format,
		    bool ignore_eweights = false
		);

		void buildGraph(const spMtx<double>& matrix, bool ignore_eweights);

		int_t getVerticesCount() const noexcept {
			return n;
		}

		int_t getEdgesCount() const noexcept {
			return m;
		}

		int_t getSumOfVertexWeights() const {
			return total_vertex_weight;
		}

		int_t getVertexWeight(int_t v) const {
			return vertex_weights[v];
		}

		Graph selectSubgraph(const std::vector<int_t>& sub_vertices) const;
	};
} // namespace yagkp