
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <stdio.h>
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "driver/gpio.h"    
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs.h"


#define EXAMPLE_ESP_WIFI_SSID      "your_ssid"
#define EXAMPLE_ESP_WIFI_PASS      "your_pass"
#define LED_WIFI_GPIO 2  
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define EX_UART_NUM UART_NUM_2
#define PATTERN_CHR_NUM    (3)       
#define BUF_SIZE (1024)
#define UART_BUF_SIZE 128
#define RD_BUF_SIZE (BUF_SIZE)
#define RELAY1_GPIO  14
#define RELAY2_GPIO  27
#define RELAY3_GPIO  26
#define RELAY4_GPIO  25

#define WIFI_MAXIMUM_RETRY 5
static int s_retry_num = 0;
#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif


static EventGroupHandle_t s_wifi_event_group;
static SemaphoreHandle_t xMutex_http;
static QueueHandle_t uart0_queue;
QueueHandle_t uart_queue_data;
static const char *TAG_UART = "uart_events";
static const char *TAG_WIFI = "wifi station";
static const char *TAGFB = "FIREBASE_POST";
static const char *TAGFB_0 = "FIREBASE_GET";
static const char *TAG = "WWCR";
volatile bool wifi_connected = false;
//Config Firebase
extern const uint8_t firebase_ca_pem_start[] asm("_binary_firebase_ca_pem_start");
extern const uint8_t firebase_ca_pem_end[]   asm("_binary_firebase_ca_pem_end");

#define FIREBASE_HOST "https://your_host.com"
#define FIREBASE_PATH "/sensor.json"  
#define FIREBASE_PATH_HUMIDITY "/sensor/humidity.json" 
#define FIREBASE_PATH_TEMPWATER "/sensor/tempDS18B20.json"
#define FIREBASE_PATH_TEMPAIR "/sensor/tempDHT.json" 
#define FIREBASE_PATH_TDS "/sensor/tds.json" 
#define FIREBASE_AUTH "your_auth" 
#define FIREBASE_PATH_LEVEL "/sensor/waterLevel.json" 
#define FIREBASE_PATH_NTU "/sensor/turbidity.json" 
#define FIREBASE_PATH_PH "/sensor/pH.json" 
static esp_http_client_handle_t client = NULL;

float temperature = 0;
float humidity = 0;
float TempWater = 0;
float TDS = 0;
float Level = 0;
float NTU = 0;
float pH = 0;


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_WIFI, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG_WIFI,"connect to the AP fail");
      
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_connected = true;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG_WIFI, "wifi_init_sta finished.");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_WIFI, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG_WIFI, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG_WIFI, "UNEXPECTED EVENT");
    }
}
void wifi_monitor_task(void *pvParameters)
{
    wifi_init_sta();

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                                pdTRUE,
                                                pdFALSE,
                                                portMAX_DELAY);

        if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG_WIFI, "Connected to Wi-Fi");
        } else if (bits & WIFI_FAIL_BIT) {
            ESP_LOGI(TAG_WIFI, "Failed to connect to Wi-Fi");
            vTaskDelay(pdMS_TO_TICKS(5000)); 
            esp_wifi_connect(); 
        }
    }
}
void led_wifi_task(void *pvParameter) {
    esp_rom_gpio_pad_select_gpio(LED_WIFI_GPIO);
    gpio_set_direction(LED_WIFI_GPIO, GPIO_MODE_OUTPUT);

    while (1) {
        if (wifi_connected) {
            gpio_set_level(LED_WIFI_GPIO, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(LED_WIFI_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            gpio_set_level(LED_WIFI_GPIO, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(LED_WIFI_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

static void vTask_uart(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    for(;;) {
        if(xQueueReceive(uart0_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            switch(event.type) {
                case UART_DATA:
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    int len = event.size;
                    if (len >= UART_BUF_SIZE) len = UART_BUF_SIZE - 1;
                    dtmp[len] = '\0';
                    xQueueSend(uart_queue_data, dtmp, portMAX_DELAY);                 
                    //ESP_LOGI(TAG_UART, "Received: %s", (char*)dtmp);
                    break;
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG_UART, "hw fifo overflow");
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG_UART, "ring buffer full");

                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                case UART_BREAK:
                    ESP_LOGI(TAG_UART, "uart rx break");
                    break;
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG_UART, "uart parity error");
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG_UART, "uart frame error");
                    break;
                default:
                    ESP_LOGI(TAG_UART, "uart event type: %d", event.type);
                    break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}
typedef struct {
    const char* keyword;
    const char* format;
    float* variable;
    const char* firebase_path;
} SensorData_t;

SensorData_t sensor_list[] = {
    { "TempWater:", "TempWater: %f", &TempWater, FIREBASE_PATH_TEMPWATER },
    { "DHT Temp:", "DHT Temp: %f", &temperature, FIREBASE_PATH_TEMPAIR },
    { "Humidity:", "Humidity: %f", &humidity, FIREBASE_PATH_HUMIDITY },
    { "TDS:", "TDS: %f", &TDS, FIREBASE_PATH_TDS },
    { "NTU:", "NTU: %f", &NTU, FIREBASE_PATH_NTU },
    { "Level:", "Level: %f", &Level, FIREBASE_PATH_LEVEL },
    { "pHValue: ", "pHValue: %f", &pH, FIREBASE_PATH_PH }
    };
void send_to_firebase(float TempWater, float temperature, float humidity, float TDS, float NTU, float Level, float pH) {
    char post_data[256];
    snprintf(post_data, sizeof(post_data),
                "{ \"tempDS18B20\": %.2f, \"tempDHT\": %.2f, \"humidity\": %.2f, \"tds\": %.2f, \"turbidity\": %.2f, \"waterLevel\": %.2f, \"pH\": %.2f }",
                TempWater, temperature, humidity, TDS, NTU, Level, pH);

    char url[256];
    snprintf(url, sizeof(url), "%s/sensor.json?auth=%s", FIREBASE_HOST, FIREBASE_AUTH);

    xSemaphoreTake(xMutex_http, portMAX_DELAY);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PUT,
        .cert_pem = (const char *)firebase_ca_pem_start,
        .timeout_ms = 10000,
        .keep_alive_enable = false,  
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAGFB, "Failed to initialize HTTP client");
        xSemaphoreGive(xMutex_http);
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    ESP_LOGI("DEBUG", "Free heap: %lu", (unsigned long)esp_get_free_heap_size());

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAGFB, "HTTPS POST Status = %d, content_length = %lld",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAGFB, "HTTP POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);  
    xSemaphoreGive(xMutex_http);
}

void vTask_post_data(void *pvParameters)
{
    const int SENSOR_COUNT = sizeof(sensor_list) / sizeof(sensor_list[0]);

    static bool sensor_flags[7] = { false, false, false, false, false, false, false };
    bool all_sensors_ready() {
        for (int i = 0; i < SENSOR_COUNT; i++) {
            if (!sensor_flags[i]) return false;
        }
        return true;
    }

    void reset_sensor_flags() {
        for (int i = 0; i < SENSOR_COUNT; i++) {
            sensor_flags[i] = false;
        }
    }

    for (;;) {
        uint8_t rx_buf[64];

        if (xQueueReceive(uart_queue_data, rx_buf, portMAX_DELAY)) {
            rx_buf[63] = '\0';  // Đảm bảo kết thúc chuỗi
            printf("Received: %s\n", (char *)rx_buf);

            for (int i = 0; i < SENSOR_COUNT; i++) {
                if (strstr((char *)rx_buf, sensor_list[i].keyword)) {
                    sscanf((char *)rx_buf, sensor_list[i].format, sensor_list[i].variable);
                    sensor_flags[i] = true;
                    printf("%s = %.2f\n", sensor_list[i].keyword, *(sensor_list[i].variable));

                    if (all_sensors_ready()) {
                        send_to_firebase(TempWater, temperature, humidity, TDS, NTU, Level, pH); 
                        reset_sensor_flags(); 
                    }
                    break;  
                }
            }
            
            if (strstr((char *)rx_buf, "relay1: 1")) {
                gpio_set_level(RELAY1_GPIO, 1);  
                ESP_LOGI(TAG, "Relay 1 ON");
            } else if (strstr((char *)rx_buf, "relay1: 0")) {
                gpio_set_level(RELAY1_GPIO, 0);
                ESP_LOGI(TAG, "Relay 1 OFF");
            }
            
            if (strstr((char *)rx_buf, "relay2: 1")) {
                gpio_set_level(RELAY2_GPIO, 1);  
                ESP_LOGI(TAG, "Relay 2 ON");
            } else if (strstr((char *)rx_buf, "relay2: 0")) {
                gpio_set_level(RELAY2_GPIO, 0); 
                ESP_LOGI(TAG, "Relay 2 OFF");
            }
            
            if (strstr((char *)rx_buf, "relay3: 1")) {
                gpio_set_level(RELAY3_GPIO, 1); 
                ESP_LOGI(TAG, "Relay 3 ON");
            } else if (strstr((char *)rx_buf, "relay3: 0")) {
                gpio_set_level(RELAY3_GPIO, 0); 
                ESP_LOGI(TAG, "Relay 3 OFF");
            }
            
            if (strstr((char *)rx_buf, "relay4: 1")) {
                gpio_set_level(RELAY4_GPIO, 1); 
                ESP_LOGI(TAG, "Relay 4 ON");
            } else if (strstr((char *)rx_buf, "relay4: 0")) {
                gpio_set_level(RELAY4_GPIO, 0);  
                ESP_LOGI(TAG, "Relay 4 OFF");
            }
          
        }
    }
}

void vTask_get_data(void *pvParameters)
{
    for (;;)
    {
        xSemaphoreTake(xMutex_http, portMAX_DELAY);

        esp_http_client_config_t config = {
            .url = "https://your_host.com/Control.json",
            .method = HTTP_METHOD_GET,
            .cert_pem = (const char *)firebase_ca_pem_start,
            .timeout_ms = 10000,
            .keep_alive_enable = false, 
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL)
        {
            ESP_LOGE(TAGFB_0, "Failed to init HTTP client");
            xSemaphoreGive(xMutex_http);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK)
        {
            int content_length = esp_http_client_fetch_headers(client);
            char *buffer = malloc(content_length + 1);
            if (buffer)
            {
                int read_len = esp_http_client_read_response(client, buffer, content_length);
                buffer[read_len] = '\0';
                ESP_LOGI(TAGFB_0, "Dữ liệu nhận được: %s", buffer);
                cJSON *json = cJSON_Parse(buffer);
                if (json != NULL)
                {
                    cJSON *mode = cJSON_GetObjectItem(json, "mode");
                    int state_relay1 = cJSON_GetObjectItem(json, "relay1")->valueint;
                    int state_relay2 = cJSON_GetObjectItem(json, "relay2")->valueint;
                    int state_relay3 = cJSON_GetObjectItem(json, "relay3")->valueint;
                    int state_relay4 = cJSON_GetObjectItem(json, "relay4")->valueint;
                    int threshold_tds = cJSON_GetObjectItem(json, "TDS")->valueint;
                    int threshold_ntu = cJSON_GetObjectItem(json, "turbidity")->valueint;

                    if (strcmp(mode->valuestring, "manual") == 0)
                    {
                        gpio_set_level(RELAY1_GPIO, state_relay1);
                        gpio_set_level(RELAY2_GPIO, state_relay2);
                        gpio_set_level(RELAY3_GPIO, state_relay3);
                        gpio_set_level(RELAY4_GPIO, state_relay4);
                    }
                    else if (strcmp(mode->valuestring, "auto") == 0)
                    {
                        gpio_set_level(RELAY1_GPIO, (NTU > threshold_ntu));
                        gpio_set_level(RELAY2_GPIO, (TDS > threshold_tds));
                        gpio_set_level(RELAY3_GPIO, (TDS > threshold_tds));
                        gpio_set_level(RELAY4_GPIO, state_relay4);
                    }

                    cJSON_Delete(json);
                }
                else
                {
                    ESP_LOGE(TAGFB_0, "Failed to parse JSON");
                }

                free(buffer);
            }
        }
        else
        {
            ESP_LOGE(TAGFB_0, "HTTP request thất bại: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);
        xSemaphoreGive(xMutex_http);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    gpio_reset_pin(RELAY1_GPIO);
    gpio_reset_pin(RELAY2_GPIO);
    gpio_reset_pin(RELAY3_GPIO);
    gpio_reset_pin(RELAY4_GPIO);
    gpio_set_direction(RELAY1_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RELAY2_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RELAY3_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RELAY4_GPIO, GPIO_MODE_OUTPUT);

    xMutex_http = xSemaphoreCreateMutex();
    if (xMutex_http == NULL) {
        printf("Failed to create HTTP mutex!\n");
        while(1); 
    }

    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);
    esp_log_level_set(TAG_UART, ESP_LOG_INFO);
    uart_set_pin(EX_UART_NUM, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_queue_data = xQueueCreate(10, UART_BUF_SIZE); 
    xTaskCreate(wifi_monitor_task, "wifi_monitor_task", 4096, NULL, 5, NULL);
    xTaskCreate(vTask_uart, "vTask_uart", 4096, NULL, 7, NULL);      
    xTaskCreate(vTask_post_data, "vTask_post_data", 4096, NULL, 6, NULL);  
    xTaskCreate(vTask_get_data, "vTask_get_data", 4096, NULL, 4, NULL);    
    xTaskCreate(led_wifi_task, "led_wifi_task", 2048, NULL, 5, NULL);
}
