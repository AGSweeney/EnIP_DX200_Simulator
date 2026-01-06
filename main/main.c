/*
 * Copyright (c) 2025, Adam G. Sweeney <agsweeney@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "esp_event.h"
#include "esp_eth.h"
#include "esp_eth_mac.h"
#include "esp_eth_phy.h"
#include "esp_eth_com.h"
#include "esp_eth_mac_esp.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "opener.h"
#include "webui.h"
#include "system_config.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"

static const char *TAG = "main";
static esp_netif_t *s_eth_netif = NULL;
static esp_eth_handle_t s_eth_handle = NULL;

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

static void configure_netif(esp_netif_t *netif) {
    if (netif == NULL) {
        return;
    }
    
    system_ip_config_t config;
    bool loaded = system_ip_config_load(&config);
    
    if (!loaded) {
        system_ip_config_get_defaults(&config);
        ESP_LOGI(TAG, "Using default DHCP configuration (NVS config not found)");
    } else {
        ESP_LOGI(TAG, "Loaded network configuration from NVS");
    }
    
    if (config.use_dhcp) {
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(netif));
        ESP_ERROR_CHECK(esp_netif_dhcpc_start(netif));
        ESP_LOGI(TAG, "Network configured for DHCP");
    } else {
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(netif));
        
        esp_netif_ip_info_t ip_info;
        ip_info.ip.addr = config.ip_address;
        ip_info.netmask.addr = config.netmask;
        ip_info.gw.addr = config.gateway;
        
        ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));
        
        if (config.dns1 != 0) {
            ip4_addr_t dns1_addr;
            dns1_addr.addr = config.dns1;
            esp_netif_dns_info_t dns_info = {0};
            dns_info.ip.u_addr.ip4.addr = dns1_addr.addr;
            dns_info.ip.type = ESP_IPADDR_TYPE_V4;
            esp_err_t ret = esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to set DNS1: %s", esp_err_to_name(ret));
            }
        }
        
        if (config.dns2 != 0) {
            ip4_addr_t dns2_addr;
            dns2_addr.addr = config.dns2;
            esp_netif_dns_info_t dns_info = {0};
            dns_info.ip.u_addr.ip4.addr = dns2_addr.addr;
            dns_info.ip.type = ESP_IPADDR_TYPE_V4;
            esp_err_t ret = esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to set DNS2: %s", esp_err_to_name(ret));
            }
        }
        
        char ip_str[16], netmask_str[16], gw_str[16];
        ip4_addr_t ip_log, netmask_log, gw_log;
        ip_log.addr = ip_info.ip.addr;
        netmask_log.addr = ip_info.netmask.addr;
        gw_log.addr = ip_info.gw.addr;
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_log));
        snprintf(netmask_str, sizeof(netmask_str), IPSTR, IP2STR(&netmask_log));
        snprintf(gw_str, sizeof(gw_str), IPSTR, IP2STR(&gw_log));
        ESP_LOGI(TAG, "Network configured with static IP: %s, Netmask: %s, Gateway: %s", 
                 ip_str, netmask_str, gw_str);
    }
}

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    
    esp_netif_dns_info_t dns_info;
    esp_err_t dns_ret = esp_netif_get_dns_info(s_eth_netif, ESP_NETIF_DNS_MAIN, &dns_info);
    if (dns_ret != ESP_OK || dns_info.ip.u_addr.ip4.addr == 0) {
        uint32_t dns1_addr = esp_ip4addr_aton("8.8.8.8");
        dns_info.ip.u_addr.ip4.addr = dns1_addr;
        dns_info.ip.type = ESP_IPADDR_TYPE_V4;
        esp_err_t ret = esp_netif_set_dns_info(s_eth_netif, ESP_NETIF_DNS_MAIN, &dns_info);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Set default DNS1: 8.8.8.8");
        }
        
        uint32_t dns2_addr = esp_ip4addr_aton("8.8.4.4");
        dns_info.ip.u_addr.ip4.addr = dns2_addr;
        ret = esp_netif_set_dns_info(s_eth_netif, ESP_NETIF_DNS_BACKUP, &dns_info);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Set default DNS2: 8.8.4.4");
        }
    }

    if (s_eth_netif != NULL) {
        struct netif *lwip_netif = esp_netif_get_netif_impl(s_eth_netif);
        if (lwip_netif != NULL) {
            ESP_LOGI(TAG, "Initializing OpENer EtherNet/IP stack...");
            opener_init(lwip_netif);
            
            ESP_LOGI(TAG, "Initializing Web UI...");
            if (!webui_init()) {
                ESP_LOGW(TAG, "Failed to initialize Web UI");
            }
        } else {
            ESP_LOGE(TAG, "Failed to get lwIP netif from esp_netif");
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "--- Application Startup ---");
    
    if (esp_psram_is_initialized()) {
        size_t psram_size = esp_psram_get_size();
        ESP_LOGI(TAG, "PSRAM initialized: %zu bytes (%.2f MB)", psram_size, psram_size / (1024.0f * 1024.0f));
    } else {
        ESP_LOGW(TAG, "PSRAM not initialized - check CONFIG_SPIRAM_BOOT_INIT=y");
    }
    
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    s_eth_netif = esp_netif_new(&cfg);
    ESP_ERROR_CHECK(esp_netif_set_default_netif(s_eth_netif));

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, 
                                               &eth_event_handler, s_eth_netif));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, 
                                               &got_ip_event_handler, s_eth_netif));

    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    
    phy_config.phy_addr = 1;
    phy_config.reset_gpio_num = 51;

    esp32_emac_config.smi_gpio.mdc_num = 31;
    esp32_emac_config.smi_gpio.mdio_num = 52;
    esp32_emac_config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
    esp32_emac_config.clock_config.rmii.clock_gpio = 50;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    s_eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &s_eth_handle));

    esp_eth_netif_glue_handle_t glue = esp_eth_new_netif_glue(s_eth_handle);
    ESP_ERROR_CHECK(esp_netif_attach(s_eth_netif, glue));

    ESP_ERROR_CHECK(esp_netif_set_hostname(s_eth_netif, "Motoman-DX200-Sim"));

    configure_netif(s_eth_netif);
    
    ESP_ERROR_CHECK(esp_eth_start(s_eth_handle));
}
