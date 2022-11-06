//Bibliotecas
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
//#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
//#include "protocol_examples_common.h"

#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"
#include "mqtt.h"
#include "conectado.h"
//-----------------------------------------------------------------------------------------
//Definicoes e declaracoes
static const char *TAG = "MQTT";
esp_mqtt_client_handle_t client;
//-----------------------------------------------------------------------------------------
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    //int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        xEventGroupSetBits(IM_event_group, MQTT_DISPONIVEL_BIT);
        /*
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        */
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        xEventGroupClearBits(IM_event_group, MQTT_DISPONIVEL_BIT);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
        //ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED");
        //ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
        //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        //printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id");
        //ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }

}



void mqtt_start(void)
{
    // Espera o wifi ser conectado
    xEventGroupWaitBits(
        IM_event_group,               //EventGroup q vai esperar
        INTERNET_DISPONIVEL_BIT ,     //Bits q está esperando a mudanca
        pdFALSE,                      //Os bits n vao ser limpos dps de lidos      
        pdFALSE,                      //pdFALSE é igual a "or",ou seja, espera qualquer um dos dois para continuar  
        portMAX_DELAY                 //tempo para esperar os dois bits dps q o primeiro é ativado   
        );
    //-----------------------------------------------------------------------------------------    
    // Configura o cliente mqtt:
    esp_mqtt_client_config_t mqtt_cfg = 
    {
        .uri = "mqtt://broker.emqx.io",
    };
    //-----------------------------------------------------------------------------------------
    // Registra o cliente configurado como um identificador de evento 
    
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client); //(Identificador, tipos de eventos(qualquer um), identificador desse evento, argumento passado ) 
    esp_mqtt_client_start(client);                                                        //Inicializa o cliente configurado

}

void publicar(void)
{
    //Aguarda MQTT estar disponivel
    xEventGroupWaitBits(
       IM_event_group,               //EventGroup q vai esperar
       MQTT_DISPONIVEL_BIT ,         //Bits q está esperando a mudanca
       pdFALSE,                      //Os bits n vao ser limpos dps de lidos      
       pdFALSE,                      //pdFALSE é igual a "or",ou seja, espera qualquer um dos dois para continuar  
       portMAX_DELAY                 //tempo para esperar os dois bits dps q o primeiro é ativado   
       );
    //-----------------------------------------------------------------------------------------
    //Publica a msg      
    int msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);           //(client,topic,data,len(se deixar 0 ele calcula o tamanho da string),qos,retain)
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);                                                                                                    
}
