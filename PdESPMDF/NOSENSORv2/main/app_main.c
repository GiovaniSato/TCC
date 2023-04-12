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
//#include "mqtt.h"
#include "conectado.h"
#include "dht.h"
#include "mdf_common.h"
#include "mwifi.h"

#include "mwcomwifi.h"

#define SENSOR_TYPE DHT_TYPE_AM2301
const int DHT_DATA_GPIO = 17;
float temperatura, umidade;

/**
 * @brief Lê o sensor e envia os dados para o no pai. Essa Task que repete-se a cada 10000 ms. 
 * 
 * @param pvParameters Nao e passado nenhum paramentro
 */

void dht_test(void *pvParameters)
{
  while(1)
  {
    if (dht_read_float_data(SENSOR_TYPE, DHT_DATA_GPIO, &umidade, &temperatura) == ESP_OK)
    { 
      mandar_no(temperatura, umidade);
    }
    else
    {
      printf("Could not read data from sensor\n");
    }
    vTaskDelay(pdMS_TO_TICKS(10000));
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
  
  /* Cria a task dht_test */
  xTaskCreate(dht_test, "dht_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

  
  /* Para evitar watch dog*/
  while(1)
  {
    vTaskDelay(pdMS_TO_TICKS(2000));
    

    
  }
  
}

