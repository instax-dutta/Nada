cmake_minimum_required(VERSION 3.27)
project(Nada VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(DEFINED CMAKE_TOOLCHAIN_FILE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Quick Sql OpenGL)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib)
pkg_check_modules(SOXR   REQUIRED soxr)
pkg_check_modules(FFTW3F REQUIRED fftw3f)
pkg_check_modules(FFTW3  REQUIRED fftw3)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(src vendor)

if(WIN32)
    set(PLATFORM_OUTPUT src/output/WasapiOutput.cpp)
    set(PLATFORM_LIBS ole32 uuid)
elseif(APPLE)
    set(PLATFORM_OUTPUT src/output/CoreAudioOutput.cpp)
    set(PLATFORM_LIBS "-framework CoreAudio" "-framework AudioToolbox")
else()
    pkg_check_modules(ALSA REQUIRED alsa)
    set(PLATFORM_OUTPUT src/output/AlsaOutput.cpp)
    set(PLATFORM_LIBS ${ALSA_LIBRARIES})
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(NADA_ICON "${CMAKE_SOURCE_DIR}/resources/icons/nada.icns")

add_executable(nada
    resources/resources.qrc
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/LibraryModel.cpp
    src/ui/LibraryView.cpp
    src/ui/WaveformSeekBar.cpp
    src/ui/NowPlayingView.cpp
    src/ui/QueueView.cpp
    src/ui/SystemTray.cpp
    src/ui/VisualizerWidget.cpp
    src/ui/FreqResponseGraph.cpp
    src/ui/EQView.cpp
    src/ui/SettingsView.cpp
    src/ui/ToggleSwitch.cpp
    src/ui/SlimSlider.cpp
    src/ui/PlayingIndicator.cpp
    src/engine/PlaybackEngine.cpp
    src/decoder/WavDecoder.cpp
    src/decoder/FlacDecoder.cpp
    src/decoder/Mp3Decoder.cpp
    src/decoder/DecoderFactory.cpp
    src/output/AudioOutputFactory.cpp
    src/dsp/ParametricEQ.cpp
    src/dsp/DspChain.cpp
    src/library/TagReader.cpp
    src/library/CoverArtCache.cpp
    src/library/LibraryScanner.cpp
    src/library/LibraryManager.cpp
    src/library/PlaylistManager.cpp
    src/library/ReplayGainScanner.cpp
    ${PLATFORM_OUTPUT}
)

target_link_libraries(nada PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Quick Qt6::Sql Qt6::OpenGL
    ${TAGLIB_LIBRARIES}
    ${SOXR_LIBRARIES}
    ${FFTW3F_LIBRARIES}
    ${FFTW3_LIBRARIES}
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    ${PLATFORM_LIBS}
)

target_include_directories(nada PRIVATE
    ${TAGLIB_INCLUDE_DIRS}
    ${SOXR_INCLUDE_DIRS}
    ${FFTW3F_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
)

target_link_directories(nada PRIVATE
    ${TAGLIB_LIBRARY_DIRS}
    ${SOXR_LIBRARY_DIRS}
    ${FFTW3F_LIBRARY_DIRS}
    ${FFTW3_LIBRARY_DIRS}
)

add_subdirectory(tests)

if(APPLE)
    set_source_files_properties(${NADA_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(nada PRIVATE ${NADA_ICON})
    set_target_properties(nada PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.nada.player"
        MACOSX_BUNDLE_BUNDLE_NAME "Nada"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "nada.icns"
    )
endif()

# CPack packaging
set(CPACK_PACKAGE_NAME "Nada")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Native HiFi Music Player")
set(CPACK_PACKAGE_VENDOR "Nada")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Nada")

if(APPLE)
    set(CPACK_BUNDLE_NAME "Nada")
    set(CPACK_BUNDLE_PLIST_SOURCE "${CMAKE_SOURCE_DIR}/packaging/Info.plist.in")
    set(CPACK_GENERATOR "DragNDrop")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")
else()
    set(CPACK_GENERATOR "DEB;RPM")
endif()

include(CPack)
