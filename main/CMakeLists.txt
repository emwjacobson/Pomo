idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")

spiffs_create_partition_image(www ${CMAKE_CURRENT_SOURCE_DIR}/../www FLASH_IN_PROJECT)
