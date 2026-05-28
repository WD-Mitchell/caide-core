cmake_minimum_required(VERSION 3.24)
project(caide_core VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CAIDE_BUILD_TESTS "Build unit tests" ON)
option(CAIDE_BUILD_SHARED "Build shared library" ON)
option(CAIDE_ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(CAIDE_BUILD_FUZZ "Build libFuzzer harnesses (requires Clang + libFuzzer)" OFF)
option(CAIDE_FORCE_STUB "Skip OpenCASCADE detection and build in stub mode" OFF)

include(cmake/CompilerWarnings.cmake)
include(cmake/Platform.cmake)

if(CAIDE_FORCE_STUB)
    message(STATUS "CAIDE_FORCE_STUB=ON — building in stub mode (OCCT skipped)")
    set(CAIDE_STUB_MODE ON)
else()
    find_package(OpenCASCADE QUIET)
    if(NOT OpenCASCADE_FOUND)
        message(WARNING "OpenCASCADE not found — building in stub mode")
        set(CAIDE_STUB_MODE ON)
    endif()
endif()

add_subdirectory(src)

if(CAIDE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(CAIDE_BUILD_FUZZ)
    add_subdirectory(tests/fuzz)
endif()

include(GNUInstallDirs)
install(DIRECTORY include/caide_core DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS caide_core
    EXPORT CaideCoreTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/CaideCoreConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    cmake/CaideCoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CaideCoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CaideCore
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CaideCoreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/CaideCoreConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CaideCore
)

install(EXPORT CaideCoreTargets
    NAMESPACE Caide::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CaideCore
)
