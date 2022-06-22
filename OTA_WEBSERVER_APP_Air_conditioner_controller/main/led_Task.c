#include "led_Task.h"
#include "driver/ledc.h"

extern uint8_t ip_addr1,ip_addr2,ip_addr3,ip_addr4;

#define LEDC_LS_TIMER          LEDC_TIMER_1
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_LS_CH0_GPIO       (3)
#define LEDC_LS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_LS_CH1_GPIO       (4)
#define LEDC_LS_CH1_CHANNEL    LEDC_CHANNEL_1
#define LEDC_LS_CH2_GPIO       (19)
#define LEDC_LS_CH2_CHANNEL    LEDC_CHANNEL_2

#define LEDC_TEST_CH_NUM       (3)
#define LEDC_TEST_DUTY         (2)

void LED_Task_init(void)
{
    xEventGroupSetBits(APP_event_group,APP_event_Standby_BIT);
    // Clear the bit
	xEventGroupClearBits(APP_event_group,APP_event_run_BIT);
}

void led_instructions(void *pvParam)
{
    uint8_t con = 0;
    portMUX_TYPE  test = portMUX_INITIALIZER_UNLOCKED;
    uint8_t pcWriteBuffer[2048];

    int ch;

    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,           // timer mode
        .timer_num = LEDC_LS_TIMER,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);
    /*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: if different channels use one timer,
     *         then frequency and bit_num of these channels
     *         will be the same
     */
    ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
        {
            .channel    = LEDC_LS_CH0_CHANNEL,
            .duty       = 0,
            .gpio_num   = LEDC_LS_CH0_GPIO,
            .speed_mode = LEDC_LS_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_LS_TIMER
        },
        {
            .channel    = LEDC_LS_CH1_CHANNEL,
            .duty       = 0,
            .gpio_num   = LEDC_LS_CH1_GPIO,
            .speed_mode = LEDC_LS_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_LS_TIMER
        },
        {
            .channel    = LEDC_LS_CH2_CHANNEL,
            .duty       = 0,
            .gpio_num   = LEDC_LS_CH2_GPIO,
            .speed_mode = LEDC_LS_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_LS_TIMER
        },
    };

    // Set LED Controller with previously prepared configuration
    for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

    while(1) 
    {
        EventBits_t staBits = xEventGroupWaitBits(APP_event_group, APP_event_Standby_BIT | APP_event_run_BIT,\
                                                pdFALSE,pdFALSE, 100 / portTICK_PERIOD_MS);
        if ((staBits & APP_event_Standby_BIT) != 0)
		{
            ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, LEDC_TEST_DUTY);
            ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
        }      
        else
        {
            ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, 0);
            ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
        }

        if ((staBits & APP_event_run_BIT) != 0)
		{
            ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, LEDC_TEST_DUTY);
            ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
        }      
        else
        {
            ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, 0);
            ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
        }    
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if ((sleep_keep & sleep_keep_Thermohygrometer_Low_battery_BIT) == sleep_keep_Thermohygrometer_Low_battery_BIT)
		{
            ledc_set_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel, LEDC_TEST_DUTY);
            ledc_update_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel);
        } 
        else
        {
            ledc_set_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel, 0);
            ledc_update_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel);
        }
        con++;
        if(con >= 100)
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
