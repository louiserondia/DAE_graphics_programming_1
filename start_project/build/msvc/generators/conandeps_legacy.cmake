message(STATUS "Conan: Using CMakeDeps conandeps_legacy.cmake aggregator via include()")
message(STATUS "Conan: It is recommended to use explicit find_package() per dependency instead")

find_package(spdlog)
find_package(assimp)
find_package(stb)
find_package(SDL3)
find_package(glad)

set(CONANDEPS_LEGACY  spdlog::spdlog  assimp::assimp  stb::stb  sdl::sdl  glad::glad )