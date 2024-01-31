#---------------------------------*-CMake-*----------------------------------#
# Copyright 2020-2024 UT-Battelle, LLC, and other Celeritas developers.
# See the top-level COPYRIGHT file for details.
# SPDX-License-Identifier: (Apache-2.0 OR MIT)
#----------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.14...3.28)
project(G4VG VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  celeritas
  # Current tip of celeritas develop - adjust as needed
  URL https://github.com/celeritas-project/celeritas/archive/5742b0c924a67bf02ec0ce9fc85719a70f7857e0.zip
)

# Set/force any Celeritas CMake args here before making it available, e.g.
set(CELERITAS_USE_MPI OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(celeritas)

# Use any celeritas targets as normal (or CMake modules)
