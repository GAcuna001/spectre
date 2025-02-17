// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <array>
#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

#include "DataStructures/SliceVariables.hpp"
#include "DataStructures/Tensor/EagerMath/Magnitude.hpp"
#include "DataStructures/Tensor/Tensor.hpp"
#include "DataStructures/Variables.hpp"
#include "DataStructures/VariablesTag.hpp"
#include "Domain/Domain.hpp"
#include "Domain/ElementMap.hpp"
#include "Domain/FaceNormal.hpp"
#include "Domain/Structure/Direction.hpp"
#include "Domain/Structure/DirectionMap.hpp"
#include "Domain/Structure/Element.hpp"
#include "Domain/Structure/IndexToSliceAt.hpp"
#include "Domain/Tags.hpp"
#include "Domain/Tags/FaceNormal.hpp"
#include "Domain/Tags/Faces.hpp"
#include "Elliptic/DiscontinuousGalerkin/Tags.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/MortarHelpers.hpp"
#include "NumericalAlgorithms/DiscontinuousGalerkin/Tags.hpp"
#include "NumericalAlgorithms/LinearOperators/PartialDerivatives.hpp"
#include "NumericalAlgorithms/Spectral/Mesh.hpp"
#include "NumericalAlgorithms/Spectral/Spectral.hpp"
#include "Utilities/ErrorHandling/Assert.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/TMPL.hpp"
#include "Utilities/TaggedTuple.hpp"

namespace elliptic::dg {

/// Initialize the background-independent geometry for the elliptic DG operator
template <size_t Dim>
struct InitializeGeometry {
  using return_tags = tmpl::list<
      domain::Tags::Mesh<Dim>, domain::Tags::Element<Dim>,
      domain::Tags::ElementMap<Dim>,
      domain::Tags::Coordinates<Dim, Frame::ElementLogical>,
      domain::Tags::Coordinates<Dim, Frame::Inertial>,
      domain::Tags::InverseJacobian<Dim, Frame::ElementLogical,
                                    Frame::Inertial>,
      domain::Tags::DetInvJacobian<Frame::ElementLogical, Frame::Inertial>>;
  using argument_tags = tmpl::list<domain::Tags::InitialExtents<Dim>,
                                   domain::Tags::InitialRefinementLevels<Dim>,
                                   domain::Tags::Domain<Dim>>;
  void operator()(
      gsl::not_null<Mesh<Dim>*> mesh, gsl::not_null<Element<Dim>*> element,
      gsl::not_null<ElementMap<Dim, Frame::Inertial>*> element_map,
      gsl::not_null<tnsr::I<DataVector, Dim, Frame::ElementLogical>*>
          logical_coords,
      gsl::not_null<tnsr::I<DataVector, Dim, Frame::Inertial>*> inertial_coords,
      gsl::not_null<InverseJacobian<DataVector, Dim, Frame::ElementLogical,
                                    Frame::Inertial>*>
          inv_jacobian,
      gsl::not_null<Scalar<DataVector>*> det_inv_jacobian,
      const std::vector<std::array<size_t, Dim>>& initial_extents,
      const std::vector<std::array<size_t, Dim>>& initial_refinement,
      const Domain<Dim>& domain,
      const ElementId<Dim>& element_id) const noexcept;
};

/// Initialize the geometry on faces and mortars for the elliptic DG operator
///
/// Face normals are not yet normalized, and the face-normal magnitudes are not
/// yet computed at all. Invoke `elliptic::dg::NormalizeFaceNormals` for this
/// purpose, possibly preceded by `elliptic::dg::InitializeBackground` if the
/// system has a background metric.
///
/// The `::Tags::deriv<domain::Tags::UnnormalizedFaceNormal<Dim>>` is only
/// added on external boundaries, for use by boundary conditions.
template <size_t Dim>
struct InitializeFacesAndMortars {
  using return_tags = tmpl::append<
      domain::make_faces_tags<
          Dim,
          tmpl::list<domain::Tags::Direction<Dim>,
                     domain::Tags::Coordinates<Dim, Frame::Inertial>,
                     domain::Tags::FaceNormal<Dim>,
                     domain::Tags::UnnormalizedFaceNormalMagnitude<Dim>,
                     // Possible optimization: The derivative of the face normal
                     // could be omitted for some systems, but its memory usage
                     // is probably insignificant since it's only added on
                     // external boundaries.
                     ::Tags::deriv<domain::Tags::UnnormalizedFaceNormal<Dim>,
                                   tmpl::size_t<Dim>, Frame::Inertial>>>,
      tmpl::list<::Tags::Mortars<domain::Tags::Mesh<Dim - 1>, Dim>,
                 ::Tags::Mortars<::Tags::MortarSize<Dim - 1>, Dim>>>;
  using argument_tags =
      tmpl::list<domain::Tags::Mesh<Dim>, domain::Tags::Element<Dim>,
                 domain::Tags::ElementMap<Dim>,
                 domain::Tags::InverseJacobian<Dim, Frame::ElementLogical,
                                               Frame::Inertial>>;
  void operator()(
      gsl::not_null<DirectionMap<Dim, Direction<Dim>>*> face_directions,
      gsl::not_null<DirectionMap<Dim, tnsr::I<DataVector, Dim>>*>
          face_inertial_coords,
      gsl::not_null<DirectionMap<Dim, tnsr::i<DataVector, Dim>>*> face_normals,
      gsl::not_null<DirectionMap<Dim, Scalar<DataVector>>*>
          face_normal_magnitudes,
      gsl::not_null<DirectionMap<Dim, tnsr::ij<DataVector, Dim>>*>
          deriv_unnormalized_face_normals,
      gsl::not_null<::dg::MortarMap<Dim, Mesh<Dim - 1>>*> mortar_meshes,
      gsl::not_null<::dg::MortarMap<Dim, ::dg::MortarSize<Dim - 1>>*>
          mortar_sizes,
      const Mesh<Dim>& mesh, const Element<Dim>& element,
      const ElementMap<Dim, Frame::Inertial>& element_map,
      const InverseJacobian<DataVector, Dim, Frame::ElementLogical,
                            Frame::Inertial>& inv_jacobian,
      const std::vector<std::array<size_t, Dim>>& initial_extents)
      const noexcept;
};

/// Initialize background quantities for the elliptic DG operator, possibly
/// including the metric necessary for normalizing face normals
template <size_t Dim, typename BackgroundFields>
struct InitializeBackground {
  using return_tags =
      tmpl::list<::Tags::Variables<BackgroundFields>,
                 domain::Tags::Faces<Dim, ::Tags::Variables<BackgroundFields>>>;
  using argument_tags =
      tmpl::list<domain::Tags::Coordinates<Dim, Frame::Inertial>,
                 domain::Tags::Mesh<Dim>,
                 domain::Tags::InverseJacobian<Dim, Frame::ElementLogical,
                                               Frame::Inertial>>;

  template <typename Background>
  void operator()(
      const gsl::not_null<Variables<BackgroundFields>*> background_fields,
      const gsl::not_null<DirectionMap<Dim, Variables<BackgroundFields>>*>
          face_background_fields,
      const tnsr::I<DataVector, Dim>& inertial_coords, const Mesh<Dim>& mesh,
      const InverseJacobian<DataVector, Dim, Frame::ElementLogical,
                            Frame::Inertial>& inv_jacobian,
      const Background& background) const noexcept {
    *background_fields = variables_from_tagged_tuple(background.variables(
        inertial_coords, mesh, inv_jacobian, BackgroundFields{}));
    ASSERT(mesh.quadrature(0) == Spectral::Quadrature::GaussLobatto,
           "Only Gauss-Lobatto quadrature is currently implemented for "
           "slicing background fields to faces.");
    for (const auto& direction : Direction<Dim>::all_directions()) {
      // Possible optimization: Only the background fields in the
      // System::fluxes_computer::argument_tags are needed on internal faces.
      data_on_slice(
          make_not_null(&(*face_background_fields)[direction]),
          *background_fields, mesh.extents(), direction.dimension(),
          index_to_slice_at(mesh.extents(), direction));
    }
  }
};

/// Normalize face normals, possibly using a background metric. Set
/// `InvMetricTag` to `void` to normalize face normals with the Euclidean
/// magnitude.
template <size_t Dim, typename InvMetricTag>
struct NormalizeFaceNormal {
  using return_tags =
      tmpl::list<domain::Tags::FaceNormal<Dim>,
                 domain::Tags::UnnormalizedFaceNormalMagnitude<Dim>>;
  using argument_tags =
      tmpl::conditional_t<std::is_same_v<InvMetricTag, void>, tmpl::list<>,
                          tmpl::list<InvMetricTag>>;

  template <typename... InvMetric>
  void operator()(
      const gsl::not_null<tnsr::i<DataVector, Dim>*> face_normal,
      const gsl::not_null<Scalar<DataVector>*> face_normal_magnitude,
      const InvMetric&... inv_metric) const noexcept {
    magnitude(face_normal_magnitude, *face_normal, inv_metric...);
    for (size_t d = 0; d < Dim; ++d) {
      face_normal->get(d) /= get(*face_normal_magnitude);
    }
  }
};

}  // namespace elliptic::dg
