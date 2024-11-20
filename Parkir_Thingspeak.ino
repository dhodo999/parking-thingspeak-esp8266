#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WiFiClient.h>
#include <Servo.h>

#define WIFI_SSID "" // Your Wi-Fi SSID
#define WIFI_PASSWORD "" // Your Wi-Fi password

#define THINGSPEAK_CHANNEL_ID "" // Your ThingSpeak channel ID
#define THINGSPEAK_WRITE_API_KEY "" // Your ThingSpeak write API key

// Ultrasonic & sensor pins
const int trigPinA = D1;
const int echoPinA = D2;
const int trigPinB = D5;
const int echoPinB = D6;
const int buzzerPin = D3;

int parkirKuota = 300; // Initial parking quota

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Servo myServo;

const long interval = 10000; // Interval to read ultrasonic sensor (Can be adjusted)
unsigned long previousMillis = 0; // Time to store last reading

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi. Reconnecting...");
}

long readUltrasonicDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  long distance = duration * 0.034 / 2;
  return distance;
}

void updateKuota(int perubahan) {
  if (parkirKuota + perubahan >= 0) {
    parkirKuota += perubahan;
    Serial.printf("Kuota parkir sekarang: %i\n", parkirKuota);

    WiFiClient client;
    if (client.connect("api.thingspeak.com", 80)) {
      String url = "/update?api_key=" + String(THINGSPEAK_WRITE_API_KEY) + "&field1=" + String(parkirKuota);
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: api.thingspeak.com\r\n" +
                   "Connection: close\r\n\r\n");
      Serial.println("Data sent to ThingSpeak: " + String(parkirKuota));
    } else {
      Serial.println("Connection to ThingSpeak failed.");
    }
  } else {
    Serial.println("Kuota sudah 0, tidak dapat mengurangi lebih lanjut.");
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(trigPinA, OUTPUT);
  pinMode(echoPinA, INPUT);
  pinMode(trigPinB, OUTPUT);
  pinMode(echoPinB, INPUT);
  pinMode(buzzerPin, OUTPUT);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  connectToWifi();

  myServo.attach(D4); // Servo motor pin
  Serial.println("Servo test started");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    long distanceA = readUltrasonicDistance(trigPinA, echoPinA);
    long distanceB = readUltrasonicDistance(trigPinB, echoPinB);

    // Gerbang A logic
    if (distanceA < 10 && parkirKuota > 0) {
      updateKuota(-1);
      myServo.write(180); // Open gate A
      delay(6000);        // Keep gate open for 3 seconds
      myServo.write(0);   // Close gate A
    }
    // Gerbang B logic
    if (distanceB < 10) {
      updateKuota(1);
      digitalWrite(buzzerPin, HIGH);  // Activate buzzer
      delay(1000);                    // Buzz briefly
      digitalWrite(buzzerPin, LOW);   // Deactivate buzzer
    }
  }

  delay(500);  // Stabilization delay
}
