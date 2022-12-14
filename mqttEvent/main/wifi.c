//Bibliotecas
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi.h"
#include "conectado.h"
//------------------------------------------------------------------------------------
//Declaracoes e definicoes
#define EXAMPLE_ESP_WIFI_SSID      "WIFI GIOVANI"
#define EXAMPLE_ESP_WIFI_PASS      "GiovaniSenha"
//#define EXAMPLE_ESP_MAXIMUM_RETRY  10
//extern EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "wifi station";
//static int s_retry_num = 0;
//------------------------------------------------------------------------------------

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                xEventGroupClearBits(IM_event_group, INTERNET_DISPONIVEL_BIT);
                esp_wifi_connect();
                /*
                if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
                {
                    esp_wifi_connect();
                    s_retry_num++;
                    ESP_LOGI(TAG, "retry to connect to the AP");
                }
                else 
                {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                }
                ESP_LOGI(TAG,"connect to the AP fail");
                */    
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        //s_retry_num = 0;
        xEventGroupSetBits(IM_event_group, INTERNET_DISPONIVEL_BIT);
    }    
}


void wifi_start(void)
{

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

    wifi_config_t wifi_config =
    {
        .sta = 
        {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = 
            {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    //-----------------------------------------------------------------------------------------
    //Aguarda  WIFI_CONNECTED_BIT ou WIFI_FAIL_BIT

    //------------------------------------------------------------------------------------
    //Para colocar o wifi em modo de economia
    #if CONFIG_PM_ENABLE
        /*
        * Coloca o WiFi em modo de economia de energia. (Essa op????o ?? obrigat??ria quando o gerenciamento de energia ??
        * habilitado. Caso contr??rio, a CPU sempre estar?? no clock m??ximo.)
        * As op????es s??o WIFI_PS_NONE (sem economia de energia), WIFI_PS_MIN_MODEM (economia m??nima), WIFI_PS_MAX_MODEM
        * (economia m??xima). O modo WIFI_PS_MAX_MODEM n??o traz uma economia muito maior e depende do par??metro
        * "listen_interval" da struct "wifi_sta_config_t".
        */
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
    #endif // CONFIG_PM_ENABLE
}