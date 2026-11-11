#pragma once

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

#include "yagkp/parallel.hpp"
#include "yagkp/types.hpp"

namespace yagkp {

	inline constexpr std::uint32_t RANDOM_SEED = 42;

	inline std::mt19937& ThreadRng() {
		thread_local std::mt19937 rng{
		    RANDOM_SEED + static_cast<std::uint32_t>(parallel::ThreadId())
		};
		return rng;
	}

	/**
	 * @brief Get the random permutation
	 *
	 * @param n size of permutation (ex. 5)
	 * @return shuffled vector of integers 0..n-1 (ex. {4, 2, 3, 0, 1})
	 */
	inline vector<int_t> GetRandomPermutation(int_t n) {
		vector<int_t> p(n);
		std::iota(p.begin(), p.end(), 0);
		std::shuffle(p.begin(), p.end(), ThreadRng());
		return p;
	}
} // namespace yagkp