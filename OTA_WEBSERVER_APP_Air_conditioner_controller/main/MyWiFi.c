#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "OTAServer.h"
#include "MyWiFi.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "esp_partition.h"
#include "esp_task.h"
#include "esp_ota_ops.h"
#include "freertos/message_buffer.h"
#include "app_inclued.h"
#include "tcp_client.h"

/* FreeRTOS event group to signal when we are connected*/
extern char * tcprx_buffer;
extern MessageBufferHandle_t tcp_send_data;
extern httpd_handle_t OTA_server;
extern uint8_t ip_addr1,ip_addr2,ip_addr3,ip_addr4;

void MyWiFi_init(void)
{
	xEventGroupClearBits(APP_event_group,APP_event_WIFI_AP_CONNECTED_BIT | APP_event_WIFI_STA_CONNECTED_BIT | APP_event_tcp_client_send_BIT);
	if((sleep_keep & sleep_keep_WIFI_AP_OR_STA_BIT) == sleep_keep_WIFI_AP_OR_STA_BIT)
	{
		//sleep_keep |= sleep_keep_WIFI_AP_OR_STA_BIT;
		init_wifi_softap(&OTA_server);
		printf("Welcome to esp32-c3 AP\r\n");
		tcprx_buffer = "Welcome to esp32-c3 AP";
	}
	else
	{
		//sleep_keep &= ~sleep_keep_WIFI_AP_OR_STA_BIT;
		init_wifi_station(&OTA_server);
		printf("Welcome to esp32-c3 STA.\r\n");
		tcprx_buffer = "Welcome to esp32-c3 STA.";
	}
}

static void ip_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    switch (event_id) {
		case IP_EVENT_STA_GOT_IP: 
		{
 			ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
			ip_addr1 = esp_ip4_addr1(&event->ip_info.ip);	
			ip_addr2 = esp_ip4_addr2(&event->ip_info.ip);
			ip_addr3 = esp_ip4_addr3(&event->ip_info.ip);
			ip_addr4 = esp_ip4_addr4(&event->ip_info.ip);		
        	printf("got ip:" IPSTR, IP2STR(&event->ip_info.ip));
			xEventGroupSetBits(APP_event_group,APP_event_WIFI_STA_CONNECTED_BIT);	

			/* Start the web server */
			start_OTA_webserver();
			break;
		}
		case IP_EVENT_STA_LOST_IP: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_LOST_IP\r\n");
			break;
		}
		case IP_EVENT_GOT_IP6: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_GOT_IP6\r\n");
			break;
		}
		case IP_EVENT_ETH_GOT_IP: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_ETH_GOT_IP\r\n");
			break;
		}
		default: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_ OTHER\r\n");
			break;
		}	
    }
    return;
}


static int s_retry_num = 0;
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    switch (event_id) {
		case WIFI_EVENT_WIFI_READY: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_WIFI_READY\r\n");
			break;
		}
		case WIFI_EVENT_SCAN_DONE: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_SCAN_DONE\r\n");
			break;
		}
		case WIFI_EVENT_STA_START: {
			esp_wifi_connect();
			ESP_LOGI("WiFI","Connectiing To SSID:%s : Pass:%s\r\n", CONFIG_STATION_SSID, CONFIG_STATION_PASSPHRASE);
			break;
		}
		case WIFI_EVENT_STA_STOP: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_STOP\r\n");
			break;
		}
		case WIFI_EVENT_STA_CONNECTED: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_CONNECTED\r\n");
			break;
		}
		case WIFI_EVENT_STA_DISCONNECTED: {
			xEventGroupClearBits(APP_event_group,APP_event_WIFI_STA_CONNECTED_BIT);
			if (s_retry_num < 100) 
			{
				esp_wifi_connect();
				s_retry_num++;
				printf("retry to connect to the AP.(%d / %d)\n", s_retry_num,100);
			} 
			else
			{
				printf("connect to the AP fail\r\n");
			}		
			break;
		}
		case WIFI_EVENT_STA_AUTHMODE_CHANGE: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_AUTHMODE_CHANGE\r\n");
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_SUCCESS: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_WPS_ER_SUCCESS\r\n");
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_FAILED: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_WPS_ER_FAILED\r\n");
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_TIMEOUT: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_WPS_ER_TIMEOUT\r\n");
			break;
		}
		case WIFI_EVENT_STA_WPS_ER_PIN: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_STA_WPS_ER_PIN\r\n");
			break;
		}
		case WIFI_EVENT_AP_START: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_AP_START\r\n");
			break;
		}
		case WIFI_EVENT_AP_STOP: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_AP_STOP\r\n");
			break;
		}
		case WIFI_EVENT_AP_STACONNECTED: {
			wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
			ip_addr1 = 192;	
			ip_addr2 = 168;
			ip_addr3 = 0;
			ip_addr4 = 1;	
        	printf("station "MACSTR" join, AID=%d",	\
                MAC2STR(event->mac), event->aid);
			xEventGroupSetBits(APP_event_group, APP_event_WIFI_AP_CONNECTED_BIT);
			/* Start the web server */
			start_OTA_webserver();
			break;
		}
		case WIFI_EVENT_AP_STADISCONNECTED: {
			wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        	printf("station "MACSTR" leave, AID=%d",	\
                MAC2STR(event->mac), event->aid);
			xEventGroupClearBits(APP_event_group,APP_event_WIFI_AP_CONNECTED_BIT);
			break;
		}
		case WIFI_EVENT_AP_PROBEREQRECVED: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_AP_PROBEREQRECVED\r\n");
			break;
		}
		case WIFI_EVENT_MAX: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_MAX\r\n");
			break;
		}
		default: {
			ESP_LOGI("WiFI", "SYSTEM_EVENT_ OTHER\r\n");
			break;
		}	
    }
    return;
}

/* -----------------------------------------------------------------------------
  start_dhcp_server(void)

  Notes:  
  
 -----------------------------------------------------------------------------*/
void start_dhcp_server(void) 
{
	// initialize the tcp stack
	esp_netif_init();
	// stop DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	// assign a static IP to the network interface
	tcpip_adapter_ip_info_t info;
	memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 0, 1);
	IP4_ADDR(&info.gw, 192, 168, 0, 1); //ESP acts as router, so gw addr will be its own addr
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	// start the DHCP server   
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	ESP_LOGI("WiFI","DHCP server started \n");
}
/* -----------------------------------------------------------------------------
  printStationList(void)

 print the list of connected stations  
  
 -----------------------------------------------------------------------------*/
void printStationList(void) 
{
	wifi_sta_list_t wifi_sta_list;
	tcpip_adapter_sta_list_t adapter_sta_list;
   
	memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
	memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

	// Give some time to gather intel of whats connected
	vTaskDelay(500 / portTICK_PERIOD_MS);
	
	ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&wifi_sta_list));	
	ESP_ERROR_CHECK(tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list));

	printf(" Connected Station List:\n");
	printf("--------------------------------------------------\n");
	
	

	if (adapter_sta_list.num >= 1)
	{
		for (int i = 0; i < adapter_sta_list.num; i++) 
		{
		
			tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
			printf("%d - mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x - IP:" IPSTR "\n",
				i + 1,
				station.mac[0],
				station.mac[1],
				station.mac[2],
				station.mac[3],
				station.mac[4],
				station.mac[5],
				//ip4addr_ntoa(&(station.ip)));
				IP2STR(&(station.ip)));
		}

		printf("\r\n");
	}
	else
	{
		printf("No Sations Connected\r\n");
	}

}
/* -----------------------------------------------------------------------------
  init_wifi_softap(void)

  Notes:  
  
  // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/wifi/esp_wifi.html
  // https://github.com/sankarcheppali/esp_idf_esp32_posts/blob/master/esp_softap/main/esp_softap.c
  
 -----------------------------------------------------------------------------*/
void init_wifi_softap(void *arg)
{
	esp_log_level_set("wifi", ESP_LOG_NONE);    // disable wifi driver logging
		
	ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
	
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	//ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	
	
	// configure the wifi connection and start the interface
	wifi_config_t ap_config = {
		.ap = {
			.ssid = CONFIG_AP_SSID,
		.password = CONFIG_AP_PASSPHARSE,
		.ssid_len = 0,
		.channel = 0,
		.authmode = AP_AUTHMODE,
		.ssid_hidden = AP_SSID_HIDDEN,
		.max_connection = AP_MAX_CONNECTIONS,
		.beacon_interval = AP_BEACON_INTERVAL,			
		},
	};
	
	
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	
	ESP_ERROR_CHECK(esp_wifi_start());

	start_dhcp_server();
	
	printf("ESP WiFi started in AP mode \n");
	
	
	 
	//  Spin up a task to show who connected or disconected
	 
	 
	//xTaskCreate(&print_sta_info, "print_sta_info", 4096, NULL, 1, NULL);

	// https://demo-dijiudu.readthedocs.io/en/latest/api-reference/wifi/esp_wifi.html#_CPPv225esp_wifi_set_max_tx_power6int8_t
	// This can only be placed after esp_wifi_start();
	ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(8));
	
}
/* -----------------------------------------------------------------------------
  init_wifi_station(void)

  Notes:  
  
  // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/wifi/esp_wifi.html
  
 -----------------------------------------------------------------------------*/
void init_wifi_station(void *arg)
{
	esp_log_level_set("wifi", ESP_LOG_NONE);     // disable wifi driver logging

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
	
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &ip_event_handler,
                                                        NULL,
                                                        NULL));


	//ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	
	
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_STATION_SSID,
		.password = CONFIG_STATION_PASSPHRASE
		},
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI("WiFi", "Station Set to SSID:%s Pass:%s\r\n", CONFIG_STATION_SSID, CONFIG_STATION_PASSPHRASE);

}

/* -----------------------------------------------------------------------------
  print_sta_info(void)

  Notes:  
 
 -----------------------------------------------------------------------------*/
void print_sta_info(void *pvParam) 
{
	printf("print_sta_info task started \n");
	
	while (1) 
	{	
		EventBits_t staBits = xEventGroupWaitBits(APP_event_group, APP_event_WIFI_STA_CONNECTED_BIT | APP_event_WIFI_AP_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
		
		if (staBits != 0)
		{
			printf("New station connected\n\n");
		}
		else
		{
			printf("A station disconnected\n\n");
		}
		
		printStationList();
	}
}

void wifi_ap_sta(void *pvParam)
{
	EventBits_t staBits;
	
	if((sleep_keep & sleep_keep_WIFI_AP_OR_STA_BIT) == sleep_keep_WIFI_AP_OR_STA_BIT)
	{
		staBits = xEventGroupWaitBits(APP_event_group, \
			APP_event_WIFI_AP_CONNECTED_BIT, \
			pdFALSE,                               \
			pdFALSE,                               \
			portMAX_DELAY);
		xMessageBufferReset(tcp_send_data);
		xTaskCreate(tcp_client_task, "tcp_client", 3072, NULL,ESP_TASK_PRIO_MIN + 1, NULL);
	
		printf("Create tcp_client AP\r\n");

		EventBits_t uxBits = xEventGroupWaitBits(APP_event_group, \
									APP_event_tcp_client_send_BIT, \
									pdTRUE,                               \
									pdFALSE,                               \
									30000 / portTICK_PERIOD_MS);
		if((uxBits & APP_event_tcp_client_send_BIT) != 0)
		{
			xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
		}
		tcp_client_send(ip_addr1);
		tcp_client_send(ip_addr2);
		tcp_client_send(ip_addr3);
		tcp_client_send(ip_addr4);
	}
	else
	{
		staBits = xEventGroupWaitBits(APP_event_group, \
			APP_event_WIFI_STA_CONNECTED_BIT, \
			pdFALSE,                               \
			pdFALSE,                               \
			200000 / portTICK_PERIOD_MS);
		if((staBits & APP_event_WIFI_STA_CONNECTED_BIT) != 0)
		{
			xMessageBufferReset(tcp_send_data);
			xTaskCreate(tcp_client_task, "tcp_client", 3072, NULL,ESP_TASK_PRIO_MIN + 1, NULL);	

			printf("Create tcp_client STA.\r\n");

			EventBits_t uxBits = xEventGroupWaitBits(APP_event_group, \
										APP_event_tcp_client_send_BIT, \
										pdTRUE,                               \
										pdFALSE,                               \
										30000 / portTICK_PERIOD_MS);
			if((uxBits & APP_event_tcp_client_send_BIT) != 0)
			{
				xMessageBufferSend(tcp_send_data,tcprx_buffer,strlen(tcprx_buffer), 1000 / portTICK_PERIOD_MS);
			}
			
			tcp_client_send(ip_addr1);
			tcp_client_send(ip_addr2);
			tcp_client_send(ip_addr3);
			tcp_client_send(ip_addr4);
		}
		else
		{
			sleep_keep |= sleep_keep_WIFI_AP_OR_STA_BIT;
    		printf("wifi Switch to AP \r\n");
			vTaskDelay(100 / portTICK_PERIOD_MS);
			xEventGroupSetBits(APP_event_group,APP_event_deepsleep_BIT);
		}
	}
	vTaskDelete(NULL);
}
