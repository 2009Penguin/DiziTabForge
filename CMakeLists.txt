cmake_minimum_required(VERSION 3.16)
project(DiziTabForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif ()

foreach (DIZI_CANDIDATE "D:/Anaconda3" "C:/Anaconda3" "C:/ProgramData/Anaconda3")
    if (EXISTS "${DIZI_CANDIDATE}")
        list(APPEND CMAKE_IGNORE_PREFIX_PATH "${DIZI_CANDIDATE}")
    endif ()
endforeach ()

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Network)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_MISC_DOCS OFF CACHE BOOL "" FORCE)
set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_FILE ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_FTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_GOPHER ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_MQTT ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_RTSP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_SMTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TELNET ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TFTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_DICT ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_IMAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_POP3 ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_IPFS ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_WEBSOCKETS ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_ALTSVC ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_HSTS ON CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
set(CURL_USE_GSSAPI OFF CACHE BOOL "" FORCE)
set(USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
set(CURL_BROTLI OFF CACHE BOOL "" FORCE)
set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
if (WIN32)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
endif ()

add_subdirectory(deps)

set(DIZI_MODEL_SOURCES)

set(DIZI_UTIL_SOURCES
        core/util/StringUtil.cpp
        core/util/ImageUtil.cpp
        core/util/PathUtil.cpp)

set(DIZI_CONFIG_SOURCES
        core/config/ConfigRepository.cpp)

set(DIZI_PARSE_SOURCES
        core/parse/ScoreTokenizer.cpp
        core/parse/ScoreParser.cpp
        core/parse/ScoreInterpreter.cpp
        core/parse/ScorePipeline.cpp)

set(DIZI_NETWORK_SOURCES
        core/network/ImageEncoder.cpp
        core/network/GmlClient.cpp)

set(DIZI_RENDER_SOURCES
        core/render/FingerTable.cpp
        core/render/NoteFormatter.cpp
        core/render/TrueTypeFace.cpp
        core/render/CjkFontFile.cpp
        core/render/PdfWriter.cpp
        core/render/PageCanvas.cpp
        core/render/QtPdfCanvas.cpp
        core/render/VectorPdfCanvas.cpp
        core/render/PdfRenderer.cpp)

set(DIZI_UI_SOURCES
        ui/MainWindow.cpp
        ui/MainWindow.ui
        ui/CropDialog.cpp
        ui/CropDialog.ui
        ui/SettingsDialog.cpp
        ui/SettingsDialog.ui)

set(DIZI_ROOT_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(dizi_model INTERFACE)
target_include_directories(dizi_model INTERFACE "${DIZI_ROOT_INCLUDES}")
target_link_libraries(dizi_model INTERFACE Qt6::Core)

add_library(dizi_util STATIC ${DIZI_UTIL_SOURCES})
target_include_directories(dizi_util PUBLIC "${DIZI_ROOT_INCLUDES}")
target_link_libraries(dizi_util PUBLIC Qt6::Core Qt6::Gui)

add_library(dizi_config STATIC ${DIZI_CONFIG_SOURCES})
target_include_directories(dizi_config PUBLIC "${DIZI_ROOT_INCLUDES}")
target_link_libraries(dizi_config PUBLIC Qt6::Core dizi_util)

add_library(dizi_parse STATIC ${DIZI_PARSE_SOURCES})
target_include_directories(dizi_parse PUBLIC "${DIZI_ROOT_INCLUDES}")
target_link_libraries(dizi_parse PUBLIC Qt6::Core dizi_model dizi_util)

add_library(dizi_network STATIC ${DIZI_NETWORK_SOURCES})
target_include_directories(dizi_network PUBLIC "${DIZI_ROOT_INCLUDES}")
target_link_libraries(dizi_network PUBLIC Qt6::Core dizi_model dizi_config dizi_util)
target_link_libraries(dizi_network PRIVATE dizi_deps)

add_library(dizi_render STATIC ${DIZI_RENDER_SOURCES})
target_include_directories(dizi_render PUBLIC "${DIZI_ROOT_INCLUDES}")
target_link_libraries(dizi_render PUBLIC Qt6::Core Qt6::Gui dizi_model dizi_util dizi_parse)

add_library(dizi_ui STATIC ${DIZI_UI_SOURCES})
target_include_directories(dizi_ui PUBLIC "${DIZI_ROOT_INCLUDES}")
target_link_libraries(dizi_ui PUBLIC dizi_model dizi_util dizi_config dizi_parse
        dizi_network dizi_render Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)

add_executable(DiziTabForge main.cpp res/res.qrc)
target_link_libraries(DiziTabForge PRIVATE dizi_ui)

if (WIN32)
    enable_language(RC)
    target_sources(DiziTabForge PRIVATE res/app.rc)
    set_target_properties(DiziTabForge PROPERTIES WIN32_EXECUTABLE "$<NOT:$<CONFIG:Debug>>")
endif ()

foreach (DIZI_LOG_TARGET dizi_util dizi_config dizi_parse dizi_network dizi_render dizi_ui
        DiziTabForge)
    target_compile_definitions(${DIZI_LOG_TARGET} PRIVATE
            "$<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG_OUTPUT>"
            "$<$<NOT:$<CONFIG:Debug>>:NDEBUG>")
endforeach ()

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/bin")
        add_custom_command(TARGET DiziTabForge POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:DiziTabForge>/plugins/platforms/")
        if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
            add_custom_command(TARGET DiziTabForge POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                    "$<TARGET_FILE_DIR:DiziTabForge>/plugins/platforms/")
        endif ()
        add_custom_command(TARGET DiziTabForge POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:DiziTabForge>/plugins/imageformats/")
        foreach (QT_IMAGE_PLUGIN jpeg gif webp ico tiff svg)
            if (EXISTS "${QT_INSTALL_PATH}/plugins/imageformats/q${QT_IMAGE_PLUGIN}${DEBUG_SUFFIX}.dll")
                add_custom_command(TARGET DiziTabForge POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${QT_INSTALL_PATH}/plugins/imageformats/q${QT_IMAGE_PLUGIN}${DEBUG_SUFFIX}.dll"
                        "$<TARGET_FILE_DIR:DiziTabForge>/plugins/imageformats/")
            endif ()
        endforeach ()
        foreach (QT_LIB Core Gui Widgets Network)
            add_custom_command(TARGET DiziTabForge POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                    "$<TARGET_FILE_DIR:DiziTabForge>")
        endforeach ()
    endif ()
endif ()
