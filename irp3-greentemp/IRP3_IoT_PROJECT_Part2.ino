#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "credentials.h"
#include "parameters.h"
#include "profile.h"

/**** Define IO settings *******/
#define LED 13
#define sendLED 18
#define receiveLED 19      
#define temp 32     
#define PB 0 
#define RELAY 27
#define potpin 33
#define Buzzer 25
#define Button 23

/**** Global variables *****/
float Threshold; 
long timer;
int buttonstate; 
char publish_topic[maxtopiclength];
char subscribe_topic[maxtopiclength];


/**** WIFI and MQTT Client Initialise ********/
WiFiClient espClient;
PubSubClient client(espClient);


void setup() {

  pinMode(LED, OUTPUT); 
  pinMode(sendLED, OUTPUT);
  pinMode(receiveLED, OUTPUT);
  pinMode(temp, INPUT);
  pinMode(PB, INPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(Button, INPUT);
  
  digitalWrite(RELAY, HIGH);                // initialise RELAY pin HIGH; relay not energised
  digitalWrite(LED, HIGH);                  // turn LED off
  digitalWrite(sendLED, HIGH);
  digitalWrite(receiveLED, HIGH);
  
  Serial.begin(115200);
  timer= millis();                          // initialise timer
  while (!Serial) delay(1);
  set_topics();  
    
  #ifdef Ubidots                            // Read 'profile.h' on #ifdef       
     setup_wifi();
     client.setServer(mqtt_server, mqtt_port);
     client.setCallback(callback);
     client.setKeepAlive(keepalive);
  #endif
}

void loop() {
 
 #ifdef Ubidots                             // Read 'profile.h' on #ifdef   
    if (!client.connected()) reconnect();   
    client.loop();                          // for processing callback messages ie. subscribe messages
 #endif

 if (digitalRead(Button) == HIGH) //check if BYPASS BUTTON is pressed at any time
  {
    digitalWrite(RELAY, LOW);
    tone(Buzzer, 700);
    delay(10000);
    digitalWrite(RELAY, HIGH);
    noTone(Buzzer);
  }

 if (millis() > timer){                     // non-blocking delay check timer
  
  /*** read temp ***/
  float sensor = analogRead(temp)+ 150 ;   // Read Va binary code; compensate ESP32 ADC offset error, ~150
  Serial.print("Sense binary code:");                       
  Serial.print(sensor);                                     
  
  sensor = sensor * 0.0008;               // Convert binary to voltage by x3.3/4095 ( = x0.0008)
  Serial.print(" / Sense Voltage Va (V):");                 
  Serial.print(sensor);
  Serial.print(" TEMP:");
  float tempoutput = (sensor - 0.4) / 0.02;
  Serial.print(tempoutput);

  Serial.print("Set temperature: ");
  float potsetting = (analogRead(potpin)- 280.78266) / 26.95652;
  Serial.print(potsetting);

  Threshold = potsetting;

  if (tempoutput > Threshold) //Check if temperature output is greater than threshold set by pot
  {                                  
    Serial.println(" High temperature. Relay has been activated.");
    digitalWrite(RELAY, LOW);
    tone(Buzzer, 700);                                                            
  }

 
  else 
  {                                                                                                        
    digitalWrite(RELAY, HIGH);  
    noTone(Buzzer);                                                                                           
  }

  
  

  DynamicJsonDocument doc(256);           // JSON format eg. {"obj1-label":value,"obj2-label":value,...}
  doc["dev"] = clientid;                  // first object eg. device name or id
  doc["Temperature"] = tempoutput;                    // second object eg. sense variable
  doc["SetTemperature"] = potsetting;
  
  char mqtt_message[256];
    serializeJson(doc,mqtt_message);

  #if defined(Ubidots) && defined(Publish)
    client.publish(publish_topic, mqtt_message, true);
    Serial.println("Message published ["+String(publish_topic)+"] - "+String(mqtt_message));
    digitalWrite (sendLED, LOW);
    delay(50);
    digitalWrite(sendLED, HIGH);
  #else
    Serial.println("JSON Message - "+String(mqtt_message));
  #endif
    
  timer = millis() + max(mintxinterval, 10000);   // non-blocking delay set next tx timer in ms
 }
}

/***** Call back Method for Receiving MQTT messages and Switching output ****/
void callback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);                     //copy incoming payload to p
  p[length] = NULL;
  float sub_payload = atof(p);                    //convert p to float sub_payload
  Serial.println("Message arrived ["+String(topic)+"]"+sub_payload);
  digitalWrite (receiveLED, LOW);
  delay(50);
  digitalWrite(receiveLED, HIGH);

  /* check incoming message */
    if( strcmp(topic,subscribe_topic) == 0){      //if incoming topic == subscribe_topic
     /* use of payload to define action */
      if (sub_payload < Threshold) 
      {
      digitalWrite(RELAY, HIGH);
      }                           
      else 
      {
      digitalWrite(RELAY, HIGH);
      }
    }                          
}

/************* Connect to WiFi ***********/
void setup_wifi() {
  delay(10);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
}

/************* Connect to MQTT Broker ***********/
void reconnect() {
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientid, mqtt_username, mqtt_password)) {
      //Serial.println("connected");
       #if defined(Ubidots) && defined(Subscribe)
         client.subscribe(subscribe_topic);   // subscribe the topics here
       #endif
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");   // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void set_topics(){
  sprintf(publish_topic, "/v1.6/devices/%s", clientid);
  sprintf(subscribe_topic, "/v1.6/devices/%s/%s/lv",lev2,lev3);
}
