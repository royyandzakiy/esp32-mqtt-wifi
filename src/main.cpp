#include <Arduino.h>
#include "WiFi.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "credentials.h"
#include "global.h"
#include "HelperTasks.h"
#include <HardwareSerial.h>

#define ARDUINO_MAIN_CORE 0
#define ARDUINO_TASK_CORE 1

#ifndef CREDENTIALS_H
/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "YOUR_WIFI_SSID"
#define WLAN_PASS       "YOUR_WIFI_PASSWORD"

/************************* Adafruit.io Setup *********************************/

#define MQTT_SERVER      "YOUR_MQTT_BROKER"
#define MQTT_SERVERPORT  1883                   // use 8883 for SSL
#define MQTT_USERNAME    "YOUR_MQTT_BROKER_USERNAME"
#define MQTT_KEY         "YOUR_MQTT_BROKER_PASSWORD"

#endif

/*************************** Sketch Code ************************************/

HardwareSerial ArduinoSerial(2); // RX, TX

void setupWifi();
void setupMQTT();
void MQTT_connect();

void sendArduino(String);
void listenArduino();
void processCommandArduino(String);
void echo();
void getMQTTMessages();
void TaskTestPublish(void *pvParameters);

void setup() {
  Serial.begin(9600);
  ArduinoSerial.begin(9600);
  delay(10);

  Serial.println(F("Waterbox Initialize..."));

  // Connect to WiFi access point.
  setupWifi();

  // Setup MQTT subscription
  setupMQTT();

  Serial.println(F("Waterbox Setup Complete."));
}

void loop() {
  // Ensure the connection to the MQTT server is alive
  MQTT_connect();

  if (mqtt.connected()) {
    // Get messages from subscribed topics
    getMQTTMessages();
  }

  echo();
  listenArduino();

  // ping the server to keep the mqtt connection alive
  if(!mqtt.ping()) {
    mqtt.disconnect();
  }
}

void echo() {
  if(Serial.available()) {
    String str = Serial.readStringUntil('\n');
    Serial.println(str);
    sendArduino(str);
    delay(10);
  }
}

void listenArduino() {
  if(ArduinoSerial.available()) {
    String str = ArduinoSerial.readStringUntil('\n');
    Serial.println(str);
    processCommandArduino(str);
    delay(10);
  }
}

void processCommandArduino(String str) {
  String command, topicFull, topic, message;
  int separatorPos = str.indexOf(":");
  if (separatorPos > -1) {
    command = str.substring(0,separatorPos);
    if (command == "pub") {
      topicFull = str.substring(separatorPos+1, str.indexOf(":",separatorPos+1));
      topic = topicFull.substring(topicFull.indexOf("/",10)+1);
      separatorPos = str.indexOf(":",separatorPos+1);
      if (topic == "flow_sensor") {
        message = str.substring(separatorPos+1);
        flow_sensor_pub.publish(message.toFloat());
      }
    }
  }
  Serial.println(command);
  Serial.println(topic);
  Serial.println(message);
}

void sendArduino(String str) {
  ArduinoSerial.println(str);
}

void setupMQTT() {
  Serial.println(F("MQTT Setup..."));
  mqtt.subscribe(&temp_sensor_sub);
  mqtt.subscribe(&flow_sensor_sub);
  mqtt.subscribe(&test_sub);

  // xTaskCreatePinnedToCore(
  //   TaskTestPublish
  //   ,  "TaskTestPublish"   // A name just for humans
  //   ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
  //   ,  NULL
  //   ,  0  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
  //   ,  NULL
  //   ,  ARDUINO_TASK_CORE
  // );  

  // xTaskCreatePinnedToCore(
  //   TaskTempPublish
  //   ,  "TaskTempPublish"   // A name just for humans
  //   ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
  //   ,  NULL
  //   ,  0  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
  //   ,  NULL
  //   ,  ARDUINO_TASK_CORE
  // );  

  // xTaskCreatePinnedToCore(
  //   TaskFlowPublish
  //   ,  "TaskFlowPublish"   // A name just for humans
  //   ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
  //   ,  NULL
  //   ,  0  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
  //   ,  NULL
  //   ,  ARDUINO_TASK_CORE
  // );  
}

void getMQTTMessages() {
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    Serial.println("Message Incoming!");

    // Check from which Topic did the message come from
    if (subscription == &flow_sensor_sub) {
      Serial.print(F("Flow Sensor: "));
      Serial.println((char *)flow_sensor_sub.lastread);
    }
    if (subscription == &temp_sensor_sub) {
      Serial.print(F("Temp Sensor: "));
      Serial.println((char *)temp_sensor_sub.lastread);
    }
    if (subscription == &test_sub) {
      Serial.print(F("Test Feed: "));
      Serial.println((char *)test_sub.lastread);
    }
  }
}

void setupWifi() {
  Serial.println(F("Wifi Setup..."));
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
