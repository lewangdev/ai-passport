idf_component_register(
    SRCS "main.c"
         "dagou_app.c"
         "dagou_audio.c"
         "dagou_model.c"
         "../assets/fonts/dagou_ui.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
         "demo_display.c"
         "demo_button.c"
         "demo_audio.c"
         "demo_battery.c"
         "demo_radio.c"
         "demo_wifi.c"
         "demo_ble.c"
         "demo_low_power.c"
    INCLUDE_DIRS "."
    REQUIRES bsp bt esp_event esp_hw_support esp_netif esp_timer esp_wifi nvs_flash
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)

foreach(asset da gou jiao ha ji mi)
    target_add_binary_data(${COMPONENT_LIB} "${CMAKE_CURRENT_LIST_DIR}/../assets/dagou-tap/${asset}.pcm" BINARY)
endforeach()
foreach(asset dagou_close dagou_open cat_close cat_open)
    target_add_binary_data(${COMPONENT_LIB} "${CMAKE_CURRENT_LIST_DIR}/../assets/dagou-tap/${asset}.bgra" BINARY)
endforeach()
