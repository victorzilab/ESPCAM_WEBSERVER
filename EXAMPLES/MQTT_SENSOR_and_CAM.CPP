#include <WiFi.h>
#include <esp_camera.h>
#include <PubSubClient.h>
#include <NewPing.h>

// Camera configuration
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// WiFi credentials
const char* ssid = "TECNOTRON";
const char* password = "5A5E6CX1@p";

// MQTT broker settings
const char* mqtt_server = "homebroker.local";
const int mqtt_port = 1883;
const char* mqtt_topic = "sensor/hcsr04";

WiFiClient espClient;
PubSubClient client(espClient);

// HC-SR04 sensor pins
#define TRIGGER_PIN  12
#define ECHO_PIN     13
#define MAX_DISTANCE 400
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastSensorReadTime = 0;
const unsigned long sensorReadInterval = 50; // Sensor read interval in milliseconds
const unsigned long mqttReconnectInterval = 5000; // MQTT reconnect interval in milliseconds

void setup() {
  // Serial monitor setup
  Serial.begin(115200);
  
  // Camera setup
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // WiFi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // MQTT setup
  client.setServer(mqtt_server, mqtt_port);
}

void reconnectMQTT() {
  if (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("Failed to connect to MQTT, state: ");
      Serial.println(client.state());
    }
  }
}

void loop() {
  unsigned long now = millis();

  // Maintain MQTT connection
  if (!client.connected() && (now - lastMqttReconnectAttempt > mqttReconnectInterval)) {
    lastMqttReconnectAttempt = now;
    reconnectMQTT();
  } else if (client.connected()) {
    client.loop();
  }

  // Read distance from HC-SR04 at specified intervals
  if (now - lastSensorReadTime > sensorReadInterval) {
    lastSensorReadTime = now;
    unsigned int distance = sonar.ping_cm();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println("cm");

    // Publish distance to MQTT if connected
    if (client.connected()) {
      char msg[50];
      snprintf(msg, 50, "Distance: %d cm", distance);
      client.publish(mqtt_topic, msg);
    }
  }
}
