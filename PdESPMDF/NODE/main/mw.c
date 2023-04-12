#include "mw.h"
#include "mdf_common.h"
#include "mwifi.h"
#include "conectado.h"

static const char *TAG = "MW_STARTED";

/**
 * @brief Timed printing system information
 */
/**static void print_system_info_timercb(void *timer)
{
    uint8_t primary                 = 0;
    wifi_second_chan_t second       = 0;
    mesh_addr_t parent_bssid        = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    wifi_sta_list_t wifi_sta_list   = {0x0};

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    esp_wifi_get_channel(&primary, &second);
    esp_mesh_get_parent_bssid(&parent_bssid);

    MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
             ", parent rssi: %d, node num: %d, free heap: %u", primary,
             esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
             mwifi_get_parent_rssi(), esp_mesh_get_total_node_num(), esp_get_free_heap_size());

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
#endif< MEMORY_DEBUG */
//}




static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{

    MDF_LOGI("event_loop_cb, event: %d", event);

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");
            break;

        default:
            break;
    }

    return MDF_OK;
}


void mw_start(void)
{
    /* Configura o mwifi*/
    mwifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config   = {
        .channel   = 10,
        .mesh_id   = "123456",
        .mesh_type = MESH_NODE,
    };

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
    * @brief Inicializa a rede mesh.
    */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));

    /*
    xEventGroupWaitBits(
    IM_event_group,               //EventGroup q vai esperar
    INTERNET_DISPONIVEL_BIT ,     //Bits q está esperando a mudanca
    pdFALSE,                      //Os bits n vao ser limpos dps de lidos      
    pdFALSE,                      //pdFALSE é igual a "or",ou seja, espera qualquer um dos dois para continuar  
    portMAX_DELAY                 //tempo para esperar os dois bits dps q o primeiro é ativado   
    );
*/
    MDF_ERROR_ASSERT(mwifi_start());
    

}






