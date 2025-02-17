// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Domain/LogicalCoordinates.hpp"

#include <array>

#include "DataStructures/DataVector.hpp"
#include "DataStructures/IndexIterator.hpp"
#include "DataStructures/Tensor/Tensor.hpp"  // IWYU pragma: keep
#include "Domain/Structure/Direction.hpp"    // IWYU pragma: keep
#include "Domain/Structure/Side.hpp"
#include "NumericalAlgorithms/Spectral/Mesh.hpp"  // IWYU pragma: keep
#include "NumericalAlgorithms/Spectral/Spectral.hpp"
#include "Utilities/ContainerHelpers.hpp"
#include "Utilities/Gsl.hpp"

template <size_t VolumeDim>
void logical_coordinates(
    const gsl::not_null<tnsr::I<DataVector, VolumeDim, Frame::ElementLogical>*>
        logical_coords,
    const Mesh<VolumeDim>& mesh) noexcept {
  destructive_resize_components(logical_coords, mesh.number_of_grid_points());
  for (size_t d = 0; d < VolumeDim; ++d) {
    const auto& collocation_points_in_this_dim =
        Spectral::collocation_points(mesh.slice_through(d));
    for (IndexIterator<VolumeDim> index(mesh.extents()); index; ++index) {
      logical_coords->get(d)[index.collapsed_index()] =
          collocation_points_in_this_dim[index()[d]];
    }
  }
}

template <size_t VolumeDim>
tnsr::I<DataVector, VolumeDim, Frame::ElementLogical> logical_coordinates(
    const Mesh<VolumeDim>& mesh) noexcept {
  tnsr::I<DataVector, VolumeDim, Frame::ElementLogical> result{};
  logical_coordinates(make_not_null(&result), mesh);
  return result;
}

template <size_t VolumeDim>
tnsr::I<DataVector, VolumeDim, Frame::ElementLogical>
interface_logical_coordinates(const Mesh<VolumeDim - 1>& mesh,
                              const Direction<VolumeDim>& direction) noexcept {
  const auto num_grid_points = mesh.number_of_grid_points();
  const size_t sliced_away_dim = direction.dimension();
  tnsr::I<DataVector, VolumeDim, Frame::ElementLogical> logical_x(
      num_grid_points);

  std::array<DataVector, VolumeDim - 1> collocation_points_in_each_dim{};
  for (size_t d = 0; d < VolumeDim - 1; ++d) {
    collocation_points_in_each_dim.at(d) =
        Spectral::collocation_points(mesh.slice_through(d));
  }

  for (IndexIterator<VolumeDim - 1> index(mesh.extents()); index; ++index) {
    for (size_t d = 0; d < sliced_away_dim; ++d) {
      logical_x.get(d)[index.collapsed_index()] =
          collocation_points_in_each_dim.at(d)[index()[d]];
    }
    logical_x[sliced_away_dim] =
        (Side::Lower == direction.side() ? DataVector(num_grid_points, -1.0)
                                         : DataVector(num_grid_points, 1.0));
    for (size_t d = sliced_away_dim; d < VolumeDim - 1; ++d) {
      logical_x.get(d + 1)[index.collapsed_index()] =
          collocation_points_in_each_dim.at(d)[index()[d]];
    }
  }
  return logical_x;
}

// We need this specialisation since Mesh<Dim>::slice_through() is available
// only for Dim > 0
template <>
tnsr::I<DataVector, 1, Frame::ElementLogical> interface_logical_coordinates<1>(
    const Mesh<0>& mesh, const Direction<1>& direction) noexcept {
  return tnsr::I<DataVector, 1, Frame::ElementLogical>{
      mesh.number_of_grid_points(), direction.sign()};
}

// Explicit instantiations
template tnsr::I<DataVector, 1, Frame::ElementLogical> logical_coordinates(
    const Mesh<1>&) noexcept;
template tnsr::I<DataVector, 2, Frame::ElementLogical> logical_coordinates(
    const Mesh<2>&) noexcept;
template tnsr::I<DataVector, 3, Frame::ElementLogical> logical_coordinates(
    const Mesh<3>&) noexcept;
template void logical_coordinates(
    const gsl::not_null<tnsr::I<DataVector, 1, Frame::ElementLogical>*>,
    const Mesh<1>&) noexcept;
template void logical_coordinates(
    const gsl::not_null<tnsr::I<DataVector, 2, Frame::ElementLogical>*>,
    const Mesh<2>&) noexcept;
template void logical_coordinates(
    const gsl::not_null<tnsr::I<DataVector, 3, Frame::ElementLogical>*>,
    const Mesh<3>&) noexcept;

template tnsr::I<DataVector, 2, Frame::ElementLogical>
interface_logical_coordinates(const Mesh<1>&, const Direction<2>&) noexcept;
template tnsr::I<DataVector, 3, Frame::ElementLogical>
interface_logical_coordinates(const Mesh<2>&, const Direction<3>&) noexcept;
