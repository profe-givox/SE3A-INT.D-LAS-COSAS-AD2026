#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>            // Librería para imprimir mensajes ordenados en la consola (Logs)
#include "freertos/FreeRTOS.h"  // Librería base del sistema operativo en tiempo real
#include "freertos/task.h"      // Librería para manejar tiempos y tareas del sistema operativo
#include "sdkconfig.h"          // Configuración interna del chip ESP32
#include "pn532_driver_i2c.h"   // Driver para conectar el lector por I2C
#include "pn532_driver_hsu.h"   // Driver para conectar el lector por puerto Serie (HSU)
#include "pn532_driver_spi.h"   // Driver para conectar el lector por SPI
#include "pn532.h"              // Librería general con los comandos del lector PN532

// CONFIGURACIÓN: Elige con un '1' qué tipo de conexión física usarás. Pon '0' en las demás.
#define PN532_MODE_I2C 0        // 1 = Activar conexión I2C
#define PN532_MODE_HSU 1        // 1 = Activar conexión Serie (UART de alta velocidad)
#define PN532_MODE_SPI 0        // 1 = Activar conexión SPI

// Si elegiste conexión I2C, se configuran estos pines:
#if PN532_MODE_I2C
#define SCL_PIN    (0)          // Pin de reloj
#define SDA_PIN    (1)          // Pin de datos
#define RESET_PIN  (-1)         // Pin de reinicio (-1 significa que no se usa)
#define IRQ_PIN    (3)          // Pin de aviso del lector (Interrupción)

// Si elegiste conexión por Puerto Serie (HSU), se configuran estos pines y velocidad:
#elif PN532_MODE_HSU
#define RESET_PIN      (-1)     // Pin de reinicio (no se usa)
#define IRQ_PIN        (-1)     // Pin de aviso (no se usa)
#define HSU_HOST_RX    (4)      // Pin del ESP32 que recibe datos
#define HSU_HOST_TX    (5)      // Pin del ESP32 que envía datos
#define HSU_UART_PORT  UART_NUM_1 // Puerto serie interno del ESP32 a utilizar
#define HSU_BAUD_RATE  (115200)   // Velocidad de la comunicación

// Si elegiste conexión SPI, se configuran estos pines y velocidad:
#elif PN532_MODE_SPI
#define RESET_PIN      (-1)     
#define IRQ_PIN        (6)      
#define SPI_CS         (5)      // Pin de selección de chip
#define SPI_SCK        (2)      // Pin de reloj
#define SPI_MISO       (3)      // Pin de entrada de datos
#define SPI_MOSI       (4)      // Pin de salida de datos
#define SPI_HOST_NFC   (SPI3_HOST) // Módulo SPI interno del ESP32
#define SPI_CLOCKRATE  (1000000)   // Velocidad del reloj SPI (1 MHz)
#endif

// Etiqueta de texto para identificar que estos mensajes de consola vienen de este archivo
static const char *TAG = "ntag_read";

// Función principal donde arranca el programa en el ESP32
void app_main()
{
    pn532_io_t pn532_io;  // Estructura que guardará la configuración física del lector
    esp_err_t err;        // Variable para revisar si las funciones se ejecutan con éxito o con error

    printf("APP MAIN\n");

#if 0
    // Código oculto de depuración (cambiar a #if 1 si deseas ver mensajes muy detallados en consola)
    esp_log_level_set("PN532", ESP_LOG_DEBUG);
    esp_log_level_set("pn532_driver", ESP_LOG_DEBUG);
    ...
#endif

    // Espera obligatoria de 1 segundo para dejar que el hardware se estabilice al encender
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // INICIALIZACIÓN DEL DRIVER según la conexión elegida arriba:
#if PN532_MODE_I2C
    ESP_LOGI(TAG, "init PN532 in I2C mode");
    ESP_ERROR_CHECK(pn532_new_driver_i2c(SDA_PIN, SCL_PIN, RESET_PIN, IRQ_PIN, 0, &pn532_io));

#elif PN532_MODE_HSU
    ESP_LOGI(TAG, "init PN532 in HSU mode");
    // Configura el driver en modo Serie. Si falla, ESP_ERROR_CHECK reiniciará el ESP32.
    ESP_ERROR_CHECK(pn532_new_driver_hsu(HSU_HOST_RX, HSU_HOST_TX, RESET_PIN, IRQ_PIN, HSU_UART_PORT, HSU_BAUD_RATE, &pn532_io));

#elif PN532_MODE_SPI
    ESP_LOGI(TAG, "init PN532 in SPI mode");
    ESP_ERROR_CHECK(pn532_new_driver_spi(SPI_MISO, SPI_MOSI, SPI_SCK, SPI_CS, -1, IRQ_PIN, SPI_HOST_NFC, SPI_CLOCKRATE, &pn532_io));
#endif

    // Bucle para intentar despertar y conectar con el chip PN532
    do {
        err = pn532_init(&pn532_io); // Inicializa el chip PN532
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "failed to initialize PN532"); // Avisa si falló
            pn532_release(&pn532_io);                    // Libera el driver para reintentar limpio
            vTaskDelay(1000 / portTICK_PERIOD_MS);       // Espera 1 segundo antes de volver a intentar
        }
    } while(err != ESP_OK); // Repite el bucle mientras haya errores

    ESP_LOGI(TAG, "get firmware version");
    uint32_t version_data = 0; // Variable para almacenar los datos de versión del chip
    
    // Bucle para obtener la versión del firmware del chip
    do {
        err = pn532_get_firmware_version(&pn532_io, &version_data);
        if (ESP_OK != err) {
            ESP_LOGI(TAG, "Didn't find PN53x board"); // No encuentra la placa lectora
            pn532_reset(&pn532_io);                   // Reinicia el chip por software
            vTaskDelay(1000 / portTICK_PERIOD_MS);    // Espera 1 segundo
        }
    } while (ESP_OK != err); // Repite si no pudo obtener la versión

    // Extrae y muestra en consola el modelo de chip y la versión exacta de su firmware interno
    ESP_LOGI(TAG, "Found chip PN5%x", (unsigned int)(version_data >> 24) & 0xFF);
    ESP_LOGI(TAG, "Firmware ver. %d.%d", (int)(version_data >> 16) & 0xFF, (int)(version_data >> 8) & 0xFF);

    ESP_LOGI(TAG, "Waiting for an ISO14443A Card ...");
    
    // BUCLE INFINITO: Aquí el programa se queda buscando tarjetas NFC para siempre
    while (1)
    {
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Espacio (Buffer) para guardar el número de serie de la tarjeta (UID)
        uint8_t uid_length;                     // Variable para guardar el tamaño del UID (4 o 7 bytes)

        // Intenta leer una tarjeta estándar ISO14443A (como las etiquetas Mifare o NTAG)
        err = pn532_read_passive_target_id(&pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length, 0);

        // Si se detectó una tarjeta con éxito:
        if (ESP_OK == err)
        {
            ESP_LOGI(TAG, "\nFound an ISO14443A card");
            ESP_LOGI(TAG, "UID Length: %d bytes", uid_length); // Muestra cuántos bytes mide el UID
            ESP_LOGI(TAG, "UID Value:");
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, uid, uid_length, ESP_LOG_INFO); // Imprime el número UID en formato Hexadecimal

            // Selecciona la tarjeta detectada para empezar a operar con ella
            err = pn532_in_list_passive_target(&pn532_io);
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "Failed to inList passive target");
                continue; // Si falla, ignora el resto del código y vuelve a buscar tarjetas
            }

            // Variable para identificar qué modelo exacto de NTAG es
            NTAG2XX_MODEL ntag_model = NTAG2XX_UNKNOWN;
            err = ntag2xx_get_model(&pn532_io, &ntag_model);
            if (err != ESP_OK)
                continue; // Si no puede identificar el modelo, vuelve a empezar el bucle

            int page_max; // Guardará el número máximo de páginas de memoria que tiene la tarjeta
            
            // Revisa qué modelo es para saber cuánta memoria tiene y hasta dónde leer
            switch (ntag_model) {
                case NTAG2XX_NTAG213:
                    page_max = 45; // El modelo NTAG213 tiene 45 páginas de memoria
                    ESP_LOGI(TAG, "found NTAG213 target (or maybe NTAG203)");
                    break;

                case NTAG2XX_NTAG215:
                    page_max = 135; // El modelo NTAG215 tiene 135 páginas de memoria (ej. Figuras Amiibo)
                    ESP_LOGI(TAG, "found NTAG215 target");
                    break;

                case NTAG2XX_NTAG216:
                    page_max = 231; // El modelo NTAG216 tiene 231 páginas de memoria
                    ESP_LOGI(TAG, "found NTAG216 target");
                    break;

                default:
                    ESP_LOGI(TAG, "Found unknown NTAG target!");
                    continue; // Modelo desconocido, vuelve al inicio a buscar otra tarjeta
            }

            // BUCLE DE LECTURA: Lee la tarjeta completa de 4 en 4 páginas (bloques de 16 bytes)
            for(int page=0; page < page_max; page+=4) {
                uint8_t buf[16]; // Espacio temporal para guardar los 16 bytes leídos
                
                err = ntag2xx_read_page(&pn532_io, page, buf, 16); // Intenta leer las 4 páginas juntas
                if (err == ESP_OK) {
                    // Si la lectura es exitosa, imprime los 16 bytes de datos en formato Hexadecimal y texto
                    ESP_LOG_BUFFER_HEXDUMP(TAG, buf, 16, ESP_LOG_INFO);
                }
                else {
                    // Si una página falla (por ejemplo por estar protegida con contraseña), detiene la lectura
                    ESP_LOGI(TAG, "Failed to read page %d", page);
                    break;
                }
            }
            
            // Pausa de 1 segundo después de leer la tarjeta antes de volver a habilitar el lector
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
