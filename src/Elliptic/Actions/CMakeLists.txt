# Distributed under the MIT License.
# See LICENSE.txt for details.

spectre_target_headers(
  ${LIBRARY}
  INCLUDE_DIRECTORY ${CMAKE_SOURCE_DIR}/src
  HEADERS
  InitializeAnalyticSolution.hpp
  InitializeBackgroundFields.hpp
  InitializeFields.hpp
  InitializeFixedSources.hpp
  RandomizeInitialGuess.hpp
  )
