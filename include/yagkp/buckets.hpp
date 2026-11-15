#pragma once

#include <limits>
#include <utility>
#include <vector>

#include "yagkp/assert.hpp"
#include "yagkp/types.hpp"

namespace yagkp {
	/**
	 * @brief Max-priority queue of vertices by gain, built on buckets.
	 *
	 * Every bucket is a list of vertices whose gains fall into one range of values. extract()
	 * takes a vertex from the highest bucket that is not empty.
	 *
	 * For small gains every bucket holds one gain value, and extract() returns a vertex with the
	 * largest gain. For large gains every bucket holds 2^shift neighbouring values, with at most
	 * MAX_BUCKETS_PER_VERTEX buckets per vertex, so extract() returns some vertex of the highest
	 * bucket. Gains are always stored and returned exactly.
	 *
	 * Inside one bucket, the vertex that was inserted or changed last comes first.
	 */
	class BucketPriorityQueue {
	  protected:
		/**
		 * @brief Neighbours of a vertex in the list of its bucket, -1 if none
		 */
		struct BucketLinks {
			int_t prev_vertex_id = -1;
			int_t next_vertex_id = -1;
		};

		/**
		 * @brief Index of the bucket that holds the gain
		 *
		 * @param gain gain in [min_g, max_g]
		 * @return index of the bucket
		 */
		int_t get_bucket_idx(const int_t gain) const noexcept {
			const int_t bucket_idx = (gain - min_gain) >> shift;

			YAGKP_ASSERT(
			    gain >= min_gain && bucket_idx < static_cast<int_t>(bucket_heads.size()),
			    "gain out of the range given to BucketPriorityQueue"
			);

			return bucket_idx;
		}

		/**
		 * @brief Puts the vertex at the head of the list of the bucket
		 *
		 * @param bucket_idx index of the bucket
		 * @param vertex_id the vertex; it must not be in any bucket
		 */
		void push_front(const int_t bucket_idx, const int_t vertex_id) noexcept {
			links[vertex_id].next_vertex_id = bucket_heads[bucket_idx];
			links[vertex_id].prev_vertex_id = -1;
			if (bucket_heads[bucket_idx] != -1) {
				links[bucket_heads[bucket_idx]].prev_vertex_id = vertex_id;
			}
			bucket_heads[bucket_idx] = vertex_id;
		}

		/**
		 * @brief Removes the vertex from the list of its bucket
		 *
		 * @param bucket_idx index of the bucket the vertex is in
		 * @param vertex_id the vertex
		 */
		void unlink(const int_t bucket_idx, const int_t vertex_id) noexcept {
			const int_t prev_vertex_id = links[vertex_id].prev_vertex_id;
			const int_t next_vertex_id = links[vertex_id].next_vertex_id;

			YAGKP_ASSERT(
			    prev_vertex_id != -1 || bucket_heads[bucket_idx] == vertex_id,
			    "the vertex is not in this bucket"
			);

			if (prev_vertex_id != -1) {
				links[prev_vertex_id].next_vertex_id = next_vertex_id;
			}
			else {
				bucket_heads[bucket_idx] = next_vertex_id;
			}
			if (next_vertex_id != -1) {
				links[next_vertex_id].prev_vertex_id = prev_vertex_id;
			}
		}

		/**
		 * @brief Moves top_bucket_idx down to the highest bucket that is not empty, or to 0 if the
		 * queue is empty
		 */
		void lower_top_bucket() noexcept {
			while (top_bucket_idx > 0 && bucket_heads[top_bucket_idx] == -1) {
				top_bucket_idx--;
			}
		}

	  public:
		/**
		 * @brief Creates an empty queue
		 *
		 * If the range of gains is small, every bucket holds one gain value. Otherwise every bucket
		 * holds 2^shift values, with the smallest shift that gives at most MAX_BUCKETS_PER_VERTEX
		 * buckets per vertex.
		 *
		 * @param n number of vertices in the graph, vertices are 0..n-1
		 * @param min_g the smallest possible gain
		 * @param max_g the largest possible gain
		 */
		BucketPriorityQueue(const int_t n, const int_t min_g, const int_t max_g)
		    : links(n), gains(n, GAIN_FILLER), min_gain(min_g) {
				
			YAGKP_ASSERT(n > 0, "BucketPriorityQueue for an empty graph");

			const int_t range = max_g - min_g;
			if (range >= EXACT_BUCKETS_MIN_LIMIT) {
				while ((range >> shift) >= MAX_BUCKETS_PER_VERTEX * n) {
					++shift;
				}
			}
			bucket_heads.assign((range >> shift) + 1, -1);
		}

		/**
		 * @brief Adds a vertex that is not in the queue yet
		 *
		 * @param gain gain of the vertex, in [min_g, max_g]
		 * @param vertex_id the vertex
		 */
		void insert(const int_t gain, const int_t vertex_id) noexcept {
			YAGKP_ASSERT(!contains(vertex_id), "vertex already present in BucketPriorityQueue");

			const int_t bucket_idx = get_bucket_idx(gain);
			gains[vertex_id]       = gain;
			push_front(bucket_idx, vertex_id);
			setmax(top_bucket_idx, bucket_idx);
			n_vertices++;
		}

		/**
		 * @brief Changes the gain of the vertex; does nothing if the vertex is not in the queue
		 *
		 * @param delta change of the gain; the new gain must stay in [min_g, max_g]
		 * @param vertex_id the vertex
		 */
		void add(const int_t delta, const int_t vertex_id) noexcept {
			if (!contains(vertex_id)) {
				return;
			}

			const int_t old_bucket_idx = get_bucket_idx(gains[vertex_id]);
			gains[vertex_id] += delta;
			const int_t new_bucket_idx = get_bucket_idx(gains[vertex_id]);

			unlink(old_bucket_idx, vertex_id);
			push_front(new_bucket_idx, vertex_id);

			if (new_bucket_idx > top_bucket_idx) {
				top_bucket_idx = new_bucket_idx;
			}
			else {
				lower_top_bucket();
			}
		}

		/**
		 * @brief Removes the first vertex of the top bucket from the queue
		 *
		 * @return {gain, vertex}; the queue must not be empty
		 */
		std::pair<int_t, int_t> extract() noexcept {
			const auto [gain, vertex_id] = top();

			unlink(top_bucket_idx, vertex_id);
			gains[vertex_id] = GAIN_FILLER;
			n_vertices--;
			lower_top_bucket();

			return {gain, vertex_id};
		}

		/**
		 * @brief The first vertex of the top bucket, without removing it
		 *
		 * @return {gain, vertex}; the queue must not be empty
		 */
		std::pair<int_t, int_t> top() const noexcept {
			YAGKP_ASSERT(!empty(), "top() can't be used with empty BucketPriorityQueue");

			const int_t vertex_id = bucket_heads[top_bucket_idx];
			YAGKP_ASSERT(vertex_id != -1, "top_bucket_idx points to an empty bucket");
			return {gains[vertex_id], vertex_id};
		}

		/**
		 * @return whether the queue has no vertices
		 */
		bool empty() const noexcept {
			return n_vertices == 0;
		}

		/**
		 * @param vertex_id the vertex
		 * @return whether the vertex is in the queue
		 */
		bool contains(const int_t vertex_id) const noexcept {
			return gains[vertex_id] != GAIN_FILLER;
		}

	  protected:
		// Every bucket holds one gain value if the range of gains is less than
		// max(EXACT_BUCKETS_MIN_LIMIT, MAX_BUCKETS_PER_VERTEX * n)
		static constexpr int_t EXACT_BUCKETS_MIN_LIMIT = int_t{1} << 22;
		static constexpr int_t MAX_BUCKETS_PER_VERTEX  = 8;

		// Gain of a vertex that is not in the queue
		static constexpr int_t GAIN_FILLER = std::numeric_limits<int_t>::lowest();

		vector<int_t> bucket_heads; // first vertex of every bucket, -1 if empty
		vector<BucketLinks> links;  // links[vertex_id]: neighbours in the list of its bucket
		vector<int_t> gains;        // exact gain of every vertex, GAIN_FILLER if absent

		int_t min_gain;
		int_t shift          = 0; // a bucket holds 2^shift gain values
		int_t top_bucket_idx = 0; // the highest bucket that is not empty
		int_t n_vertices     = 0; // number of vertices in the queue
	};
} // namespace yagkp