idf_component_register(SRC_DIRS "src"
                        INCLUDE_DIRS "include"
                        PRIV_REQUIRES "rtos_cpp"
                                    "bt"
                                    "nvs_flash"
                                    "esp_driver_uart"
                                    "esp_driver_gpio")