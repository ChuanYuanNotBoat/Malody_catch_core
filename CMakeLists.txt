cmake_minimum_required(VERSION 3.16)
project(MalodyCatchCore VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)

option(MCE_BUILD_QT_TRANSITIONAL "Build Qt-based transitional core targets." ON)

if(MCE_BUILD_QT_TRANSITIONAL)
    set(CMAKE_AUTOMOC ON)
    # Transitional build: the extracted core still uses QtCore/QtGui types.
    find_package(Qt6 REQUIRED COMPONENTS Core Gui)
endif()

add_library(malody_catch_core_pure STATIC
    src/core/editor_session.cpp
    src/core/model.cpp
)

target_include_directories(malody_catch_core_pure PUBLIC include)
target_compile_features(malody_catch_core_pure PUBLIC cxx_std_17)

add_library(malody_catch_core_ffi SHARED
    src/core/ffi.cpp
)

target_link_libraries(malody_catch_core_ffi PRIVATE malody_catch_core_pure)
target_include_directories(malody_catch_core_ffi PUBLIC include)
target_compile_definitions(malody_catch_core_ffi PRIVATE MCE_BUILD_SHARED)

if(MCE_BUILD_QT_TRANSITIONAL)
    add_library(MalodyCatchCore STATIC
        src/model/Note.cpp
        src/model/BpmEntry.cpp
        src/model/MetaData.cpp
        src/model/Chart.cpp
        src/utils/MathUtils.cpp
        src/utils/Logger.cpp
        src/utils/PerformanceTimer.cpp
        src/utils/DiagnosticCollector.cpp
        src/file/ChartIO.cpp
        src/file/ProjectIO.cpp
        src/controller/ChartController.cpp
    )

    target_include_directories(MalodyCatchCore PUBLIC src)
    target_link_libraries(MalodyCatchCore PUBLIC Qt6::Core Qt6::Gui)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(malody_catch_core_model_tests
        tests/core_model_tests.cpp
    )
    target_link_libraries(malody_catch_core_model_tests PRIVATE malody_catch_core_pure)

    add_executable(malody_catch_core_editor_session_tests
        tests/editor_session_tests.cpp
    )
    target_link_libraries(malody_catch_core_editor_session_tests PRIVATE malody_catch_core_pure)

    add_executable(malody_catch_core_ffi_tests
        tests/ffi_tests.cpp
    )
    target_link_libraries(malody_catch_core_ffi_tests PRIVATE malody_catch_core_ffi)

    add_test(NAME pure_core_model_tests COMMAND malody_catch_core_model_tests)
    add_test(NAME pure_core_editor_session_tests COMMAND malody_catch_core_editor_session_tests)
    add_test(NAME pure_core_ffi_tests COMMAND malody_catch_core_ffi_tests)

    if(MCE_BUILD_QT_TRANSITIONAL)
        add_executable(MalodyCatchCoreTests
            tests/minimal_tests.cpp
        )
        target_link_libraries(MalodyCatchCoreTests PRIVATE MalodyCatchCore)
        add_test(NAME core_minimal_tests COMMAND MalodyCatchCoreTests)
        if(WIN32)
            set_tests_properties(core_minimal_tests PROPERTIES
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$<TARGET_FILE_DIR:Qt6::Gui>;$ENV{PATH}")
        endif()
    endif()
endif()
