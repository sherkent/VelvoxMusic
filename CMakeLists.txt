cmake_minimum_required(VERSION 3.21)

project(VelvoxMusic
    VERSION 0.1.0
    DESCRIPTION "A modern local music player for Windows"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick Qml Sql Svg ShaderTools Widgets Core5Compat)

# Suppress non-fatal Qt CMake policy warnings while keeping compatible behavior.
# QTP0001: use the new default resource prefix for QML modules.
# QTP0004: we have .qml files in subdirectories without per-directory qmldir files.
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 OLD)

qt_standard_project_setup()

# ------------------------------------------------------------------------------
# Third-party libraries
# ------------------------------------------------------------------------------
include(FetchContent)

# miniaudio: single-header audio playback/decoding library (Unlicense / dual-licensed)
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniaudio)

# kissfft: small FFT library used for realtime spectrum analysis (BSD-3-Clause)
FetchContent_Declare(
    kissfft
    GIT_REPOSITORY https://github.com/mborgerding/kissfft.git
    GIT_TAG 131.1.0
    GIT_SHALLOW TRUE
)
# We only need the kissfft source files; avoid building its CMake targets
# (shared/static libraries, tests, tools) and the old-cmake deprecation warning.
FetchContent_Populate(kissfft)

# TagLib: LGPL metadata parser. Velvox Music links to TagLib dynamically.
# You must ship the TagLib shared library (e.g. taglib.dll on Windows) and its
# license text alongside the application. See README.md for details.
#
# vcpkg and distro packages use different config/target names, so probe both.
set(TAGLIB_TARGET "")
foreach(_pkg_name TagLib taglib)
    find_package(${_pkg_name} CONFIG QUIET)
    if (${_pkg_name}_FOUND)
        foreach(_tgt_name TagLib::TagLib taglib::taglib)
            if (TARGET ${_tgt_name})
                set(TAGLIB_TARGET ${_tgt_name})
                break()
            endif()
        endforeach()
    endif()
    if (TAGLIB_TARGET)
        break()
    endif()
endforeach()

if (NOT TAGLIB_TARGET)
    find_package(TagLib REQUIRED)
    if (TARGET TagLib::TagLib)
        set(TAGLIB_TARGET TagLib::TagLib)
    elseif(TARGET tag)
        set(TAGLIB_TARGET tag)
    else()
        message(FATAL_ERROR "TagLib target not found")
    endif()
endif()

# ------------------------------------------------------------------------------
# Backend sources
# ------------------------------------------------------------------------------
set(CORE_SOURCES
    src/core/Track.h
    src/core/Playlist.h
    src/core/MetadataReader.h
    src/core/MetadataReader.cpp
    src/core/DatabaseManager.h
    src/core/DatabaseManager.cpp
    src/core/LibraryManager.h
    src/core/LibraryManager.cpp
    src/core/AudioEngine.h
    src/core/AudioEngine.cpp
    src/core/SpectrumAnalyzer.h
    src/core/SpectrumAnalyzer.cpp
    src/core/LyricsParser.h
    src/core/LyricsParser.cpp
    src/core/WindowsSystemIntegration.h
    src/core/WindowsSystemIntegration.cpp
    src/core/WindowBackdrop.h
    src/core/WindowBackdrop.cpp
    src/core/CoverColorExtractor.h
    src/core/CoverColorExtractor.cpp
)

set(MODEL_SOURCES
    src/models/TrackListModel.h
    src/models/TrackListModel.cpp
    src/models/TrackListProxyModel.h
    src/models/TrackListProxyModel.cpp
    src/models/AlbumListModel.h
    src/models/AlbumListModel.cpp
    src/models/ArtistListModel.h
    src/models/ArtistListModel.cpp
    src/models/PlaylistModel.h
    src/models/PlaylistModel.cpp
    src/models/LyricsModel.h
    src/models/LyricsModel.cpp
)

# ------------------------------------------------------------------------------
# Main executable
# ------------------------------------------------------------------------------
qt_add_executable(VelvoxMusic
    src/app/main.cpp
    ${CORE_SOURCES}
    ${MODEL_SOURCES}
    ${kissfft_SOURCE_DIR}/kiss_fft.c
    ${kissfft_SOURCE_DIR}/kiss_fftr.c
)

# Mark singleton QML types
set_source_files_properties(qml/theme/ThemeManager.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/theme/Motion.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# ------------------------------------------------------------------------------
# QML module (modern CMake, no hand-written .qrc)
# ------------------------------------------------------------------------------
qt_add_qml_module(VelvoxMusic
    URI VelvoxMusic
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/pages/HomePage.qml
        qml/pages/AudioTestPage.qml
        qml/pages/LibraryTestPage.qml
        qml/pages/LibraryPage.qml
        qml/pages/AlbumPage.qml
        qml/pages/ArtistPage.qml
        qml/pages/PlaylistsPage.qml
        qml/pages/SearchPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/NowPlayingPage.qml
        qml/components/AppButton.qml
        qml/components/MeshGradientBackground.qml
        qml/components/AcrylicPanel.qml
        qml/components/MiniPlayerWindow.qml
        qml/components/PlayPauseButton.qml
        qml/components/CrossFadeImage.qml
        qml/components/LyricsView.qml
        qml/theme/ThemeManager.qml
        qml/theme/Motion.qml
    RESOURCES
        resources/icons/app-icon.svg
)

# ------------------------------------------------------------------------------
# Shaders (compile .frag to .qsb at build time via Qt Shader Tools)
# ------------------------------------------------------------------------------
qt_add_shaders(VelvoxMusic "VelvoxMusicShaders"
    BASE "resources/shaders"
    PREFIX "/"
    FILES
        resources/shaders/meshgradient.frag
)

set_target_properties(VelvoxMusic PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE
)

# Release build optimizations: LTO, stripped symbols, high optimization.
# These significantly reduce binary size and improve runtime performance.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|MSVC")
    set_property(TARGET VelvoxMusic PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET VelvoxMusic PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
endif()

if(MSVC)
    # /O2 /Ob2: maximum speed + inline expansion
    # /GL: whole-program optimization (enables LTCG at link time)
    # /Gy: function-level linking, allows /OPT:REF to discard unused functions
    target_compile_options(VelvoxMusic PRIVATE
        $<$<CONFIG:Release>:/O2 /Ob2 /GL /Gy>
        $<$<CONFIG:MinSizeRel>:/O1 /Ob1 /GL /Gy>
    )
    target_link_options(VelvoxMusic PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
        $<$<CONFIG:MinSizeRel>:/LTCG /OPT:REF /OPT:ICF>
    )
    # DWM API for system Acrylic/Mica backdrop.
    target_link_libraries(VelvoxMusic PRIVATE dwmapi)
else()
    # Let CMake's IPO property add -flto automatically; here we just add
    # optimization and symbol stripping. MinGW users will see the binary size
    # drop significantly with -s.
    target_compile_options(VelvoxMusic PRIVATE
        $<$<CONFIG:Release>:-O3 -fomit-frame-pointer>
        $<$<CONFIG:MinSizeRel>:-Os>
    )
    target_link_options(VelvoxMusic PRIVATE
        $<$<CONFIG:Release>:-s>
        $<$<CONFIG:MinSizeRel>:-s>
    )
endif()

target_link_libraries(VelvoxMusic PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Sql
    Qt6::Svg
    Qt6::Widgets
    Qt6::Core5Compat
    ${TAGLIB_TARGET}
)

target_include_directories(VelvoxMusic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${miniaudio_SOURCE_DIR}
    ${kissfft_SOURCE_DIR}
)

# ------------------------------------------------------------------------------
# Install
# ------------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS VelvoxMusic
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

# ------------------------------------------------------------------------------
# Windows deployment: copy Qt + TagLib dependencies into the build output dir
# ------------------------------------------------------------------------------
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "${QT6_INSTALL_PREFIX}/bin" "${Qt6Core_DIR}/../../../bin"
    )
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET VelvoxMusic POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt dependencies with windeployqt..."
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                $<IF:$<CONFIG:Debug>,--debug,--release>
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                --qmldir ${CMAKE_CURRENT_SOURCE_DIR}/qml
                $<TARGET_FILE:VelvoxMusic>
            WORKING_DIRECTORY $<TARGET_FILE_DIR:VelvoxMusic>
            COMMENT "Running windeployqt"
        )
    else()
        message(WARNING "windeployqt not found; Qt deployment step will be skipped")
    endif()

    # TagLib is LGPL and dynamically linked; windeployqt cannot know about it,
    # so we attempt to copy the DLL next to the executable automatically.
    if(TARGET ${TAGLIB_TARGET})
        get_target_property(TAGLIB_IMPORTED_LOCATION ${TAGLIB_TARGET} IMPORTED_LOCATION_RELEASE)
        if(NOT TAGLIB_IMPORTED_LOCATION)
            get_target_property(TAGLIB_IMPORTED_LOCATION ${TAGLIB_TARGET} IMPORTED_LOCATION)
        endif()
        if(TAGLIB_IMPORTED_LOCATION)
            get_filename_component(TAGLIB_DIR ${TAGLIB_IMPORTED_LOCATION} DIRECTORY)
            find_file(TAGLIB_DLL
                NAMES tag.dll taglib.dll
                PATHS ${TAGLIB_DIR} ${TAGLIB_DIR}/../bin ${TAGLIB_DIR}/../../bin
                NO_DEFAULT_PATH
            )
            if(TAGLIB_DLL)
                add_custom_command(TARGET VelvoxMusic POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TAGLIB_DLL} $<TARGET_FILE_DIR:VelvoxMusic>
                    COMMENT "Copying TagLib DLL (${TAGLIB_DLL}) to output directory"
                )
            else()
                message(WARNING "TagLib import library found but DLL not found; you must ship the TagLib DLL manually")
            endif()
        endif()
    endif()
endif()
