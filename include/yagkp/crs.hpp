#pragma once

#include <cstddef>
#include <functional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "yagkp/assert.hpp"
#include "yagkp/types.hpp"

namespace yagkp {
	/**
	 * @brief Stores variable-length rows in a contiguous buffer
	 *
	 * Offsets always start with zero, including when there are no rows.
	 * Row i occupies values[offsets[i] .. offsets[i + 1]). An empty row has equal
	 * start and end offsets. Values can be accessed as crs[i][j].
	 *
	 * Use operator[] for one row, flat_values() for all elements in row order,
	 * or rows() to iterate over the rows, including empty ones.
	 *
	 * Reserve and append operations may invalidate existing views and iterators.
	 */
	template <typename T> class CRS {
		YAGKP_STATIC_ASSERT(!std::is_same_v<T, bool>, "CRS isn't support bool type");

	public:
		/**
		 * @brief Creates an empty container
		 */
		CRS()           = default;
		CRS(const CRS&) = default;

		/**
		 * @brief Takes the vertex buffer and builds a row for each pair or singleton
		 */
		explicit CRS(Matching matching)
		    requires std::is_same_v<T, int_t>
		    : values(std::move(matching.vertices)) {
			const int_t n     = value_count();
			const int_t pairs = matching.pair_count;

			YAGKP_ASSERT(pairs >= 0 && pairs <= n / 2, "Invalid matching pair count");
			
			offsets.reserve(n - pairs + 1);
			for (int_t pair = 0; pair < pairs; ++pair) {
				offsets.push_back(offsets.back() + 2);
			}
			for (int_t vertex = 2 * pairs; vertex < n; ++vertex) {
				offsets.push_back(vertex + 1);
			}
		}

		CRS(CRS&& other) : CRS() {
			swap(other);
		}

		CRS& operator=(CRS other) noexcept {
			swap(other);
			return *this;
		}

		/**
		 * @brief Creates rows with the requested lengths in one contiguous buffer
		 *
		 * For example, lengths {2, 0, 3} produce offsets {0, 2, 2, 5}.
		 * The second row is empty. All five elements are value-initialized.
		 *
		 * @param row_sizes non-negative lengths of the rows
		 * @return a CRS ready to fill through crs[row][column]
		 */
		static CRS from_row_sizes(std::span<const int_t> row_sizes) {
			CRS result;
			result.reserve(row_sizes.size(), 0);

			int_t row_end = 0;
			for (const int_t row_size: row_sizes) {
				YAGKP_ASSERT(row_size >= 0, "CRS row length must be non-negative");
				row_end += row_size;
				result.offsets.push_back(row_end);
			}

			result.values.resize(row_end);
			return result;
		}

		/**
		 * @brief Reserves storage without changing the number of rows or elements
		 *
		 * @param expected_rows number of rows to reserve space for
		 * @param expected_values total number of elements across all rows
		 */
		void reserve(const int_t expected_rows, const int_t expected_values) {
			YAGKP_ASSERT(
			    expected_rows >= 0 && expected_values >= 0,
			    "expected values in CRS should be non negative"
			);

			offsets.reserve(expected_rows + 1);
			values.reserve(expected_values);
		}

		/**
		 * @brief Adds an empty row at the end
		 */
		void append_row() {
			offsets.push_back(offsets.back());
		}

		/**
		 * @brief Copies existing elements into a new row at the end
		 *
		 * If copying an element fails, the new row keeps the elements already copied.
		 *
		 * @param row elements to copy from an external buffer
		 * @warning The source must not refer to this CRS. Appending may reallocate its buffer.
		 */
		void append_row(std::span<const T> row) {
			append_row();
			for (const T& value: row) {
				push_back(value);
			}
		}

		/**
		 * @brief Appends an element directly to the last row
		 *
		 * The container must have at least one row.
		 *
		 * @param value element to append
		 */
		void push_back(const T& value) {
			YAGKP_ASSERT(row_count() > 0, "CRS push_back requires a row");
			values.push_back(value);
			++offsets.back();
		}

		void push_back(T&& value) {
			YAGKP_ASSERT(row_count() > 0, "CRS push_back requires a row");
			values.push_back(std::move(value));
			++offsets.back();
		}

		/**
		 * @brief Returns the number of rows, including empty rows
		 */
		int_t row_count() const noexcept {
			return offsets.size() - 1;
		}

		/**
		 * @brief Returns the total number of elements across all rows
		 */
		int_t value_count() const noexcept {
			return values.size();
		}

		/**
		 * @brief Returns a view of one row
		 *
		 * @param row index in [0, row_count())
		 * @return the elements of the row
		 */
		std::span<T> operator[](const int_t row) noexcept {
			YAGKP_ASSERT(row < row_count(), "CRS row index out of bounds");
			return flat_values().subspan(offsets[row], offsets[row + 1] - offsets[row]);
		}

		std::span<const T> operator[](const int_t row) const noexcept {
			YAGKP_ASSERT(row < row_count(), "CRS row index out of bounds");
			return flat_values().subspan(offsets[row], offsets[row + 1] - offsets[row]);
		}

		/**
		 * @brief Returns a view of all elements in row order
		 */
		std::span<T> flat_values() noexcept {
			return values;
		}

		std::span<const T> flat_values() const noexcept {
			return values;
		}

		/**
		 * @brief Returns a lazy range of row views, including empty rows
		 *
		 * The range walks row indices and gets each row through operator[].
		 */
		auto rows() noexcept {
			auto indices = std::views::iota(int_t{0}, row_count());
			return std::views::transform(indices, [this](int_t row) { return (*this)[row]; });
		}

		auto rows() const noexcept {
			auto indices = std::views::iota(int_t{0}, row_count());
			return std::views::transform(indices, [this](int_t row) { return (*this)[row]; });
		}

	private:
		void swap(CRS& other) noexcept {
			offsets.swap(other.offsets);
			values.swap(other.values);
		}

		vector<int_t> offsets{0};
		vector<T> values;
	};
} // namespace yagkp
