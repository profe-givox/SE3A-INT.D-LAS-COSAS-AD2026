#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "led_strip.h" // Componente oficial para controlar LEDs RGB direccionables

static const char *TAG = "SISTEMA";

#define BLINK_GPIO GPIO_NUM_8  // El LED integrado (RGB) del ESP32-C6 suele usar el GPIO 8

// Objeto global para manejar la tira/led RGB
static led_strip_handle_t led_strip;

// TAREA 1: Parpadeo de un LED (Simula control de periféricos)
void vTareaLed(void *pvParameters) {
    // SE ELIMINÓ la configuración manual de GPIO para evitar conflictos con el driver del LED RGB
    uint8_t estado_led = 0;

    while (1) {
        estado_led = !estado_led;
        if (estado_led) {
            // PARÁMETROS CORREGIDOS: (manejador, índice_led, R, G, B)
            // Ponemos el color Verde con brillo alto (0, 255, 0) en el LED índice 0
            led_strip_set_pixel(led_strip, 0, 0, 255, 0);
            led_strip_refresh(led_strip);
            ESP_LOGI("TAREA_LED", "Led RGB encendido (Verde)");
        } else {
            // Apagamos el LED (R: 0, G: 0, B: 0)
            led_strip_clear(led_strip);
            ESP_LOGI("TAREA_LED", "Led RGB apagado");
        }
        // Se bloquea por 500ms reales
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// TAREA 2: Lectura de un sensor ficticio (Simula adquisición de datos)
void vTareaSensor(void *pvParameters) {
    int contador_lecturas = 0;

    while (1) {
        contador_lecturas++;
        ESP_LOGW("TAREA_SENSOR", "Leyendo sensor... Muestra #%d", contador_lecturas);

        // Se bloquea por 1000ms. Al tener tiempos de delay diferentes a la Tarea 1,
        // el planificador alternará perfectamente entre ambas sin saturar el núcleo.
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando configuración del LED RGB mediante RMT...");

    // 1. Configuración básica del LED
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1,
    };

    // 2. Configuración usando el Backend RMT (El estándar nativo y más confiable para WS2812 en ESP32-C6)
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz requeridos para la sincronización del WS2812
        .flags.with_dma = false,
    };

    // 3. Crear el dispositivo usando el backend RMT
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    
    // Aseguramos que inicie apagado
    led_strip_clear(led_strip);

    ESP_LOGI(TAG, "Creando tareas en FreeRTOS...");
    ESP_LOGI(TAG, "Iniciando configuración de tareas en el único núcleo del ESP32-C6...");

    // Creamos la Tarea 1 (Prioridad 2) - Stack aumentado a 4096 para la librería del LED
    xTaskCreate(
        vTareaLed,          // Función que ejecuta la tarea
        "ControlLed",       // Nombre identificativo (para debug)
        4096,               // Stack size aumentado a 4096 por estabilidad del driver
        NULL,               // Parámetros de entrada
        2,                  // Prioridad
        NULL                // Descriptor
    );

    // Creamos la Tarea 2 (Misma Prioridad 2)
    xTaskCreate(
        vTareaSensor,       // Función que ejecuta la tarea
        "LecturaSensor",    // Nombre identificativo
        2048,               // Stack size en bytes
        NULL,               // Parámetros de entrada
        2,                  // Prioridad igual para activar el Time-slicing
        NULL                // Descriptor
    );

    ESP_LOGI(TAG, "Tareas creadas. app_main entra en bucle de bajo consumo.");

    // app_main se queda como un monitor de fondo. 
    while (1) {
        ESP_LOGI(TAG, "[Alerta] El sistema sigue vivo y estable.");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
