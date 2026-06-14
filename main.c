/*---------- Conclusion del Equipo ----------*/
/*

Integrantes:
-Pablo Mansilla Hernández

En esta practica pude comprender el funcionamiento basico de FreeRTOS, junto con funciones 
como lo son la creacion de tareas, la asignación de prioridades, los retardos, las 
suspenciones, y la reanudación de tareas. Ademas de esto, pude ver como es que dentro de un sistema 
embebido, pueden haber varias tareas trabajando de forma paralela

*/

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"

#define LED_PIN     GPIO_NUM_2
#define BUTTON_PIN  GPIO_NUM_4
#define ADC_CHANNEL ADC_CHANNEL_0  // GPIO36 = VP

volatile bool g_sensorActivo  = false;
volatile bool g_botonPres     = false;

TaskHandle_t hLedRapido = NULL;
TaskHandle_t hLedLento  = NULL;
TaskHandle_t hMonitor   = NULL;

adc_oneshot_unit_handle_t adc_handle = NULL;

// Tarea LED rápido
void vTaskLedRapido(void *pv)
{
    bool s = false; // Estado actual del LED
    TickType_t t = xTaskGetTickCount(); // Guarda el tiempo inicial
    while (1) {
        s = !s; // Invierte el estado actual del LED
        gpio_set_level(LED_PIN, s); // Actualiza la salida
        vTaskDelayUntil(&t, pdMS_TO_TICKS(100)); // Hace que el LED parpade cada 100 ms
    }
}

// Tarea LED lento
void vTaskLedLento(void *pv)
{
    bool s = false; // Estado actual del LED
    TickType_t t = xTaskGetTickCount();
    while (1) {
        s = !s;
        gpio_set_level(LED_PIN, s);
        vTaskDelayUntil(&t, pdMS_TO_TICKS(500)); // Hace que el LED parpade cada 500 ms
    }
}

// Tarea Sensor
void vTaskSensor(void *pv) // Detecta el boton, luego lee el ADC, y le da los datos al monitor
{
    int lastBtn = 1; // Guarda el estado previo del boton
    while (1) {
        if (g_sensorActivo) { // Hace que solo se revise el estado del boton cuando lo permite el monitor
            int btn = gpio_get_level(BUTTON_PIN);
            if (btn == 0 && lastBtn == 1) { // Detecta cuando el boton se presiona
                // Leer y imprimir voltaje
                int raw = 0;
                adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw); // Obtiene la lectura del ADC
                float voltaje = (raw * 3.3f) / 4095.0f; // Convierte el valor el voltaje
                printf("[SENS] ADC Raw: %d | Voltaje: %.2f V\n", raw, voltaje); // Imprime el voltaje

                g_botonPres = true;
                vTaskResume(hMonitor);
            }
            lastBtn = btn;
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Revisa el estado del boton cada 50 ms
    }
}

// Tarea Monitor
void vTaskMonitor(void *pv) // Controla la secuencia principal del sistema
{
    while (1) {
        // Etapa 1: LED rápido, 5 segundos
        printf("[LED_R] Modo RAPIDO\n");
        g_botonPres = false;
        vTaskResume(hLedRapido); // Activa la tarea de LED rapido
        vTaskDelay(pdMS_TO_TICKS(5000)); // Mantiene la tarea durante 5 segundos
        vTaskSuspend(hLedRapido); // Detiene la tarea
        gpio_set_level(LED_PIN, 0);

        // Etapa 2: LED lento, 5 segundos
        printf("[LED_L] Modo LENTO\n");
        vTaskResume(hLedLento);
        vTaskDelay(pdMS_TO_TICKS(5000));
        vTaskSuspend(hLedLento);
        gpio_set_level(LED_PIN, 0);

        // Etapa 3: Idle, máximo 5 segundos
        printf("[IDLE] Modo IDLE - esperando boton...\n"); // Espera un evento del botón o un timeout
        g_sensorActivo = true; // Habilita la detección del botón
        g_botonPres    = false;

        // Se suspende a sí mismo — vTaskSensor lo despierta si hay botón,
        // o si el timeout lo despierta tras 5 segundos
        vTaskSuspend(hMonitor);

        // Se deshabilita el sensor luego del timeout
        g_sensorActivo = false;

        if (g_botonPres) {
            printf("[MON] Boton detectado - reiniciando ciclo\n");
        } else {
            printf("[MON] Timeout - reiniciando ciclo\n");
        }
        // Regresa automáticamente a la Etapa 1 de LED rapido
    }
}

// Tarea Timeout
void vTaskTimeout(void *pv) // Despierta al Monitor después de 5 segundos en modo IDLE
{
    while (1) {
        if (g_sensorActivo) { // Solo funciona cuando el monitor está esperando
            vTaskDelay(pdMS_TO_TICKS(5000)); // Cuenta 5 segundos
            if (g_sensorActivo) {
                vTaskResume(hMonitor); // Se despierta al Monitor
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Idle Hook
void vApplicationIdleHook(void)
{
    static int64_t lastPrint = 0; // Guarda el último instante de impresión
    if (g_sensorActivo) {
        int64_t now = esp_timer_get_time() / 1000;
        if (now - lastPrint >= 2000) {
            printf("[IDLE] CPU libre - esperando evento de boton\n");
            lastPrint = now;
        }
    }
}

void app_main(void)
{
    // LED
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .intr_type    = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en   = 0,
    };
    gpio_config(&io);

    // Botón con pull-up
    io.pin_bit_mask = (1ULL << BUTTON_PIN);
    io.mode         = GPIO_MODE_INPUT;
    io.pull_up_en   = 1;
    io.pull_down_en = 0;
    gpio_config(&io);

    // ADC
    adc_oneshot_unit_init_cfg_t adc_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t ch_cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten    = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &ch_cfg));

    printf("=== Practica 1 - FreeRTOS ESP32 ===\n");

    xTaskCreate(vTaskLedRapido, "LED_R",   2048, NULL, 1, &hLedRapido);
    xTaskCreate(vTaskLedLento,  "LED_L",   2048, NULL, 2, &hLedLento);
    xTaskCreate(vTaskSensor,    "SENS",    2048, NULL, 3, NULL);
    xTaskCreate(vTaskMonitor,   "MON",     3072, NULL, 4, &hMonitor);
    xTaskCreate(vTaskTimeout,   "TIMEOUT", 2048, NULL, 3, NULL);

    vTaskSuspend(hLedRapido);
    vTaskSuspend(hLedLento);
}
