cmake_minimum_required(VERSION 3.10)

project(small_mem VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_library(small_mem STATIC
    src/smem.c
)

target_include_directories(small_mem PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS small_mem
    EXPORT small_memTargets
    DESTINATION lib
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/inc DESTINATION include/small_mem)

install(EXPORT small_memTargets
    FILE small_mem-config.cmake
    NAMESPACE small_mem::
    DESTINATION lib/cmake/small_mem
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    small_mem-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/small_mem-config-version.cmake
    DESTINATION lib/cmake/small_mem
)
