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
//#include "wifi.h"
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
 * @brief Lê o sensor e publica os dados no broker mqtt
 * 
 * Task que repete-se a cada 10000 ms. Publica a leitura de temperatura e umidade do sensor
 * 
 */
void dht_test(void *pvParameters)
{
  while(1)
  {
    if (dht_read_float_data(SENSOR_TYPE, DHT_DATA_GPIO, &umidade, &temperatura) == ESP_OK)
    { 
      //publicar("giovani/teste/0", 0, 0, "Temperatura : %0.1fC", temperatura);
      //publicar("giovani/teste/0", 0, 0, "Umidade : %0.1f%%", umidade);
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

  mwcomwifi_start();
  
  /* Cria a task dht_test */
  xTaskCreate(dht_test, "dht_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

  
  /* Para evitar watch dog*/
  while(1)
  {
    vTaskDelay(pdMS_TO_TICKS(2000));
    

    
  }
  
}

