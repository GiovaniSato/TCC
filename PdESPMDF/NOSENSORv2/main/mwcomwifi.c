#include "mwcomwifi.h"
#include "mdf_common.h"
#include "mwifi.h"
#include "conectado.h"

static const char *TAG = "MWCOMWIFI";



void mandar_no(float temp, float umi){
    
    mdf_err_t ret = MDF_OK;
    size_t size   = 0;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    mwifi_data_type_t data_type = {0x0};

    
    if(mwifi_is_connected()){
        size = sprintf(data, " Temperatura: %0.1f C e Umidade:  %0.1f%%", temp, umi);
        ret = mwifi_write(NULL, &data_type, data, size, true);
        //MDF_ERROR_CONTINUE(ret != MDF_OK, "mwifi_write, ret: %x", ret);
        MDF_FREE(data);
    }
    
    else{
        MDF_LOGI("MWIFI ainda nao esta conectado");
    }

}

/**
 * @brief All module events will be sent to this task in esp-mdf
 *
 * @Note:
 *     1. Do not block or lengthy operations in the callback function.
 *     2. Do not consume a lot of memory in the callback function.
 *        The task memory of the callback function is only 4KB.
 */
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


void mw_start(void){
    
    mwifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config   = {
        .channel   = 13,
        .mesh_id   = "123456",
        .mesh_type = MESH_NODE,
    };

    /**
     * @brief Set the log level for serial port printing.
     */
    //esp_log_level_set("*", ESP_LOG_INFO);
    //esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief Initialize wifi mesh.
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));

    MDF_ERROR_ASSERT(mwifi_start());

}