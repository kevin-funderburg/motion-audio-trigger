idf_component_register(SRCS "esp32_pir_main.c" "wifi/wifi_connection.c" "http/motion_http.c"
                       INCLUDE_DIRS "." "${CMAKE_CURRENT_BINARY_DIR}"
                       REQUIRES esp_driver_gpio esp_wifi esp_event esp_netif esp_timer nvs_flash esp_http_client)

set(MOTION_SERVER_URL "" CACHE STRING "Full URL of the Pi's /motion endpoint")
target_compile_definitions(${COMPONENT_LIB} PRIVATE MOTION_SERVER_URL="${MOTION_SERVER_URL}")

file(GLOB LOCAL_WIFI_CONFIG CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wifi/wifi_config.h")
if(LOCAL_WIFI_CONFIG)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/wifi/wifi_config.h" wifi/wifi_config_build.h COPYONLY)
else()
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/wifi/wifi_config.example.h" wifi/wifi_config_build.h COPYONLY)
endif()
