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
#include "conectado.h"
#include "dht.h"

#include "mdf_common.h"
#include "mwifi.h"
#include "wifi.h"
#include "mwcomwifi.h"

static const char *TAG = "get_started";

/**
 * @brief Realiza a leitura dos dados recebidos no no filho e printa o que recebeu
 * 
 * @param arg Nenhum argumento e passado
 */
static void node_read_task(void *arg)
{
    /*Inicializa as variaveis utilizadas*/
    mdf_err_t ret = MDF_OK;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    MDF_LOGI("No filho esta pronto para ler");

while(1) {
    /*Verifica se mwifi esta conectado*/
    if(mwifi_is_connected()) {
        size = MWIFI_PAYLOAD_LEN;                                                                           //Tamanho maximo do dado recebido
        memset(data, 0, MWIFI_PAYLOAD_LEN);                                                                 //Limpa o vetor de dado
        ret = mwifi_read(src_addr, &data_type, data, &size, 1000 / portTICK_RATE_MS);                       //Leitura dos dados recebidos pelo no filho
        if(ret == MDF_OK) {                                                                                 //Verica se a operacao de leitura ocorreu bem
            MDF_LOGI("Node receive, addr: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);  //Se sim, mostra oq recebeu
        }
        else if(ret != MDF_ERR_TIMEOUT) {
            MDF_LOGE("mwifi_read, ret: %x", ret);                                                           //Se nao, indica erro
        }
    }
    else {                                                                                                  //Se mwifi n estiver conectado
        MDF_LOGI("MWIFI ainda nao esta conectado");                                                         //indica q mwifi nao estao conectado                                                  
    }
    vTaskDelay(1000 / portTICK_RATE_MS);                                                                    //Espera 1000ms
}
    
}

/**
 * @brief Printa informacoes do sistema de forma periodica
 * 
 */
static void print_system_info_timercb(void *timer)
{
    uint8_t primary                 = 0;
    wifi_second_chan_t second       = 0;
    mesh_addr_t parent_bssid        = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    wifi_sta_list_t wifi_sta_list   = {0x0};

    while(1){
        esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);                                                         //Armazena o endereco MAC do dispositivo
        esp_wifi_ap_get_sta_list(&wifi_sta_list);                                                           //Obtem a lista de estações (STA) conectadas a um ponto de acesso (AP) Wi-Fi.
        esp_wifi_get_channel(&primary, &second);                                                            //obtem o canal de operação atual da interface Wi-Fi
        esp_mesh_get_parent_bssid(&parent_bssid);                                                           //Armazena o endereco MAC do no pai

        /*Printa as informacoes*/
        MDF_LOGI("Infomacoes do sistema, canal: %d, camada: %d, proprio mac: " MACSTR ", endereco pai: " MACSTR
                ", parent rssi: %d, numero de nos: %d,  memória heap disponível: %u", primary,
                esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
                mwifi_get_parent_rssi(), esp_mesh_get_total_node_num(), esp_get_free_heap_size());

        vTaskDelay(5000 / portTICK_RATE_MS);                                                                //Aguarda 5000ms
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
  
  /*Inicia o wifi no modo sta*/
  wifi_start();
  /*Inicia a rede mesh e configura o dispositivo como no filho*/
  mw_start();

  /**
   * @brief xTimerCreate() cria um temporizador em um sistema operacional FreeRTOS e retorna um identificador para o temporizador criado
   * 
   * @param pcTimerName Nome do temporizador.
   * @param xTimerPeriod Período de tempo em milissegundos entre as chamadas da função de retorno.
   * @param uxAutoReload Indica se deve ser iniciado automaticamente ou nao. True indica que deve ser criado automaticamente.
   * @param pvTimerID Identificador para o timer criado.
   * @param pxCallbackFunction A funcao que vai ser chamada.
   */
  TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS,true, NULL, print_system_info_timercb);
  xTimerStart(timer, 0);                                                                                    //Inicializa o timer criado
  /*Cria a task node_read_tas*/
  xTaskCreate(node_read_task, "node_read_task", 4 * 1024,NULL, 5, NULL);

  /* Para evitar watch dog*/
  while(1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  
}

