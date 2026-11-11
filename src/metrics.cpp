#include "yagkp/metrics.hpp"

namespace yagkp::PartitionMetrics {
	int_t GetEdgeCut(const UndirectedGraph& graph, const Partition& partition) {
		int_t edge_cut = 0;

		for (int_t curr_V = 0; curr_V < graph.n; ++curr_V) {
			for (auto [next_V, w]: graph[curr_V]) {
				if (curr_V < next_V && partition[curr_V] != partition[next_V]) {
					edge_cut += w;
				}
			}
		}

		return edge_cut;
	}

	int_t GetEdgeCut(const UndirectedGraph& graph, const Bisection& partition) {
		int_t edge_cut = 0;

		for (int_t curr_V = 0; curr_V < graph.n; ++curr_V) {
			for (auto [next_V, w]: graph[curr_V]) {
				if (curr_V < next_V && partition[curr_V] != partition[next_V]) {
					edge_cut += w;
				}
			}
		}

		return edge_cut;
	}

	namespace {
		// Relative weight of every part: the fraction of the total vertex weight
		vector<fp_t> GetBalances(
		    const UndirectedGraph& graph,
		    const int_t k,
		    const Partition& partition
		) {
			vector<fp_t> balances(k, 0.0);

			int_t total_W = graph.total_vertex_weight;

			for (int_t curr_V = 0; curr_V < graph.n; ++curr_V) {
				balances[partition[curr_V]] += static_cast<fp_t>(graph.vertex_weights[curr_V]);
			}

			for (int_t curr_V = 0; curr_V < k; ++curr_V) {
				balances[curr_V] = balances[curr_V] / total_W;
			}

			return balances;
		}
	} // namespace

	fp_t GetImbalance(const UndirectedGraph& graph, const int_t k, const Partition& partition) {
		vector<fp_t> balances = GetBalances(graph, k, partition);

		fp_t imbalance = balances[0] - 1.0 / k;
		for (int_t curr_V = 1; curr_V < k; ++curr_V) {
			if (balances[curr_V] - 1.0 / k > imbalance) {
				imbalance = balances[curr_V] - 1.0 / k;
			}
		}
		return imbalance * k;
	}

	int_t GetMaxPartWeight(
	    const UndirectedGraph& graph,
	    const int_t k,
	    const Partition& partition
	) {
		vector<int_t> weights(k, 0);

		for (int_t curr_V = 0; curr_V < graph.n; ++curr_V) {
			weights[partition[curr_V]] += graph.vertex_weights[curr_V];
		}

		int_t max_weight = 0;
		for (int_t i = 0; i < k; ++i) {
			if (max_weight < weights[i]) {
				max_weight = weights[i];
			}
		}

		return max_weight;
	}
} // namespace yagkp::PartitionMetrics