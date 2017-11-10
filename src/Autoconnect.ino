#include <FS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <PubSubClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <Portal.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

//#define DHTTYPE           DHT11     // DHT 11
#define DHTTYPE           DHT22     // DHT 22 (AM2302)
//#define DHTTYPE           DHT21     // DHT 21 (AM2301)
float temp ;
float humi ;
int try_again = 0 ;
int state ;
float voltage;

WiFiClient espClient;
PubSubClient client(espClient);


//Portal
//Local intialization. Once its business is done, there is no need to keep it around
Portal Portal;


void DHT_get_temp() {

  DHT_Unified dht(Portal.GPIO, DHTTYPE);
  dht.begin();
  sensors_event_t event;
  dht.temperature().getEvent(&event);
   if (isnan(event.temperature)) {
     Serial.println("Error reading temperature!");
    } else {
     Serial.print("Temperature: ");
     Serial.print(event.temperature);
     Serial.println(" *C");
     temp = event.temperature ;
   }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    Serial.print("Humidity: ");
    Serial.print(event.relative_humidity);
    Serial.println("%");
    humi = event.relative_humidity ;
  }
}

void Get_voltage() {
 int adc = analogRead(A0) ;
 Serial.print("Voltage=");
 Serial.println(adc);
 voltage = (4.3/1023)*adc;
}

void reconnect() {
  // Loop until we're reconnected
 try_again = 0 ;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
     // client.publish(Portal.mqtt_topic, String(temp).c_str());
      // ... and resubscribe
     // client.subscribe("inTopic");
    } else {
       if (try_again > 2 ) {
        Serial.print("failed 3 times, exiting...\n") ;
        break ;
       }
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      try_again ++ ;
    }
  }
}

void setup() {
 Serial.begin(115200);
 pinMode(13, INPUT_PULLUP); // Gpio13 to put the ESP back to AP
 //fetches ssid and pass from eeprom and tries to connect

if (Portal.autoConnect()) {
  //auto generated name ESP + ChipID
  //and goes into a blocking loop awaiting configuration from the webpage...
  //......
  //Now quitting the loop...
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //Convert from String to Char*
  Portal.mqtt_server = &Portal._mqttIP[0];
  Portal.mqtt_topic = &Portal._topic[0];
  Portal.mqtt_topic2 = &Portal._topic2[0];
  //convert String to int
  Portal.RefreshTime = atoi(Portal._RefreshTime.c_str());
  Portal.GPIO = atoi(Portal._GPIO.c_str());

  Serial.println("Reading data values:");
  Serial.print("MQTTIP=");
  Serial.println(Portal._mqttIP);
  Serial.print("topic1=");
  Serial.println(Portal._topic);
  Serial.print("topic2=");
  Serial.println(Portal._topic2);
  Serial.print("GPIO=");
  Serial.println(Portal._GPIO);
  Serial.print("RestartTime=");
  Serial.println(Portal._RefreshTime);
  Serial.print("HasSensor=");
  Serial.println(Portal._hasSensor);

  //Start a temp measure
  if (Portal._hasSensor == "true") {
  Serial.println("Has sensor!");
  DHT_get_temp() ;
  } else {
  Serial.println("Has no sensor, read GPIO state.");
  state = digitalRead(Portal.GPIO);
  }

 //Start a voltage measure
  Get_voltage();

 //Start MQTT client
  client.setServer(Portal.mqtt_server, 1883);

  if (!client.connected()) {
    reconnect();
  }
  if (try_again < 3) {
    client.loop();
    if (Portal._hasSensor == "true") {
       //Temp
       Serial.printf("Publish message: %s:%s \n ", Portal.mqtt_topic, String(temp).c_str());
       client.publish(Portal.mqtt_topic, String(temp).c_str());
       //humidity
      delay(100);
       Serial.printf("Publish message: %s:%s \n ", Portal.mqtt_topic2, String(humi).c_str());
       client.publish(Portal.mqtt_topic2, String(humi).c_str());
    } else {
       Serial.printf("Publish message: %s:%s \n ", Portal.mqtt_topic, String(state).c_str());
       client.publish(Portal.mqtt_topic, String(state).c_str());
    }

  Serial.printf("Publish message: %s:%s \n ", "voltage/batterie", String(voltage).c_str());
  client.publish("voltage/batterie", String(voltage).c_str());

  }

 Serial.printf("Now, I'm going to sleep, bye bye, see you in %d secondes\n ", Portal.RefreshTime) ;
 ESP.deepSleep(Portal.RefreshTime * 1000000);

 }
 else {

 Serial.printf("Wifi_STA failed, let's retry in 10 min, going to deepsleep") ;
 ESP.deepSleep(600 * 1000000);

 }

}

void loop() {
}
