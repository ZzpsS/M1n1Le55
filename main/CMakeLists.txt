idf_component_register(SRCS "main.c" "fm1_wire.c" "fm1_params.c" "fm1_host.c" "fm1_ui.c" "fm1_diag.c" "fm1_package.c" "fm1_storage.c" "fm1_manager.c" "fm1_ota.c" "fm1_update.c" "fm1_operation.c" "fm1_ble.c" "fm1_ble_midi.c" "fm1_sample_format.c" "fm1_samples.c" "m1_theme.c" "m1_fonts.c" "m1_waveform.c" "fm1_adapter_wire.c" "fm1_adapter_parse.c" "fm1_adapter.c" "fm1_adapter_ui.c" "fm1_console.c" "fm1_exchange_file.c" "fm1_action.c" "fm1_action_summary.c" "fm1_object.c"
                       INCLUDE_DIRS "." REQUIRES usb esp_timer esp_driver_usb_serial_jtag json mbedtls fatfs bt nvs_flash esp_hosted)
target_sources(${COMPONENT_LIB} PRIVATE "m1_firmware_icons.c")

# ESP-Hosted 2.12.9 runs its constructor before FreeRTOS releases startup RAM.
# Tab5 initializes it explicitly in the BLE worker, after the scheduler starts.
# Keep the managed component intact; omit only its automatic init/deinit TU.
idf_component_get_property(hosted_lib espressif__esp_hosted COMPONENT_LIB)
get_target_property(hosted_sources ${hosted_lib} SOURCES)
list(FILTER hosted_sources EXCLUDE REGEX "port_esp_hosted_host_init[.]c$")
set_property(TARGET ${hosted_lib} PROPERTY SOURCES "${hosted_sources}")




