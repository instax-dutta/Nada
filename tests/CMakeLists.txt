find_package(Catch2 REQUIRED)

add_executable(nada_tests
    test_decoders.cpp
    test_dsp.cpp
    test_library.cpp
    test_playlist.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/ParametricEQ.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/DspChain.cpp
    ${CMAKE_SOURCE_DIR}/src/library/TagReader.cpp
    ${CMAKE_SOURCE_DIR}/src/library/CoverArtCache.cpp
    ${CMAKE_SOURCE_DIR}/src/library/LibraryScanner.cpp
    ${CMAKE_SOURCE_DIR}/src/library/LibraryManager.cpp
    ${CMAKE_SOURCE_DIR}/src/library/PlaylistManager.cpp
)

target_link_libraries(nada_tests PRIVATE
    Catch2::Catch2WithMain
    Qt6::Core Qt6::Widgets Qt6::Sql Qt6::Gui
    ${TAGLIB_LIBRARIES}
    ${SOXR_LIBRARIES}
    ${FFTW3F_LIBRARIES}
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

target_include_directories(nada_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor
    ${TAGLIB_INCLUDE_DIRS}
    ${SOXR_INCLUDE_DIRS}
    ${FFTW3F_INCLUDE_DIRS}
)

target_link_directories(nada_tests PRIVATE
    ${TAGLIB_LIBRARY_DIRS}
    ${SOXR_LIBRARY_DIRS}
    ${FFTW3F_LIBRARY_DIRS}
)

include(CTest)
include(Catch)
catch_discover_tests(nada_tests)
