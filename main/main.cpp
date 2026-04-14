#include <RadioLib.h>
#include "hal/ESP32S3Hal/Esp32S3Hal.hpp" 
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h" 
#include "nvs_flash.h"

// --- Pines Heltec V3 ---
#define RADIO_NSS   (8)  
#define RADIO_IRQ   (14) 
#define RADIO_RST   (12) 
#define RADIO_GPIO  (13) 
#define RADIO_SCK   (9)
#define RADIO_MISO  (11)
#define RADIO_MOSI  (10)

#define UART_NUM         UART_NUM_1
#define TXD_PIN          33  // Conectar al RX del sensor (GPIO 18)
#define RXD_PIN          35  // Conectar al TX del sensor (GPIO 17)
#define UART_BAUD_RATE   115200

static const char *TAG = "HELTEC_MASTER";

uint8_t devEui[]  = {0x34, 0xCD, 0xB0, 0xFF, 0xFE, 0x3D, 0x82, 0x60};
uint8_t joinEui[] = {0x34, 0xCD, 0xB0, 0xFF, 0xFE, 0x3D, 0x82, 0x60}; 
uint8_t appKey[]  = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00};

uint64_t arrayTo64(uint8_t* arr) {
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) value |= ((uint64_t)arr[i] << (56 - (i * 8)));
    return value;
}

void sync_time_with_compiler() {
    struct tm tm;
    char month_str[4];
    int day, year, hour, minute, second;
    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    sscanf(__DATE__, "%3s %d %d", month_str, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);
    tm.tm_year = year - 1900;
    tm.tm_mon = (strstr(months, month_str) - months) / 3;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;
    struct timeval tv = { .tv_sec = mktime(&tm), .tv_usec = 0 };
    settimeofday(&tv, NULL);
}

void get_timestamp(char *buf) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buf, 20, "%H:%M:%S", &timeinfo);
}

void init_uart(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, 1024 * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

extern "C" void app_main(void) {
    char t_buf[20];
    nvs_flash_init();
    sync_time_with_compiler();
    init_uart();

    Esp32S3Hal hal(RADIO_SCK, RADIO_MISO, RADIO_MOSI);
    Module mod(&hal, RADIO_NSS, RADIO_IRQ, RADIO_RST, RADIO_GPIO);
    SX1262 radio(&mod);

    radio.begin();
    radio.setTCXO(1.8, 500); 
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    radio.setDio2AsRfSwitch(true);    
    radio.setOutputPower(14); 
    radio.setRxBoostedGainMode(false); 

    LoRaWANNode node(&radio, &EU868);
    node.beginOTAA(arrayTo64(joinEui), arrayTo64(devEui), appKey, appKey);
    
    bool unido = false;
    while(!unido) {
        get_timestamp(t_buf);
        ESP_LOGI(TAG, "[%s] Intentando Join...", t_buf);
        int16_t state = node.activateOTAA();
        if (state == RADIOLIB_LORAWAN_NEW_SESSION || state == RADIOLIB_ERR_NONE) {
            ESP_LOGI(TAG, "¡CONECTADO!");
            unido = true;
        } else {
            ESP_LOGE(TAG, "Fallo. Reiniciando para intento limpio...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            esp_restart();
        }
    }

    TickType_t ultimo_envio = 0;
    while (true) {
        if ((xTaskGetTickCount() - ultimo_envio) >= pdMS_TO_TICKS(30000) || ultimo_envio == 0) {
            ESP_LOGI(TAG, "Solicitando dato al sensor...");
            const char* cmd = "G";
            uart_write_bytes(UART_NUM, cmd, 1);

            uint8_t buffer[128];
            int rxBytes = uart_read_bytes(UART_NUM, buffer, sizeof(buffer)-1, pdMS_TO_TICKS(1500));

            if (rxBytes > 0) {
                buffer[rxBytes] = '\0';
                get_timestamp(t_buf);
                ESP_LOGI(TAG, "[%s] Enviando LoRa: %s", t_buf, (char*)buffer);
                node.sendReceive(buffer, rxBytes);
            } else {
                ESP_LOGW(TAG, "Sin respuesta del sensor.");
            }
            ultimo_envio = xTaskGetTickCount();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}