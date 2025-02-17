// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Evolution/Systems/RadiationTransport/M1Grey/Sources.hpp"

#include <array>
#include <cstddef>

#include "DataStructures/DataBox/Tag.hpp"
#include "DataStructures/DataVector.hpp"
#include "DataStructures/Tensor/Tensor.hpp"
#include "DataStructures/Variables.hpp"
#include "PointwiseFunctions/GeneralRelativity/IndexManipulation.hpp"
#include "PointwiseFunctions/GeneralRelativity/Tags.hpp"  // IWYU pragma: keep

// IWYU pragma: no_forward_declare Tensor

namespace {
struct AlphaTildeP : db::SimpleTag {
  using type = tnsr::II<DataVector, 3>;
};
}  //  namespace

namespace RadiationTransport::M1Grey::detail {
void compute_sources_impl(
    const gsl::not_null<Scalar<DataVector>*> source_tilde_e,
    const gsl::not_null<tnsr::i<DataVector, 3>*> source_tilde_s,
    const Scalar<DataVector>& tilde_e, const tnsr::i<DataVector, 3>& tilde_s,
    const tnsr::II<DataVector, 3>& tilde_p, const Scalar<DataVector>& source_n,
    const tnsr::i<DataVector, 3>& source_i, const Scalar<DataVector>& lapse,
    const tnsr::i<DataVector, 3>& d_lapse,
    const tnsr::iJ<DataVector, 3>& d_shift,
    const tnsr::ijj<DataVector, 3>& d_spatial_metric,
    const tnsr::II<DataVector, 3>& inv_spatial_metric,
    const tnsr::ii<DataVector, 3>& extrinsic_curvature) noexcept {
  Variables<tmpl::list<Tags::TildeSVector<Frame::Inertial>, AlphaTildeP>>
      temp_tensors(get(tilde_e).size());

  constexpr size_t spatial_dim = 3;

  auto& tilde_s_M = get<Tags::TildeSVector<Frame::Inertial>>(temp_tensors);
  raise_or_lower_index(make_not_null(&tilde_s_M), tilde_s, inv_spatial_metric);
  auto& alpha_tilde_p = get<AlphaTildeP>(temp_tensors);
  for (size_t i = 0; i < spatial_dim; ++i) {
    for (size_t j = 0; j < spatial_dim; ++j) {
      alpha_tilde_p.get(i, j) = tilde_p.get(i, j) * get(lapse);
    }
  }

  // unroll contributions from m=0 and n=0 to avoid initializing
  // source terms to zero
  get(*source_tilde_e) =
      get<0, 0>(extrinsic_curvature) * get<0, 0>(alpha_tilde_p) -
      get<0>(tilde_s_M) * get<0>(d_lapse) + get(source_n);
  for (size_t m = 1; m < spatial_dim; ++m) {
    get(*source_tilde_e) +=
        extrinsic_curvature.get(0, m) * alpha_tilde_p.get(0, m) +
        extrinsic_curvature.get(m, 0) * alpha_tilde_p.get(m, 0) -
        tilde_s_M.get(m) * d_lapse.get(m);
    for (size_t n = 1; n < spatial_dim; ++n) {
      get(*source_tilde_e) +=
          extrinsic_curvature.get(m, n) * alpha_tilde_p.get(m, n);
    }
  }

  for (size_t i = 0; i < spatial_dim; ++i) {
    source_tilde_s->get(i) =
        -get(tilde_e) * d_lapse.get(i) + get<0>(tilde_s) * d_shift.get(i, 0) +
        0.5 * get<0, 0>(alpha_tilde_p) * d_spatial_metric.get(i, 0, 0) +
        source_i.get(i);
    for (size_t m = 1; m < spatial_dim; ++m) {
      source_tilde_s->get(i) +=
          tilde_s.get(m) * d_shift.get(i, m) +
          0.5 * (alpha_tilde_p.get(0, m) * d_spatial_metric.get(i, 0, m) +
                 alpha_tilde_p.get(m, 0) * d_spatial_metric.get(i, m, 0));
      for (size_t n = 1; n < spatial_dim; ++n) {
        source_tilde_s->get(i) +=
            0.5 * alpha_tilde_p.get(m, n) * d_spatial_metric.get(i, m, n);
      }
    }
  }
}

}  // namespace RadiationTransport::M1Grey::detail
