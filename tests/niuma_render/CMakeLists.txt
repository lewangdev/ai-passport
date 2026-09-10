cmake_minimum_required(VERSION 3.20)
project(niuma_render C CXX)
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
add_subdirectory(../../managed_components/lvgl__lvgl lvgl)
add_executable(niuma_render render.c ../../main/niuma_model.c ../../main/niuma_game.c
    ../../main/niuma_events.c ../../main/niuma_art.c ../../assets/fonts/niuma_font.c
    ../../assets/images/niuma/niuma_cover.c)
target_include_directories(niuma_render PRIVATE stubs . ../../components/bsp/include ../../main)
target_link_libraries(niuma_render PRIVATE lvgl m)
