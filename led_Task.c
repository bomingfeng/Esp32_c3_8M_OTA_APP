#include "led_Task.h"

extern uint8_t ip_addr1,ip_addr2,ip_addr3,ip_addr4;

void LED_Task_init(void)
{
    xEventGroupSetBits(APP_event_group,APP_event_Standby_BIT);
    // Clear the bit
	xEventGroupClearBits(APP_event_group,APP_event_run_BIT);
}

void led_instructions(void *pvParam)
{
    uint8_t con = 0;
    uint8_t Standby = 0xaa;
    portMUX_TYPE  test = portMUX_INITIALIZER_UNLOCKED;
    uint8_t pcWriteBuffer[2048];

    while(1) 
    {
        EventBits_t staBits = xEventGroupWaitBits(APP_event_group, APP_event_Standby_BIT | APP_event_run_BIT | APP_event_ir_rx_flags_BIT,\
                                                pdFALSE,pdFALSE, 500 / portTICK_PERIOD_MS);
        if (((staBits & APP_event_Standby_BIT) != 0) && (Standby == 0xaa) || ((sleep_keep & sleep_keep_Thermohygrometer_Low_battery_BIT) == sleep_keep_Thermohygrometer_Low_battery_BIT))
		{
            Standby = 0x55;
            ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, 300);
            ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
        }             
        else
        {
            Standby = 0xaa;
            ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, 0);
            ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
        } 

        if ((staBits & APP_event_run_BIT) != 0)
		{
            ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, 5000);
            ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
        } 
        else if ((staBits & APP_event_ir_rx_flags_BIT) != 0)
        {
            ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, 15000);
            ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
            xEventGroupClearBits(APP_event_group,APP_event_ir_rx_flags_BIT);
        }     
        else
        {
            ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, 0);
            ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
        } 

        con++;
        if(con >= 20)
        {
            con = 0;
            tcp_client_send(ip_addr1);
            tcp_client_send(ip_addr2);
            tcp_client_send(ip_addr3);
            tcp_client_send(ip_addr4);
            vTaskList((char *)&pcWriteBuffer);
            printf("%s\r\n", pcWriteBuffer); 
        }
    }
}
