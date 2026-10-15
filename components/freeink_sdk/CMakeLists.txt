# FreeInk SDK libraries (vendored under freeink-sdk/) built as one ESP-IDF component on top of
# Arduino-as-a-component. The device flag selects FreeInkBoy's board profile and panel driver.
set(sdk ${CMAKE_CURRENT_LIST_DIR}/../../freeink-sdk/libs)
file(GLOB srcs
    ${sdk}/display/FreeInkDisplay/src/*.cpp
    ${sdk}/display/FreeInkDisplay/src/bus/*.cpp
    ${sdk}/display/FreeInkDisplay/src/driver/*.cpp
    ${sdk}/hardware/InputManager/src/*.cpp)
idf_component_register(SRCS ${srcs}
    INCLUDE_DIRS ${sdk}/hardware/BoardConfig/include ${sdk}/hardware/InputManager/include
                 ${sdk}/display/FreeInkDisplay/include
    REQUIRES framework-arduinoespressif32 esp_netif esp_driver_gpio
    PRIV_REQUIRES ssd1681_fast esp_pm)
# Single framebuffer: Ssd1681Driver keeps its own previous frame for fast diffs.
target_compile_definitions(${COMPONENT_LIB} PUBLIC FREEINK_DEVICE_WS154=1 EINK_DISPLAY_SINGLE_BUFFER_MODE=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -Wno-unused-function)
