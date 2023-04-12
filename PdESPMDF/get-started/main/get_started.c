// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mdf_common.h"
#include "mwifi.h"
#include "mwcomwifi.h"
#include "conectado.h"

// #define MEMORY_DEBUG

static const char *TAG = "get_started";


static void node_read_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    MDF_LOGI("Note read task is running");

    while(1){

        if(mwifi_is_connected()){
            size = MWIFI_PAYLOAD_LEN;
            memset(data, 0, MWIFI_PAYLOAD_LEN);
            ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "mwifi_read, ret: %x", ret);
            MDF_LOGI("Node receive, addr: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
            MDF_FREE(data);

            
        }
        else{
            MDF_LOGI("MWIFI ainda nao esta conectado");
        }
        vTaskDelay(1000 / portTICK_RATE_MS);

    }
    
}

void node_write_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    int count     = 0;
    size_t size   = 0;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    mwifi_data_type_t data_type = {0x0};

    

    MDF_LOGI("Node write task is running");

    while(1){
        if(mwifi_is_connected()){
        size = sprintf(data, "(%d) Hello root!", count++);
        ret = mwifi_write(NULL, &data_type, data, size, true);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "mwifi_write, ret: %x", ret);
        MDF_FREE(data);
        
        }
        else{
            MDF_LOGI("MWIFI ainda nao esta conectado");
        }
        
        vTaskDelay(5000 / portTICK_RATE_MS); 

    }
}

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
                ", parent rssi: %d, node num: %d, free heap: %u", primary,
                esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
                mwifi_get_parent_rssi(), esp_mesh_get_total_node_num(), esp_get_free_heap_size());
        vTaskDelay(5000 / portTICK_RATE_MS);        
    }

    

}

void app_main()
{

    criarEvento();

    mwcomwifi_start();

    /**
     * @brief Data transfer between wifi mesh devices
     */
    
    xTaskCreate(node_write_task, "node_write_task", 4 * 1024,
                 NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    xTaskCreate(node_read_task, "node_read_task", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    

    TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS,true, NULL, print_system_info_timercb);
    xTimerStart(timer, 0);

    while(1){
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
