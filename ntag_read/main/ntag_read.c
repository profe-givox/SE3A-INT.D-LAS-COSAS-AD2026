#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>            // Librería para logs en consola
#include "freertos/FreeRTOS.h"  // Librería base FreeRTOS
#include "freertos/task.h"      // Manejo de tareas y delays
#include "driver/gpio.h"        // Driver para control de GPIOs
#include "sdkconfig.h"
#include "pn532_driver_i2c.h"
#include "pn532_driver_hsu.h"
#include "pn532_driver_spi.h"
#include "pn532.h"

// CONFIGURACIÓN DE PINES DEL LED RGB
#define GPIO_LED_RED   GPIO_NUM_18
#define GPIO_LED_GREEN GPIO_NUM_19
#define GPIO_LED_BLUE  GPIO_NUM_21

// CONFIGURACIÓN DE CONEXIÓN PN532
#define PN532_MODE_I2C 0
#define PN532_MODE_HSU 1
#define PN532_MODE_SPI 0

#if PN532_MODE_HSU
#define RESET_PIN       (-1)
#define IRQ_PIN         (-1)
#define HSU_HOST_RX     (4)
#define HSU_HOST_TX     (5)
#define HSU_UART_PORT   UART_NUM_1
#define HSU_BAUD_RATE   (115200)
#endif

static const char *TAG = "ntag_read";

// DEFINICIÓN DE TARJETAS AUTORIZADAS (UIDs autorizados)
// Cambia estos valores por el UID exacto de tu tarjeta permitida
static const uint8_t AUTHORIZED_UID[] = {0x99, 0xb7, 0x90, 0x29}; // Ejemplo de UID de 7 bytes
static const uint8_t AUTHORIZED_UID_LEN = sizeof(AUTHORIZED_UID);

// Función para inicializar los pines del LED RGB
void init_rgb_led(void) {
    gpio_reset_pin(GPIO_LED_RED);
    gpio_reset_pin(GPIO_LED_GREEN);
    gpio_reset_pin(GPIO_LED_BLUE);

    gpio_set_direction(GPIO_LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LED_BLUE, GPIO_MODE_OUTPUT);

    // Apagar todos los colores al inicio (0 para Cátodo Común, 1 si fuera Ánodo Común)
    gpio_set_level(GPIO_LED_RED, 0);
    gpio_set_level(GPIO_LED_GREEN, 0);
    gpio_set_level(GPIO_LED_BLUE, 0);
}

// Función para cambiar el color del LED RGB
void set_rgb_color(uint8_t red, uint8_t green, uint8_t blue) {
    gpio_set_level(GPIO_LED_RED, red);
    gpio_set_level(GPIO_LED_GREEN, green);
    gpio_set_level(GPIO_LED_BLUE, blue);
}

// Función para verificar si el UID leído está autorizado
bool is_access_granted(uint8_t *uid, uint8_t uid_len) {
    if (uid_len != AUTHORIZED_UID_LEN) {
        return false;
    }
    return memcmp(uid, AUTHORIZED_UID, uid_len) == 0;
}

void app_main()
{
    pn532_io_t pn532_io;
    esp_err_t err;

    printf("APP MAIN\n");

    // Inicializa los GPIOs del LED RGB
    init_rgb_led();

    // Espera obligatoria para estabilizar el hardware
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Inicialización del driver PN532 (HSU)
#if PN532_MODE_HSU
    ESP_LOGI(TAG, "init PN532 in HSU mode");
    ESP_ERROR_CHECK(pn532_new_driver_hsu(HSU_HOST_RX, HSU_HOST_TX, RESET_PIN, IRQ_PIN, HSU_UART_PORT, HSU_BAUD_RATE, &pn532_io));
#endif

    // Bucle para despertar el chip PN532
    do {
        err = pn532_init(&pn532_io);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "failed to initialize PN532");
            pn532_release(&pn532_io);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    } while(err != ESP_OK);

    ESP_LOGI(TAG, "Waiting for an ISO14443A Card ...");

    // Indicación inicial: Azul rápido para indicar listo/esperando
    set_rgb_color(0, 0, 1);

    while (1)
    {
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
        uint8_t uid_length = 0;

        // Intenta detectar una tarjeta
        err = pn532_read_passive_target_id(&pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length, 0);

        if (ESP_OK == err)
        {
            ESP_LOGI(TAG, "\nFound an ISO14443A card");
            ESP_LOGI(TAG, "UID Length: %d bytes", uid_length);
            ESP_LOGI(TAG, "UID Value:");
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, uid, uid_length, ESP_LOG_INFO);

            // Comprueba si el UID tiene acceso
            if (is_access_granted(uid, uid_length)) {
                ESP_LOGI(TAG, ">>> ACCESO CONCEDIDO <<<");
                set_rgb_color(0, 1, 0); // Verde
            } else {
                ESP_LOGW(TAG, ">>> ACCESO DENEGADO <<<");
                set_rgb_color(1, 0, 0); // Rojo
            }

            // Mantiene el LED encendido durante 2 segundos
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            // Regresa al estado de espera (Azul o apagado)
            set_rgb_color(0, 0, 1);
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}