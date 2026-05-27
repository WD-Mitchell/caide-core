set(CAIDE_CORE_SOURCES
    error.cpp
    geometry/primitives.cpp
    geometry/booleans.cpp
    geometry/transforms.cpp
    geometry/features.cpp
    geometry/sketch.cpp
    geometry/advanced.cpp
    commands/executor.cpp
    commands/history.cpp
    export/mesh_export.cpp
    export/tessellation.cpp
    printability/analyzer.cpp
    printability/support_estimator.cpp
    references/naming.cpp
    abi/abi_geometry.cpp
    abi/abi_commands.cpp
    abi/abi_export.cpp
    abi/abi_printability.cpp
    abi/abi_context.cpp
)

if(CAIDE_BUILD_SHARED)
    add_library(caide_core SHARED ${CAIDE_CORE_SOURCES})
else()
    add_library(caide_core STATIC ${CAIDE_CORE_SOURCES})
endif()

add_library(Caide::caide_core ALIAS caide_core)

caide_apply_warnings(caide_core)

target_include_directories(caide_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32 AND CAIDE_BUILD_SHARED)
    target_compile_definitions(caide_core PRIVATE CAIDE_CORE_EXPORTS)
endif()

if(CAIDE_STUB_MODE)
    target_compile_definitions(caide_core PUBLIC CAIDE_STUB_MODE=1)
endif()

if(OpenCASCADE_FOUND)
    if(TARGET OpenCASCADE::OpenCASCADE)
        target_link_libraries(caide_core PUBLIC OpenCASCADE::OpenCASCADE)
    elseif(DEFINED OpenCASCADE_LIBRARIES)
        target_link_libraries(caide_core PUBLIC ${OpenCASCADE_LIBRARIES})
    endif()
    if(DEFINED OpenCASCADE_INCLUDE_DIR)
        target_include_directories(caide_core PRIVATE ${OpenCASCADE_INCLUDE_DIR})
    elseif(DEFINED OpenCASCADE_INCLUDE_DIRS)
        target_include_directories(caide_core PRIVATE ${OpenCASCADE_INCLUDE_DIRS})
    endif()
endif()
