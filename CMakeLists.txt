cmake_minimum_required(VERSION 3.22)

set(IDF_TARGET "esp32s3" CACHE STRING "Waveshare ESP32-S3-ePaper-1.54 V2 target")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# ESP-IDF 5.x drops the emulator's config-dependent USB host requirement during
# early component expansion; 6.x gets it from the component manager instead.
if(IDF_VERSION_MAJOR LESS 6)
    idf_build_set_property(__COMPONENT_REQUIRES_COMMON usb APPEND)
endif()
project(freeinkboy)
