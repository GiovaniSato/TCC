//Bibliotecas
#include <string.h>
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
//-----------------------------------------------------------------------------------------
//Declaracoes e definicoes
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
extern EventGroupHandle_t s_wifi_event_group ;
static const char *TAG = "Teste conectado";
//-----------------------------------------------------------------------------------------
void app_main(void)
{      
  // Inicializa NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
//-----------------------------------------------------------------------------------------
  // Para esp 32 entrar em modo de economia
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
  //-----------------------------------------------------------------------------------------
  // Cria o evento que vai ser utilizado em wifi e mqtt
  criarEvento();
  //-----------------------------------------------------------------------------------------
  // Inicializa wifi e o MQTT
  wifi_start();
  // Espera o wifi ser conectado
  xEventGroupWaitBits(
          s_wifi_event_group,      //EventGroup q vai esperar
          WIFI_CONNECTED_BIT ,     //Bits q está esperando a mudanca
          pdFALSE,                 //Os bits n vao ser limpos dps de lidos      
          pdFALSE,                 //pdFALSE é igual a "or",ou seja, espera qualquer um dos dois para continuar  
          portMAX_DELAY            //tempo para esperar os dois bits dps q o primeiro é ativado   
        );
       
  vTaskDelay(pdMS_TO_TICKS(2000));   // Para n printar logo de uma vez
  ESP_LOGI(TAG, "Conectar MQTT");    // Teste 
  // ...
  
}
