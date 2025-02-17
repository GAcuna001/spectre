// Distributed under the MIT License.
// See LICENSE.txt for details.

/// \file
/// Defines generic tensor indices used in `TensorExpression`s

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <type_traits>

#include "DataStructures/Tensor/IndexType.hpp"
#include "Utilities/ForceInline.hpp"
#include "Utilities/Requires.hpp"
#include "Utilities/TMPL.hpp"

namespace TensorExpressions {
namespace TensorIndex_detail {
// The below values are used to separate upper indices from lower indices and
// spatial indices from spacetime indices.
//
// Tensor expressions perform as many calculations as possible in a constexpr
// context, which means working with fundamental types, specifically integer
// types, is easiest. By using sentinel values defined in one location we can
// easily control the encoding without having magic values floating around in
// many places. Furthermore, encoding all the information in the `size_t` means
// that when a failure occurs in one of the constexpr calculations it is
// reasonably easy to debug because, while encoded, the full type information is
// present. This approach can effectively be thought of as using specific bits
// in the `size_t` to mark information, using the size_t more as a bitfield than
// anything else. For human readability, we use base-10 numbers instead of
// base-2 values that would truly set individual bits.
//
// Spacetime indices are represented by values [0, `spatial_sentinel`) and
// spatial indices are represented by values
// [`spatial_sentinel`, `max_sentinel`). Lower spacetime indices are represented
// by values [0, `upper_sentinel`), and upper spacetime indices are represented
// by values [`upper_sentinel`, `spatial_sentinel`). Lower spatial indices are
// represented by values
// [`spatial_sentinel`, `spatial_sentinel` + `upper_sentinel`), and upper
// spatial indices are represented by values
// [`spatial_sentinel` + `upper_sentinel`, `max_sentinel`). Values equal to or
// above `max_sentinel` are considered invalid for representing an index.
static constexpr size_t spatial_sentinel = 1000;
static constexpr size_t upper_sentinel = 500;
static constexpr size_t upper_spatial_sentinel =
    spatial_sentinel + upper_sentinel;
static constexpr size_t max_sentinel = 2000;
}  // namespace TensorIndex_detail
}  // namespace TensorExpressions

/*!
 * \ingroup TensorExpressionsGroup
 * \brief Represents the geeric indices in a TensorExpression
 *
 * \details
 * Used to denote a tensor index in a tensor slot. This allows the following
 * type of expressions to work:
 * \code{.cpp}
 * auto T = evaluate<ti_a, ti_b>(F(ti_a, ti_b) + S(ti_b, ti_a));
 * \endcode
 * where `decltype(ti_a) == TensorIndex<0>` and
 * `decltype(ti_b) == TensorIndex<1>`. That is, `ti_a` and `ti_b` are
 * placeholders for objects of type `TensorIndex<0>` and `TensorIndex<1>`,
 * respectively.
 */
template <std::size_t I,
          Requires<(I < TensorExpressions::TensorIndex_detail::max_sentinel)> =
              nullptr>
struct TensorIndex {
  using value_type = std::size_t;
  using type = TensorIndex<I>;
  static constexpr value_type value = I;
  static constexpr UpLo valence =
      ((I < TensorExpressions::TensorIndex_detail::upper_sentinel) or
       (I >= TensorExpressions::TensorIndex_detail::spatial_sentinel and
        I < TensorExpressions::TensorIndex_detail::upper_spatial_sentinel))
          ? UpLo::Lo
          : UpLo::Up;
  static constexpr bool is_spacetime =
      I < TensorExpressions::TensorIndex_detail::spatial_sentinel;
};

namespace TensorExpressions {
/*!
 * \ingroup TensorExpressionsGroup
 * \brief Returns the TensorIndex value of with opposite valence.
 *
 * \details The input value represents a TensorIndex value, which encodes
 * both the valence of the index and whether the index is spacetime or
 * spatial. This function returns the value that corresponds to the encoding of
 * the TensorIndex with the same index type, but opposite valence.
 *
 * For example, 0 is the TensorIndex value for `ti_a`. If `i == 0`, then 500
 * will be returned, which is the TensorIndex value for `ti_A`. If `i == 500`
 * (representing `ti_A`), then 0 (representing `ti_a`) is returned.
 *
 * @param i a TensorIndex value that represents a generic index
 * @return the TensorIndex value that encodes the generic index with the
 * opposite valence
 */
SPECTRE_ALWAYS_INLINE static constexpr size_t
get_tensorindex_value_with_opposite_valence(const size_t i) noexcept {
  assert(i < TensorIndex_detail::max_sentinel);  // NOLINT
  if ((i >= TensorIndex_detail::upper_sentinel and
       i < TensorIndex_detail::spatial_sentinel) or
      (i >= TensorIndex_detail::upper_spatial_sentinel)) {
    // `i` represents an upper index, so return the lower index's encoding
    return i - TensorIndex_detail::upper_sentinel;
  } else {
    // `i` represents a lower index, so return the upper index's encoding
    return i + TensorIndex_detail::upper_sentinel;
  }
}
}  //  namespace TensorExpressions

/// @{
/*!
 * \ingroup TensorExpressionsGroup
 * \brief The available TensorIndexs to use in a TensorExpression
 *
 * \details The suffix following `ti_` indicates index properties:
 * - Uppercase: contravariant/upper index
 * - Lowercase: covariant/lower index
 * - A/a - H/h: generic spacetime index
 * - I/i - M/m: generic spatial index
 * - T/t: concrete time index (defined as a spacetime `TensorIndex`)
 *
 * \snippet Test_AddSubtract.cpp use_tensor_index
 *
 * If you want to support a new generic index, definitions for the upper and
 * lower versions of the index must be added as unique `TensorIndex` types, e.g.
 * \code
 * static constexpr TensorIndex<UNIQUE_INTEGER_IN_PROPER_RANGE_LOWER> ti_x{};
 * static constexpr TensorIndex<UNIQUE_INTEGER_IN_PROPER_RANGE_UPPER> ti_X{};
 * \endcode
 * where `UNIQUE_INTEGER_IN_PROPER_RANGE_LOWER` and
 * `UNIQUE_INTEGER_IN_PROPER_RANGE_UPPER` are unique, but related integers that
 * fall in the integer ranges that properly encode the index's properties
 * according to the `_sentinel` values defined at the top of
 * `src/DataStructures/Tensor/Expressions/TensorIndex.hpp`. This enables the new
 * index to be distinguishable from others and for the upper and lower versions
 * to be recognized as related by opposite valence. See comments there on these
 * integer ranges to properly encode the new index (both upper and lower
 * definitions) that you wish to add. In short, you should simply be able to
 * continue the pattern used for the existing `TensorIndex` types that are
 * already defined. For example, if `ti_M`/`ti_m` is the highest-valued generic
 * spatial index currently defined and you want to add `ti_N`/`ti_n` as a new
 * generic spatial index, you can simply define `ti_N` and `ti_n`'s unique
 * integer values to be `INTEGER_VALUE_FOR_M + 1` and `INTEGER_VALUE_FOR_m + 1`,
 * respectively. For adding a new generic spacetime index, you should be able to
 * do the same thing with respect to the upper and lower versions of the
 * highest-valued currently defined generic spacetime `TensorIndex`.
 */
static constexpr TensorIndex<0> ti_a{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_sentinel>
    ti_A{};
static constexpr TensorIndex<1> ti_b{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_sentinel + 1>
    ti_B{};
static constexpr TensorIndex<2> ti_c{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_sentinel + 2>
    ti_C{};
static constexpr TensorIndex<3> ti_d{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_sentinel + 3>
    ti_D{};
static constexpr TensorIndex<4> ti_e{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_sentinel + 4>
    ti_E{};
static constexpr TensorIndex<5> ti_f{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_sentinel + 5>
    ti_F{};
static constexpr TensorIndex<6> ti_g{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_sentinel + 6>
    ti_G{};
static constexpr TensorIndex<7> ti_h{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_sentinel + 7>
    ti_H{};

static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_sentinel - 1>
    ti_t{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::spatial_sentinel - 1>
    ti_T{};

static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::spatial_sentinel>
    ti_i{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_spatial_sentinel>
    ti_I{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::spatial_sentinel + 1>
    ti_j{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_spatial_sentinel + 1>
    ti_J{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::spatial_sentinel + 2>
    ti_k{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_spatial_sentinel + 2>
    ti_K{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::spatial_sentinel + 3>
    ti_l{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_spatial_sentinel + 3>
    ti_L{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::spatial_sentinel + 4>
    ti_m{};
static constexpr TensorIndex<
    TensorExpressions::TensorIndex_detail::upper_spatial_sentinel + 4>
    ti_M{};
/// @}

namespace tt {
/*!
 * \ingroup TypeTraitsGroup TensorExpressionsGroup
 * \brief Check if a type `T` is a TensorIndex used in TensorExpressions
 */
template <typename T>
struct is_tensor_index : std::false_type {};
template <size_t I>
struct is_tensor_index<TensorIndex<I>> : std::true_type {};
}  // namespace tt

namespace TensorExpressions {
namespace detail {
template <auto&... TensorIndices>
struct make_tensorindex_list_impl {
  static_assert(
      (... and
       tt::is_tensor_index<std::decay_t<decltype(TensorIndices)>>::value),
      "Template parameters of make_tensorindex_list must be TensorIndex "
      "objects.");
  using type = tmpl::list<std::decay_t<decltype(TensorIndices)>...>;
};

template <typename TensorIndexList>
struct tensorindex_list_is_valid_impl;

template <typename... TensorIndices>
struct tensorindex_list_is_valid_impl<tmpl::list<TensorIndices...>> {
  static_assert(
      (... and tt::is_tensor_index<TensorIndices>::value),
      "Template parameters of tensorindex_list_is_valid must be TensorIndex "
      "types.");
  static constexpr bool value = tmpl::is_set<TensorIndices...>::value;
};

template <typename TensorIndexList>
struct remove_time_indices;
}  // namespace detail

/*!
 * \ingroup TensorExpressionsGroup
 * \brief Determine whether or not a given list of TensorIndexs is valid
 * to be used with a tensor
 *
 * \details A list of TensorIndexs is considered valid if the subset of generic
 * indices are a set. Indices with opposite valences are unique, e.g. one
 * instance each of `ti_a` and `ti_A` is valid. An arbitrary number of concrete
 * time indices, regardless of valence, is also valid.
 *
 * @tparam TensorIndexList list of generic index types, e.g. the types of
 * `ti_a, ti_b`
 */
template <typename TensorIndexList>
struct tensorindex_list_is_valid;

template <typename... TensorIndices>
struct tensorindex_list_is_valid<tmpl::list<TensorIndices...>> {
  static constexpr bool value = detail::tensorindex_list_is_valid_impl<
      typename detail::remove_time_indices<
          tmpl::list<TensorIndices...>>::type>::value;
};
}  // namespace TensorExpressions

/*!
 * \ingroup TensorExpressionsGroup
 * \brief Creates a TensorIndex type list from a list of TensorIndex objects
 *
 * @tparam TensorIndices list of generic index objects, e.g. `ti_a, ti_b`
 */
template <auto&... TensorIndices>
using make_tensorindex_list =
    typename TensorExpressions::detail::make_tensorindex_list_impl<
        TensorIndices...>::type;
