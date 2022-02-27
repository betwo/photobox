cmake_minimum_required(VERSION 3.1.0)

project(photobox)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
find_package(Gphoto2 REQUIRED)
find_package(Boost COMPONENTS REQUIRED system)


include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Gphoto2_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_LIST_DIR}/build)



## Enforce that we use C++11
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

add_subdirectory(3rdparty)

set(CMAKE_AUTOMOC ON)

#qt5_add_resources(QT_RESOURCES res/csapex_resources.qrc)
qt5_wrap_ui(QT_UI
    ui/photobox.ui)


add_executable(photobox
    src/photobox.cpp
    src/photobox_window.cpp
    src/camera.cpp
    src/director.cpp
    src/pixmap.hpp
    src/arduino_button.cpp

    ${QT_UI})

target_link_libraries(photobox
    ${Gphoto2_LIBRARIES}
    ${Boost_LIBRARIES}
    Qt5::Core Qt5::Gui Qt5::Widgets
    jpeg)

target_link_libraries(photobox
    libraw::libraw
