idf_component_register(SRCS "main.cpp"
                    REQUIRES spi_flash RadioLib oled_display nvs_flash driver
                    INCLUDE_DIRS ".")