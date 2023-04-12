#include <string.h>
#include <stdlib.h>
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "wifi.h"
#include "mqtt.h"
#include "conectado.h"
#include "dht.h"
#include "mw.h"
#include "mdf_common.h"
#include "mwifi.h"


#define SENSOR_TYPE DHT_TYPE_AM2301
const int DHT_DATA_GPIO = 17;
float temperatura, umidade;




/**
 * @brief Timed printing system information
 */
static void print_system_info_timercb(void *timer)
{
    uint8_t primary                 = 0;
    wifi_second_chan_t second       = 0;
    mesh_addr_t parent_bssid        = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    wifi_sta_list_t wifi_sta_list   = {0x0};

    while(1){
      esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
      esp_wifi_ap_get_sta_list(&wifi_sta_list);
      esp_wifi_get_channel(&primary, &second);
      esp_mesh_get_parent_bssid(&parent_bssid);

      MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
      ", parent rssi: %d, node num: %d, free heap: %u", primary,esp_mesh_get_layer(), MAC2STR(sta_mac),
      MAC2STR(parent_bssid.addr),mwifi_get_parent_rssi(), esp_mesh_get_total_node_num(), esp_get_free_heap_size());

    for (int i = 0; i < wifi_sta_list.num; i++) {
        MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }

  #ifdef MEMORY_DEBUG

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    mdf_mem_print_heap();
    mdf_mem_print_record();
    mdf_mem_print_task();
  #endif /**< MEMORY_DEBUG */

  vTaskDelay(10000 / portTICK_RATE_MS);
  }
}

static void root_task(void *arg)
{
    mdf_err_t ret                    = MDF_OK;
    char *data                       = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size                      = MWIFI_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0};
    float temp, umi = 0;

    MDF_LOGI("Root is running");

    /*Para aguardar o mwifi estar comecar*/
    xEventGroupWaitBits(
    IM_event_group,               //EventGroup q vai esperar
    MWIFI_DISPONIVEL_BIT ,        //Bits q está esperando a mudanca
    pdFALSE,                      //Os bits n vao ser limpos dps de lidos      
    pdFALSE,                      //pdFALSE é igual a "or",ou seja, espera qualquer um dos dois para continuar  
    portMAX_DELAY                 //tempo para esperar os dois bits dps q o primeiro é ativado   
    );

    while(1)
    {
     
      //Tamanho da mensagem
      size = MWIFI_PAYLOAD_LEN;
      //memset faz o vetor de dados ficar vazio
      memset(data, 0, MWIFI_PAYLOAD_LEN);

      /**
      * @brief mwifi_root_read() le a rede mesh
      * 
      * @param src_addr Endereço do dispositivo que enviou os dados 
      * @param data_type Tipo de dados
      * @param data Vetor para armazenar os dados recebidos
      * @param size Tamanho do dado recebido
      * @param portMAX_DELAY Tempo limite de espera
      */
      ret = mwifi_root_read(src_addr, &data_type, data, &size, portMAX_DELAY);
      sscanf(data, " Temperatura: %f C e Umidade: %f%%", &temp, &umi);
      MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_root_read", mdf_err_to_name(ret));
      MDF_LOGI("Temperatura: %.1f C, Umidade: %.1f%%", temp, umi);
      //MDF_LOGI("Root receive, addr: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
      vTaskDelay(1000 / portTICK_RATE_MS);
      /*
      size = sprintf(data, "Hello node!");
      ret = mwifi_root_write(src_addr, 1, &data_type, data, size, true);
      //MDF_ERROR_CONTINUE(ret != MDF_OK, "mwifi_root_recv, ret: %x", ret);
      MDF_LOGI("Root send, addr: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
      
      */
    }
    


}


void app_main(void)
{      
  /* Inicializa NVS */
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  /* Para o ESP32 entrar em modo de economia de energia */
  #if CONFIG_PM_ENABLE
    /*
    * Configure a escala de frequência dinâmica:
    * Normalmente, "max_freq_mhz" é igual a freq. padrão da CPU. Enquanto que
    * "min_freq_mhz" é um múltiplo inteiro da freq. XTAL. Contudo, 10 MHz é a
    * frequência mais baixa em que o clock padrão REF_TICK de 1 MHz pode ser gerado.
    */
    esp_pm_config_esp32_t pm_config = 
    {
      .max_freq_mhz = 80,
      .min_freq_mhz = 10,
      #if CONFIG_FREERTOS_USE_TICKLESS_IDLE
      /* O (light sleep) automático é ativado se a opção "tickless idle" estiver habilitada. */
      .light_sleep_enable = true
      #endif
    };
    ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
  #endif // CONFIG_PM_ENABLE
  
  /* Cria o evento IM_event_group para utilizar wifi e mqtt */
  criarEvento();

  /* Limpa os bits que são utilizados que indicam se internet e o mqtt estao disponiveis, assim garantem que são 0
  antes de serem utlizados */
  xEventGroupClearBits(IM_event_group, INTERNET_DISPONIVEL_BIT);
  xEventGroupClearBits(IM_event_group, MQTT_DISPONIVEL_BIT);

  /* Inicilização do wifi e mqtt */
  wifi_start();
  mqtt_start();
  /* Inicialização mwfi*/
  mw_start();
  
  /* Cria a task dht_test */
  /**
   * @brief Data transfer between wifi mesh devices
   */
  xTaskCreate(root_task, "root_task", 4 * 1024, NULL, 5, NULL);
    
  TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS,true, NULL, print_system_info_timercb);
  xTimerStart(timer, 0);

  
  /* Para evitar watch dog*/
  while(1)
  {
    vTaskDelay(pdMS_TO_TICKS(2000));
    

    
  }
  
}

