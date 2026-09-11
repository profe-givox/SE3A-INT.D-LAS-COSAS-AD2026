#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h" // Librería moderna para ADC en ESP-IDF

#define EJEMPLO_ADC_GPIO ADC_CHANNEL_4 // Canal ADC para el ESP32-C6

void app_main(void) {
    // 1. Configurar la unidad ADC (ADC1)
    // CORREGIDO: Se cambió adc_oneshot_unit_init_config_t por adc_oneshot_unit_init_cfg_t
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = 0, // 0 asigna el reloj por defecto
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // 2. Configurar el canal analógico (Atenuación y resolución)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // Resolución de 12 bits (0 - 4095)
        .atten = ADC_ATTEN_DB_12,         // Rango de voltaje para ESP-IDF v6 (0V a ~3.3V)
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EJEMPLO_ADC_GPIO, &config));

    int raw_value = 0;

    while (1) {
        // 3. Leer el valor analógico
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EJEMPLO_ADC_GPIO, &raw_value));
        
        printf("Valor ADC leído: %d\n", raw_value);

        // Esperar 1 segundo
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
