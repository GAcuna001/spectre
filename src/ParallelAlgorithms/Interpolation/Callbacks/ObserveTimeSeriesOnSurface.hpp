// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "DataStructures/DataBox/TagName.hpp"
#include "IO/Observer/Helpers.hpp"
#include "IO/Observer/ObservationId.hpp"
#include "Parallel/GlobalCache.hpp"
#include "Parallel/Info.hpp"
#include "Parallel/Invoke.hpp"
#include "Parallel/Reduction.hpp"
#include "ParallelAlgorithms/Interpolation/InterpolationTargetDetail.hpp"
#include "Utilities/Functional.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/PrettyType.hpp"
#include "Utilities/TMPL.hpp"

/// \cond
namespace db {
template <typename TagsList>
class DataBox;
}  // namespace db
namespace observers {
namespace ThreadedActions {
struct WriteReductionData;
}  // namespace ThreadedActions
template <class Metavariables>
struct ObserverWriter;
}  // namespace observers
/// \endcond

namespace intrp {
namespace callbacks {

namespace detail {

template <typename T>
struct reduction_data_type;

template <typename... Ts>
struct reduction_data_type<tmpl::list<Ts...>> {
  // We use ReductionData because that is what is expected by the
  // ObserverWriter.  We do a "reduction" that involves only one
  // processing element (often equivalent to a core),
  // so AssertEqual is used here as a no-op.

  // The first argument is for Time, the others are for
  // the list of things being observed.
  using type = Parallel::ReductionData<
      Parallel::ReductionDatum<double, funcl::AssertEqual<>>,
      Parallel::ReductionDatum<typename Ts::type, funcl::AssertEqual<>>...>;
};

template <typename... Ts>
auto make_legend(tmpl::list<Ts...> /* meta */) {
  return std::vector<std::string>{"Time", db::tag_name<Ts>()...};
}

template <typename DbTags, typename... Ts>
auto make_reduction_data(const db::DataBox<DbTags>& box, double time,
                         tmpl::list<Ts...> /* meta */) {
  using reduction_data = typename reduction_data_type<tmpl::list<Ts...>>::type;
  return reduction_data(time, get<Ts>(box)...);
}

}  // namespace detail

/// \brief post_interpolation_callback that outputs
/// a time series on a surface.
///
/// Uses:
/// - Metavariables
///   - `temporal_id`
/// - DataBox:
///   - `TagsToObserve`
///
/// `ObservationType` is a type that distinguishes this observation
/// from other things that call observers::ThreadedActions::ObserverWriter,
/// so that different observations do not collide.
///
/// This is an InterpolationTargetTag::post_interpolation_callback;
/// see InterpolationTarget for a description of InterpolationTargetTag.
template <typename TagsToObserve, typename ObservationType,
          typename InterpolationTargetTag>
struct ObserveTimeSeriesOnSurface {
  using observed_reduction_data_tags = observers::make_reduction_data_tags<
      tmpl::list<typename detail::reduction_data_type<TagsToObserve>::type>>;
  using observation_types = tmpl::list<ObservationType>;

  static constexpr double fill_invalid_points_with =
      std::numeric_limits<double>::quiet_NaN();

  template <typename DbTags, typename Metavariables>
  static void apply(const db::DataBox<DbTags>& box,
                    Parallel::GlobalCache<Metavariables>& cache,
                    const typename InterpolationTargetTag::temporal_id::type&
                        temporal_id) noexcept {
    auto& proxy = Parallel::get_parallel_component<
        observers::ObserverWriter<Metavariables>>(cache);

    // We call this on proxy[0] because the 0th element of a NodeGroup is
    // always guaranteed to be present.
    using ParallelComponent =
        intrp::InterpolationTarget<Metavariables, InterpolationTargetTag>;
    auto& my_proxy = Parallel::get_parallel_component<ParallelComponent>(cache);
    Parallel::threaded_action<observers::ThreadedActions::WriteReductionData>(
        proxy[0],
        observers::ObservationId(
            InterpolationTarget_detail::get_temporal_id_value(temporal_id),
            pretty_type::get_name<ObservationType>()),
        static_cast<size_t>(Parallel::my_node(*my_proxy.ckLocal())),
        std::string{"/" + pretty_type::short_name<InterpolationTargetTag>()},
        detail::make_legend(TagsToObserve{}),
        detail::make_reduction_data(
            box, InterpolationTarget_detail::get_temporal_id_value(temporal_id),
            TagsToObserve{}));
  }
};
}  // namespace callbacks
}  // namespace intrp
