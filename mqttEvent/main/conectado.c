//Bibliotecas
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
//-----------------------------------------------------------------------------------------
//Declaracoes e definicoes
EventGroupHandle_t s_wifi_event_group;
//-----------------------------------------------------------------------------------------
void criarEvento(void)
{
    //cria o evento
    s_wifi_event_group = xEventGroupCreate();
}