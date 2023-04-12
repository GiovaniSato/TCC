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

static const char *TAG = "MW_STARTED";

/**
 * @brief Envia dados (temperatura e umidade) para um nó filho de forma periódica. Seu período depende do bit ENVIARMW_DISPONIVEL_BIT
 * que é coloca em high na task root_task() após a leitura desses dados. 
 * 
 * @param arg Nenhum argumento é passado
 */
void root_enviar_task(void *arg)
{
  /*Inicializacao das variaveis utilizadas*/
  mdf_err_t ret                    = MDF_OK;
  char *data                       = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
  size_t size                      = MWIFI_PAYLOAD_LEN;
  uint8_t dest_addr[] = {0xc0, 0x49, 0xef, 0xe4, 0x97, 0xd8};
  mwifi_data_type_t data_type      = {0};
  
  MDF_LOGI("Root enviar está esperando");

  while(1)
  {
    /*Aguarda receber os dados para enviar para enviar para o no filho*/
    xEventGroupWaitBits(
    IM_event_group,                       //EventGroup q vai esperar
    ENVIARMW_DISPONIVEL_BIT ,             //Bits q está esperando a mudanca
    pdTRUE,                               //Os bits vao ser limpos dps de lidos      
    pdFALSE,                              //pdFALSE é igual a "or",ou seja, espera qualquer um dos dois para continuar  
    portMAX_DELAY                         //tempo para esperar os dois bits dps q o primeiro é ativado   
    );
  
    size = MWIFI_PAYLOAD_LEN;             //Tamanho da mensagem
    memset(data, 0, MWIFI_PAYLOAD_LEN);   //memset faz o vetor de dados ficar vazio

    size = sprintf(data, "Hello node!");

    /**
    * @brief mwifi_root_write() envia dados para um ou mais nós filhos
    * 
    * @param dest_addr Endereço MAC do dispositivo que receberá os dados.  
    * @param dest_addrs_num Numero de destinos, nesse caso é 1
    * @param data_type Tipo de dado
    * @param data Vetor para armazenar os dados recebidos
    * @param size Tamanho do dado a ser enviado
    * @param block Se "true", a função espera até a confirmacao de recebimento seja recebida antes de retornar. 
    * Se "false", a função retorna imediatamente sem esperar pela confirmação.
    */
    ret = mwifi_root_write(dest_addr, 1, &data_type, data, size, false);
    /*Verifica se ocorreu erro na operacao de mwifi_root_write(). MDF_OK indica que foi bem sucedida, se n for somente registra o erro*/
    MDF_ERROR_CONTINUE(ret != MDF_OK, "mwifi_root_recv, ret: %x", ret);
    MDF_LOGI("PAI enviou dado, addr: " MACSTR ", size: %d, data: %s", MAC2STR(dest_addr), size, data);
  }  
}

static void root_task(void *arg)
{
  /*Inicializacao das variaveis utilizadas*/
  mdf_err_t ret                    = MDF_OK;
  char *data                       = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
  size_t size                      = MWIFI_PAYLOAD_LEN;
  uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
  mwifi_data_type_t data_type      = {0};
  float temp, umi                  = 0;

  MDF_LOGI("Root ler está esperadno");

  /*Para aguardar o mwifi estar comecar*/
  xEventGroupWaitBits(
  IM_event_group,                         //EventGroup q vai esperar
  MWIFI_DISPONIVEL_BIT ,                  //Bits q está esperando a mudanca
  pdFALSE,                                //Os bits n vao ser limpos dps de lidos      
  pdFALSE,                                //pdFALSE é igual a "or",ou seja, espera qualquer um dos dois para continuar  
  portMAX_DELAY                           //tempo para esperar os dois bits dps q o primeiro é ativado   
  );

  while(1)
  {
    size = MWIFI_PAYLOAD_LEN;             //Tamanho da mensagem
    memset(data, 0, MWIFI_PAYLOAD_LEN);   //memset faz o vetor de dados ficar vazio

    /**
    * @brief mwifi_root_read() recebe dados enviados pelos nós filhos 
    * 
    * @param src_addr Endereço do dispositivo que enviou os dados 
    * @param data_type Tipo de dados
    * @param data Vetor para armazenar os dados recebidos
    * @param size Tamanho do dado recebido
    * @param portMAX_DELAY Tempo limite de espera
    */
    ret = mwifi_root_read(src_addr, &data_type, data, &size, portMAX_DELAY);
    /*Verifica se ocorreu erro na operacao de mwifi_root_read. MDF_OK indica que foi bem sucedida, se n for somente registra o erro*/
    MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_root_read", mdf_err_to_name(ret));    
    /*Extrai a temperatura e umidade dos dados recebidos, os armazena em temp e umi respectivamente*/
    sscanf(data, " Temperatura: %f C e Umidade: %f%%", &temp, &umi);

    /**
    * @brief publicar() e uma funcao de mqtt.h. Publica os dados no mqtt
    * 
    * @param topic Topico
    * @param qos O nivel de qualidade de servico da mensagem. Pode ser 0, 1 ou 2.
    * @param retain Se a mensagem deve ser retida ou n. Pode ser 0 (n retida ) ou 1 (retida).
    * @param dado O dado que e enviado.
    */
    publicar("giovani/teste/0", 0, 0, "Temperatura : %0.1fC", temp);
    publicar("giovani/teste/0", 0, 0, "Umidade : %0.1f%%", umi);

    /*Indica que pode enviar os dados para o no filho*/
    xEventGroupSetBits(IM_event_group, ENVIARMW_DISPONIVEL_BIT);
    
    vTaskDelay(2000 / portTICK_RATE_MS);
    
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

  /*Inicialização MWIFI*/
  mw_start();
  /* Cria a task do root*/
  xTaskCreate(root_task, "root_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
  xTaskCreate(root_enviar_task, "root_enviar_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

  /**/
  
  /* Para evitar watch dog*/
  while(1)
  {
    vTaskDelay(pdMS_TO_TICKS(2000));
    
  }
  
}

