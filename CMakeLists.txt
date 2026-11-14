cmake_minimum_required(VERSION 3.15)
project(HayDayPrices LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

add_executable(HayDayPrices MACOSX_BUNDLE main.cpp)

target_link_libraries(HayDayPrices PRIVATE ${wxWidgets_LIBRARIES})

set_target_properties(HayDayPrices PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.thatsteo.haydaycalculator"
        MACOSX_BUNDLE_BUNDLE_NAME "Calcolatore Hay Day"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
)
