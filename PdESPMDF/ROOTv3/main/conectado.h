#ifndef CONECTADO_H
#define CONECTADO_H
void criarEvento(void);
extern EventGroupHandle_t IM_event_group;
#define INTERNET_DISPONIVEL_BIT BIT0
#define MQTT_DISPONIVEL_BIT     BIT1
#define MWIFI_DISPONIVEL_BIT     BIT1
#define ENVIARMW_DISPONIVEL_BIT     BIT2
#endif //CONECTADO_H