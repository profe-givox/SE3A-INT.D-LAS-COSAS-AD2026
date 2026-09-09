#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN 2
#define BLINK_TIME 1000 // Tiempo de parpadeo en milisegundos

void app_main(void)
{

    int led_state = 0; // Estado inicial del LED (apagado)

    // Configurar el pin del LED como salida
    gpio_reset_pin(LED_PIN);

    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    while(1){        
        gpio_set_level(LED_PIN, led_state); // Inicializar el LED en estado apagado
        vTaskDelay(BLINK_TIME / portTICK_PERIOD_MS); // Esperar el tiempo de parpadeo
        led_state = !led_state; // Cambiar el estado del LED
        
        if(led_state == 0){
            ESP_LOGI("LED", "El LED está apagado");
        } else {
            ESP_LOGI("LED", "El LED está encendido");
        }
    }

}
