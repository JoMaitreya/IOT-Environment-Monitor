#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <DHT.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// Adafruit IO configuration
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "your_username"
#define AIO_KEY         "Your_adafruit_io_key"

// WiFi configuration
const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish Temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperature");
Adafruit_MQTT_Publish Humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Humidity");
Adafruit_MQTT_Publish Soil = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Soil");
Adafruit_MQTT_Publish RelayStatus = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Relay");

#define DHTPIN 26
#define DHTTYPE DHT11
#define RELAY 27
DHT dht(DHTPIN, DHTTYPE);

// Create the lcd object address 0x27 and 16 columns x 2 rows 
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Temp, Hum & Soil");
  delay(1000);
  lcd.clear();
  
  // Initialize Serial
  Serial.begin(115200);
  
  // Initialize Relay
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);

  // Initialize DHT sensor
  dht.begin();

  // Connect to WiFi
  connectToWiFi();
  
  // Connect to Adafruit IO
  connectToMQTT();
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  
  // Keep trying to connect for 10 seconds
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected!");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" Failed to connect to WiFi");
    // Handle WiFi connection failure here if needed
  }
}

void connectToMQTT() {
  while (mqtt.connect() != 0) {
    Serial.println("MQTT connection failed! Retrying...");
    delay(5000); // Delay for retrying
  }
  Serial.println("Connected to Adafruit IO!");
}

void loop() {
  // Reconnect to MQTT if necessary
  if (!mqtt.ping(3)) {
    if (!mqtt.connected()) {
      connectToMQTT();
    }
  }

  // Read sensor values
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float i = analogRead(34);

  // Determine soil moisture status
  String msg = i < 2165 ? "WET" : i > 3135 ? "DRY" : "OK";
  Serial.println("Temp: " + String(t));
  Serial.println("Hum: " + String(h));
  Serial.println("Soil: " + String(i));

  // Update LCD display
  lcd.setCursor(0, 0);
  lcd.print(t);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print(h);
  lcd.print("%");
  lcd.setCursor(8, 1);
  lcd.print("Soil:");
  lcd.print(msg);

  // Control relay based on soil moisture
  if (msg == "DRY") {
    digitalWrite(RELAY, HIGH); // Turn on relay
    updateRelayStatus("ON");
  } else {
    digitalWrite(RELAY, LOW); // Turn off relay
    updateRelayStatus("OFF");
  }

  // Publish sensor values to Adafruit IO
  if (!Temperature.publish(t)) {
    Serial.println(F("Failed to publish temperature"));
  }
  if (!Humidity.publish(h)) {
    Serial.println(F("Failed to publish humidity"));
  }
  if (!Soil.publish(msg.c_str())) { // Convert String to const char*
    Serial.println(F("Failed to publish soil moisture"));
  } else {
    Serial.println(F("Data sent to Adafruit IO!"));
  }

  delay(5000); // Delay between updates
}

void updateRelayStatus(const char* status) {
  if (!RelayStatus.publish(status)) {
    Serial.println(F("Failed to publish relay status"));
  } else {
    Serial.println(F("Relay status sent to Adafruit IO!"));
  }
}
