add_library(dizi_deps INTERFACE)

add_subdirectory(json EXCLUDE_FROM_ALL)

add_subdirectory(curl EXCLUDE_FROM_ALL)

if (TARGET libcurl_static)
    set(DIZI_CURL_TARGET libcurl_static)
else ()
    set(DIZI_CURL_TARGET libcurl)
endif ()

target_link_libraries(dizi_deps INTERFACE ${DIZI_CURL_TARGET} nlohmann_json::nlohmann_json)
