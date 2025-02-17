// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include <benchmark/benchmark.h>
#pragma GCC diagnostic pop
#include <string>
#include <vector>

#include "DataStructures/DataBox/Tag.hpp"
#include "DataStructures/DataVector.hpp"
#include "DataStructures/Tensor/Tensor.hpp"
#include "DataStructures/Variables.hpp"
#include "Domain/CoordinateMaps/Affine.hpp"
#include "Domain/CoordinateMaps/CoordinateMap.hpp"
#include "Domain/CoordinateMaps/CoordinateMap.tpp"
#include "Domain/CoordinateMaps/ProductMaps.hpp"
#include "Domain/CoordinateMaps/ProductMaps.tpp"
#include "Domain/LogicalCoordinates.hpp"
#include "Domain/Structure/Element.hpp"
#include "NumericalAlgorithms/LinearOperators/PartialDerivatives.tpp"
#include "NumericalAlgorithms/Spectral/Mesh.hpp"
#include "NumericalAlgorithms/Spectral/Spectral.hpp"
#include "PointwiseFunctions/MathFunctions/PowX.hpp"

// Charm looks for this function but since we build without a main function or
// main module we just have it be empty
extern "C" void CkRegisterMainModule(void) {}

// This file is an example of how to do microbenchmark with Google Benchmark
// https://github.com/google/benchmark
// For two examples in different anonymous namespaces

namespace {
// Benchmark of push_back() in std::vector, following Chandler Carruth's talk
// at CppCon in 2015,
// https://www.youtube.com/watch?v=nXaxk27zwlk

// void bench_create(benchmark::State &state) {
//  while (state.KeepRunning()) {
//    std::vector<int> v;
//    benchmark::DoNotOptimize(&v);
//    static_cast<void>(v);
//  }
// }
// BENCHMARK(bench_create);

// void bench_reserve(benchmark::State &state) {
//  while (state.KeepRunning()) {
//    std::vector<int> v;
//    v.reserve(1);
//    benchmark::DoNotOptimize(v.data());
//  }
// }
// BENCHMARK(bench_reserve);

// void bench_push_back(benchmark::State &state) {
//  while (state.KeepRunning()) {
//    std::vector<int> v;
//    v.reserve(1);
//    benchmark::DoNotOptimize(v.data());
//    v.push_back(42);
//    benchmark::ClobberMemory();
//  }
// }
// BENCHMARK(bench_push_back);
}  // namespace

namespace {
// In this anonymous namespace is an example of microbenchmarking the
// all_gradient routine for the GH system

template <size_t Dim>
struct Kappa : db::SimpleTag {
  using type = tnsr::abb<DataVector, Dim, Frame::Grid>;
};
template <size_t Dim>
struct Psi : db::SimpleTag {
  using type = tnsr::aa<DataVector, Dim, Frame::Grid>;
};

// clang-tidy: don't pass be non-const reference
void bench_all_gradient(benchmark::State& state) {  // NOLINT
  constexpr const size_t pts_1d = 4;
  constexpr const size_t Dim = 3;
  const Mesh<Dim> mesh{pts_1d, Spectral::Basis::Legendre,
                       Spectral::Quadrature::GaussLobatto};
  domain::CoordinateMaps::Affine map1d(-1.0, 1.0, -1.0, 1.0);
  using Map3d =
      domain::CoordinateMaps::ProductOf3Maps<domain::CoordinateMaps::Affine,
                                             domain::CoordinateMaps::Affine,
                                             domain::CoordinateMaps::Affine>;
  domain::CoordinateMap<Frame::ElementLogical, Frame::Grid, Map3d> map(
      Map3d{map1d, map1d, map1d});

  using VarTags = tmpl::list<Kappa<Dim>, Psi<Dim>>;
  const InverseJacobian<DataVector, Dim, Frame::ElementLogical, Frame::Grid>
      inv_jac = map.inv_jacobian(logical_coordinates(mesh));
  const auto grid_coords = map(logical_coordinates(mesh));
  Variables<VarTags> vars(mesh.number_of_grid_points(), 0.0);

  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(partial_derivatives<VarTags>(vars, mesh, inv_jac));
  }
}
BENCHMARK(bench_all_gradient);  // NOLINT
}  // namespace

// Ignore the warning about an extra ';' because some versions of benchmark
// require it
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
BENCHMARK_MAIN();
#pragma GCC diagnostic pop
