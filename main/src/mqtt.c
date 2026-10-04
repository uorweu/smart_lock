#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "MQTT_MOD";
static esp_mqtt_client_handle_t client = NULL;
bool mqtt_connected = false;

const char *hivemq_certificate = 
"-----BEGIN CERTIFICATE-----\n"
"MIIDiTCCAnGgAwIBAgIUZwwhQz7eL85wgNaOBggCRC1PLlQwDQYJKoZIhvcNAQEL\n"
"BQAwVDELMAkGA1UEBhMCVVMxDjAMBgNVBAgMBVN0YXRlMQ0wCwYDVQQHDARDaXR5\n"
"MRIwEAYDVQQKDAlTbWFydExvY2sxEjAQBgNVBAMMCU15TG9jYWxDQTAeFw0yNjEw\n"
"MDIwODUxMzhaFw0zNjA5MjkwODUxMzhaMFQxCzAJBgNVBAYTAlVTMQ4wDAYDVQQI\n"
"DAVTdGF0ZTENMAsGA1UEBwwEQ2l0eTESMBAGA1UECgwJU21hcnRMb2NrMRIwEAYD\n"
"VQQDDAlNeUxvY2FsQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC5\n"
"A9DKtGD6ZedBfp7LFidy1eJN0JaP3fo489MO+rY5cogpcZT+EeqFXIGFJOGaA92A\n"
"vc5RwxQQfBsWipDD8iA5ighvsXZniZ+4IgOsxOY3/f83UYMGkLDL8ped//YVR4I8\n"
"btDJ0hW6C+S9qD8gBSRIa1LPXESrvzxLRtHnmbWuC4bJnMNBUyxkiahVJ05lbFGZ\n"
"yuhG8LUna6WXOj26ioYY9eYqXdRBHPkW2pMitcgXboHWNulc83SgsipFJVEnZTjK\n"
"L5qeMzfCLaJVovUmQvCgUeI6kmDZFhCPqQwS9hq1ILXbojn8bHvZrl3EXp5CF75N\n"
"itR1fmXDVtBQTJLIexC1AgMBAAGjUzBRMB0GA1UdDgQWBBQ1lfSLeDAW3PtlrTzR\n"
"6IRnNfW+ojAfBgNVHSMEGDAWgBQ1lfSLeDAW3PtlrTzR6IRnNfW+ojAPBgNVHRMB\n"
"Af8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBZfKwTm2dvUnd4jxcwG6gb1gdU\n"
"2zdehzbwAcxCufUyN376P+AsO+qCj1qUH8GmNZKdtMNqrqVZDiNDGaATClOPql7F\n"
"zQIoTtbShTbYFU3jd5+iskJB8DigeIq+LXDAS1j4ZeW0A83hYHQEsYVvtnGOrw97\n"
"b1mMlzfcuyTA4n6F3OWZxMq/Mr+cZfhkpwyJsBDFhJL5TrJoct+Q1Wjh2kWVOind\n"
"6UIWHAERHexfmdMU3Fc3WYivyNRxOszHUGokGSytamLj661kejhT/Mn7SQAhjqqu\n"
"HJZprlNqMrUBJEJ9u4vsWl02tmQlm2plFlg9yjVskvjDhcX3INLEk06NbPGb\n"
"-----END CERTIFICATE-----\n";

void mqtt_init(void){
  esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = "mqtts://192.168.1.8:8883",
    .credentials.username = "lock_hardware",
    .credentials.authentication.password = "lock123",
    .broker.verification.certificate = hivemq_certificate,
    .broker.verification.skip_cert_common_name_check = true,
    .session.keepalive = 10,
    .session.last_will = {
        .topic = "smart_lock/status",
        .msg = "OFFLINE",
        .msg_len = 7,
        .qos = 1,
        .retain = 1
    }
  };
  client = esp_mqtt_client_init(&mqtt_cfg);
}

#include "state_machine.h"

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;
  switch (event_id) {
    case MQTT_EVENT_CONNECTED:
      mqtt_connected = true;
      ESP_LOGI(TAG, "SUCCESS! ESP32 is connected to the BROKER");
      esp_mqtt_client_publish(client,  "smart_lock/status", "ONLINE", 0, 0, 1);
      esp_mqtt_client_subscribe(client, "smart_lock/command", 1);
      break;
    case MQTT_EVENT_DISCONNECTED:
      mqtt_connected = false;
      ESP_LOGI(TAG, "We disconnected from the BROKER");
      break;
    case MQTT_EVENT_ERROR:
      ESP_LOGI(TAG, "A network error occurred");
      break;
    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG, "Received message on topic: %.*s", event->topic_len, event->topic);
      ESP_LOGI(TAG, "Message data: %.*s", event->data_len, event->data);
      
      if (strncmp(event->topic, "smart_lock/command", event->topic_len) == 0) {
          if (strncmp(event->data, "UNLOCK", event->data_len) == 0) {
              ESP_LOGI(TAG, "REMOTE UNLOCK AUTHORIZED! Opening door...");
              state_machine_process_event(EVENT_INSIDE_HANDLE); 
          } 
          else if (strncmp(event->data, "LOCK", event->data_len) == 0) {
              ESP_LOGI(TAG, "REMOTE LOCK AUTHORIZED! Forcing system to sleep...");
              state_machine_process_event(EVENT_TIMEOUT); 
          }
          else if (strncmp(event->data, "RESET", event->data_len) == 0) {
              ESP_LOGI(TAG, "REMOTE RESET AUTHORIZED! Clearing alarms and lockouts...");
              state_machine_process_event(EVENT_DEV_OVERRIDE); 
          }
          else if (strncmp(event->data, "UPDATE_HASH:", 12) == 0 && event->data_len == 12 + 64) {
              ESP_LOGI(TAG, "RECEIVED NEW PASSWORD HASH FROM CLOUD!");
              uint8_t new_hash[32];
              const char* hex_str = event->data + 12;
              for (int i = 0; i < 32; i++) {
                  sscanf(hex_str + 2*i, "%2hhx", &new_hash[i]);
              }
              update_secret_hash(new_hash);
              ESP_LOGI(TAG, "NVS Storage updated successfully!");
          }
      }
      break;
    default:
      break;
  }
}

void mqtt_start(void){
  if (client != NULL){
    esp_mqtt_client_start(client);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  }
}

void mqtt_publish_state(LockState state){
  if (client == NULL || !mqtt_connected) {
      ESP_LOGW(TAG, "MQTT not fully connected yet, skipping publish to avoid freezing main loop!");
      return;
  }
  
  const char *state_str = "UNKNOWN";
  switch (state){
    case STATE_SLEEP:                   state_str = "LOCKED"; break;
    case STATE_ENTERING_PIN:            state_str = "ENTERING_PIN"; break;
    case STATE_UNLOCKED:                state_str = "UNLOCKED"; break;
    case STATE_DENIED:                  state_str = "DENIED"; break;
    case STATE_AUTHORIZED_OPENING:      state_str = "AUTHORIZED_OPENING"; break;
    case STATE_ALARM:                   state_str = "ALARM_FORCED_OPENING"; break;
    case STATE_RECOVERY:                state_str = "RECOVERY"; break;
    case STATE_LOCKED_OUT:              state_str = "LOCKED_OUT"; break;
  }
  esp_mqtt_client_publish(client, "smart_lock/state", state_str, 0, 0, 1); 
  ESP_LOGI(TAG, "Published state to cloud: %s", state_str);
}
