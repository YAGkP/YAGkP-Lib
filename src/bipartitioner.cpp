#include "yagkp/bipartitioner.hpp"

#include <stdexcept>

#include "yagkp/buckets.hpp"
#include "yagkp/config.hpp"
#include "yagkp/metrics.hpp"
#include "yagkp/queue.hpp"
#include "yagkp/random.hpp"

namespace yagkp::Bipartitioner {

	namespace {
		template <typename BipartitionAlgorithm>
		Bisection GetBestBipartition(
		    const Graph& graph,
		    const BipartitionAlgorithm& bipartition_algorithm
		) {
			const int_t launches = ProgramConfig::bipartitioning_launches_count;

			std::vector<Bisection> partitions(launches);
			std::vector<int_t> edge_cuts(launches);

#pragma omp taskloop default(none) grainsize(1)                                                    \
    shared(graph, bipartition_algorithm, partitions, edge_cuts) firstprivate(launches)
			for (int_t i = 0; i < launches; ++i) {
				partitions[i] = bipartition_algorithm();
				edge_cuts[i]  = PartitionMetrics::GetEdgeCut(graph, partitions[i]);
			}

			int_t best = 0;
			for (int_t i = 1; i < launches; ++i) {
				if (edge_cuts[i] < edge_cuts[best]) {
					best = i;
				}
			}
			return std::move(partitions[best]);
		}

		Bisection GraphGrowingAlgorithm(
		    const Graph& graph,
		    const int_t C1,
		    [[maybe_unused]] const int_t C2
		) {
			return GetBestBipartition(graph, [&]() {
				const int_t n = graph.n;

				Bisection partition(n, Part::Second);
				std::vector<bool> visited(n, false);

				Queue<int_t> q;
				q.reserve(n / 2);

				std::vector<int_t> order = GetRandomPermutation(n);

				for (int_t start_V: order) {
					if (graph.vertex_weights[start_V] <= C1) {
						q.push(start_V);
						visited[start_V] = true;
						break;
					}
				}

				int_t current_weight = 0;

				while (!q.empty()) {
					int_t curr_V = q.front();
					q.pop();

					if (current_weight + graph.vertex_weights[curr_V] > C1) {
						continue;
					}

					partition[curr_V] = Part::First;
					current_weight += graph.vertex_weights[curr_V];

					for (auto [next_V, w]: graph[curr_V]) {
						if (!visited[next_V]) {
							visited[next_V] = true;
							q.push(next_V);
						}
					}
				}

				return partition;
			});
		}

		Bisection GreedyGraphGrowingAlgorithm(
		    const Graph& graph,
		    const int_t C1,
		    [[maybe_unused]] const int_t C2
		) {
			const int_t n = graph.n;

			int_t max_possible_gain = 0;
			std::vector<int_t> sum_of_incident_edges_weights(n, 0);
			for (int_t curr_V = 0; curr_V < n; ++curr_V) {
				int_t sum = 0;
				for (auto [next_V, w]: graph[curr_V]) {
					sum += w;
				}
				sum_of_incident_edges_weights[curr_V] = sum;
				setmax(max_possible_gain, sum);
			}

			return GetBestBipartition(graph, [&]() {
				Bisection partition(n, Part::Second);
				std::vector<bool> blocked(n, false);
				int_t current_weight = 0;

				BucketPriorityQueue pq(n, -max_possible_gain, max_possible_gain);
				std::vector<int_t> order = GetRandomPermutation(n);

				auto touch = [&](int_t u, int_t w) {
					if (blocked[u])
						return;
					if (pq.contains(u)) {
						pq.add(+2 * w, u);
					}
					else {
						pq.insert(2 * w - sum_of_incident_edges_weights[u], u);
					}
				};

				int_t next_unchecked = 0;

				bool progressed = true;
				while (progressed) {
					progressed = false;

					for (; next_unchecked < n; next_unchecked++) {
						int_t V = order[next_unchecked];
						if (blocked[V])
							continue;
						if (current_weight + graph.getVertexWeight(V) > C1)
							continue;

						progressed   = true;
						partition[V] = Part::First;
						blocked[V]   = true;
						current_weight += graph.getVertexWeight(V);

						for (auto [u, w]: graph[V])
							touch(u, w);
						break;
					}

					while (!pq.empty()) {
						auto [gain, curr_V] = pq.extract();
						blocked[curr_V]     = true;

						if (current_weight + graph.vertex_weights[curr_V] > C1) {
							continue;
						}

						current_weight += graph.vertex_weights[curr_V];
						partition[curr_V] = Part::First;

						for (auto [next_V, w]: graph[curr_V]) {
							touch(next_V, w);
						}
					}
				}
				return partition;
			});
		}
	} // namespace

	Bisection GetGraphBipartition(const Graph& graph, const int_t C1, const int_t C2) {

		using Method = ProgramConfig::BipartitioningMethod;

		switch (ProgramConfig::bipartitioning_method) {
		case Method::GraphGrowingAlgorithm:
			return GraphGrowingAlgorithm(graph, C1, C2);

		case Method::GreedyGraphGrowingAlgorithm:
			return GreedyGraphGrowingAlgorithm(graph, C1, C2);
		}
		throw std::logic_error("Unknown bipartitioning method");
	}

} // namespace yagkp::Bipartitioner