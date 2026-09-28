/* 
   Las líneas con #include son "Directivas de Inclusión". 
   Equivalen a los 'import' en Python/Java o 'require' en JavaScript.
   Sirven para traer funciones y herramientas que no vienen por defecto en el lenguaje.
*/
#include <stdio.h>                       // Biblioteca estándar de C para entrada/salida (como printf).
#include "freertos/FreeRTOS.h"          // Sistema operativo en tiempo real (RTOS) usado por el ESP32.
#include "freertos/task.h"              // Permite manejar tareas y pausas (delays).
#include "driver/ledc.h"                // Controlador oficial de ESP-IDF para manejar PWM (LED Control).
#include "esp_adc/adc_oneshot.h"        // Controlador oficial de ESP-IDF para lecturas analógicas (ADC).
#include "esp_err.h"                    // Herramientas para el manejo de errores en ESP-IDF.

/* 
   Las líneas con #define son "Macros". 
   No son variables. El preprocesador de C reemplaza estos nombres por sus valores 
   justo antes de compilar el código. No consumen memoria RAM.
*/
// Configuración de PWM (LEDC)
#define LEDC_OUTPUT_IO          (2)               // Pin físico (GPIO) donde conectaremos el LED.
#define LEDC_CHANNEL            LEDC_CHANNEL_0    // El chip tiene varios "canales" internos de PWM; usamos el 0.
#define LEDC_MODE               LEDC_LOW_SPEED_MODE // Modo de velocidad del periférico.
#define LEDC_TIMER              LEDC_TIMER_0      // Temporizador interno asignado al PWM.
#define LEDC_DUTY_RES           LEDC_TIMER_12_BIT // Resolución: 12 bits significa que el brillo irá de 0 a 4095.
#define LEDC_FREQUENCY          (5000)            // Frecuencia de la señal en Hertz (5 kHz).

// Configuración de ADC (Lectura analógica)
#define ADC_CHANNEL             ADC_CHANNEL_4     // Canal 6 del ADC1 (En la mayoría de ESP32 es el GPIO 4).
#define ADC_ATTEN               ADC_ATTEN_DB_12   // Atenuador eléctrico. Permite leer el rango completo de 0V a 3.3V.

/*
   app_main es el punto de entrada del programa.
   Equivale al 'main' en otros lenguajes o al 'void setup()' + 'void loop()' de Arduino.
   'void' significa que la función no devuelve ningún valor.
*/
void app_main(void)
{
    /* 
       En C, una 'struct' (estructura) es un contenedor que agrupa múltiples variables.
       Aquí creamos una estructura de tipo 'ledc_timer_config_t' llamada 'ledc_timer' 
       y la llenamos con las configuraciones que definimos arriba.
    */
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK  // Selecciona automáticamente la mejor fuente de reloj interna.
    };
    
    /*
       ESP_ERROR_CHECK es una macro de ESP-IDF para seguridad.
       Si la función de adentro falla (devuelve algo distinto de ESP_OK), el ESP32
       reinicia el programa e imprime el error en la consola. Es como un 'try-catch' automático.
       
       El símbolo '&' antes de 'ledc_timer' significa "pasar por referencia" (Puntero). 
       En C, en lugar de pasarle toda la estructura pesada a la función, le pasamos 
       únicamente la dirección de memoria donde está guardada.
    */
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configuramos el canal de PWM asociándolo al temporizador y al pin del LED
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE, // Desactivamos interrupciones por hardware para este canal.
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0,                 // El LED inicia apagado (Ciclo de trabajo 0%).
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));



    /* 
       Manejo de memoria/Objetos en C:
       'adc_oneshot_unit_handle_t' es un tipo de dato "manejador" (un puntero oculto).
       Sirve como una referencia o "ID" para controlar el hardware del ADC.
    */
    adc_oneshot_unit_handle_t adc1_handle;
    
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1, // El ESP32 tiene dos bloques ADC independientes; usamos el 1.
    };
    
    // CORRECCIÓN PARA v5.0+: La función correcta es 'adc_oneshot_new_unit'
    // Inicializamos el bloque del ADC pasándole la dirección de nuestro manejador '&adc1_handle'
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // Configuramos los parámetros específicos del canal de lectura (resolución)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12, // Resolución de 12 bits: las lecturas físicas de voltaje irán de 0 a 4095.
        .atten = ADC_ATTEN,
    };
    
    // CORRECCIÓN PARA v5.0+: La función correcta es 'adc_oneshot_config_channel'
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

    // Declaramos una variable entera (32 bits en ESP32) para almacenar el valor analógico.
    int adc_raw_value = 0;

    /* 
       while(1) crea un bucle infinito. 
       Equivale al 'while True:' de Python o al bloque 'loop()' de Arduino.
    */
    while (1) {
        /* 
           Leemos el estado actual del potenciómetro.
           Pasamos '&adc_raw_value'. La función buscará esa variable en la memoria 
           y modificará su valor directamente con el resultado de la lectura física.
        */
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw_value));

        /* 
           Como configuramos el ADC a 12 bits (0-4095) y el PWM a 12 bits (0-4095),
           los rangos son matemáticamente idénticos. 
           Pasamos el valor del potenciómetro directamente al brillo del LED.
        */
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, adc_raw_value));
        
        // Esta función aplica físicamente los cambios de brillo que configuramos en la línea anterior.
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

        /* 
           printf imprime texto en la terminal serial. 
           El modificador '%d' es un marcador de posición para números enteros (decimales). 
           Los valores de 'adc_raw_value' sustituyen a los '%d' en orden.
        */
        printf("Lectura ADC: %d | Duty Cycle PWM: %d\n", adc_raw_value, adc_raw_value);

        /* 
           En sistemas operativos como FreeRTOS, nunca debes dejar un bucle infinito vacío o sin pausas, 
           porque bloquearías el procesador impidiendo que haga otras tareas del sistema.
           'vTaskDelay' pausa esta tarea. 'pdMS_TO_TICKS(50)' convierte 50 milisegundos a los 
           "ticks" internos que entiende el sistema operativo.
        */
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}
