#include "app_init.h"

extern int32_t BLe_battery;
nvs_handle_t app_data;

void app_init(void)
{
    /* IO 初始化 */
	gpio_reset_pin(3);
    gpio_reset_pin(4);
    gpio_reset_pin(5);
    gpio_reset_pin(18);
    gpio_reset_pin(19);
    gpio_set_level(18, 0);//白
    gpio_set_level(19, 0);//橙
    gpio_set_level(3, 0); //三色灯红  
    gpio_set_level(4, 0); //三色灯绿
    gpio_set_level(5, 0); //三色灯橙
    gpio_set_direction(3, GPIO_MODE_OUTPUT);
    gpio_set_direction(4, GPIO_MODE_OUTPUT);
    gpio_set_direction(5, GPIO_MODE_OUTPUT);
    gpio_set_direction(18, GPIO_MODE_OUTPUT);
    gpio_set_direction(19, GPIO_MODE_OUTPUT);

    //Initialize NVS
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);



    // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    err = nvs_open("storage", NVS_READWRITE, &app_data);
    if (err != ESP_OK) 
    {
       printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else 
    {
        printf("Done\n");

        // Read
        printf("Reading restart counter from NVS ... ");
        
        err = nvs_get_i32(app_data, "BLe_battery", &BLe_battery);
        switch (err) 
        {
            case ESP_OK:
                printf("Done\n");
                printf("Restart counter = %d\n", BLe_battery);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        // Close
        nvs_close(app_data);
    }
}