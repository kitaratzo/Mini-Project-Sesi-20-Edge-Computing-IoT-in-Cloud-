#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHTesp.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define LEBAR_LAYAR 128
#define TINGGI_LAYAR 64

#define ledMerah 14
#define ledKuning 12
#define ledBiru 13
#define buzzer 25
const int DHT_PIN = 15;

DHTesp dhtSensor;
Adafruit_SSD1306 oled(LEBAR_LAYAR, TINGGI_LAYAR, &Wire, -1);

// WiFi & MQTT
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
String ruangan;

void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32ClientRoom")) {
      Serial.println(" connected!");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay();
  oled.display();

  pinMode(ledMerah, OUTPUT);
  pinMode(ledKuning, OUTPUT);
  pinMode(ledBiru, OUTPUT);
  pinMode(buzzer, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float temp = data.temperature;

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(20, 0);
  oled.println("Ruang Server");

  oled.setCursor(0, 20);
  oled.print("Nilai Suhu : ");
  oled.setCursor(75, 20);
  oled.print(temp);
  oled.print(" C");

  if (temp < 20) {
    digitalWrite(ledBiru, HIGH);
    digitalWrite(ledKuning, LOW);
    digitalWrite(ledMerah, LOW);
    ruangan = "SUHU RENDAH";
  } else if (temp >= 20 && temp <= 35) {
    digitalWrite(ledBiru, LOW);
    digitalWrite(ledKuning, HIGH);
    digitalWrite(ledMerah, LOW);
    ruangan = "SUHU CUKUP";
  } else {
    digitalWrite(ledBiru, LOW);
    digitalWrite(ledKuning, LOW);
    digitalWrite(ledMerah, HIGH);
    tone(buzzer, 150);
    delay(500);
    noTone(buzzer);
    ruangan = "SUHU PANAS";
  }

  oled.setCursor(0, 40);
  oled.print("Keadaan Ruangan = ");
  oled.setCursor(0, 50);
  oled.print(ruangan);
  oled.display();

  // Kirim ke MQTT
  String payload = "{\"temperature\":" + String(temp) + ",\"status\":\"" + ruangan + "\"}";
  client.publish("/chandra", payload.c_str());
  Serial.println("Published: " + payload);

  delay(3000);
}
