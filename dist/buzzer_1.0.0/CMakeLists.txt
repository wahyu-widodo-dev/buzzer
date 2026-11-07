idf_component_register(
    SRCS "buzzer.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_common esp_driver_gpio esp_driver_ledc esp_timer
    PRIV_REQUIRES log
)
