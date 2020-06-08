#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <ArduinoJson.h>

#define RXD2 16
#define TXD2 17
#define LORARX "radio_rx"

char msg[500];
const char* ssid = "FOSCAM";
const char* password = "HOWLAB2020";
const char* mqtt_server = "155.210.139.83";

//Variables WiFi
WiFiClient espClient;
PubSubClient client(espClient);
long lastReconnectAttempt = 0;
const size_t capacity = JSON_ARRAY_SIZE(37) + 37*JSON_OBJECT_SIZE(2) + 37*JSON_OBJECT_SIZE(3) + 660;
DynamicJsonDocument doc(capacity);
JsonObject obj[37];
JsonObject val[37];

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

//config wifi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//reconectar mqtt
boolean reconnect() {
  if (client.connect("espClient")) {
    // Once connected, publish an announcement...
    client.publish("outTopic","hello world");
  }
  return client.connected();
}

//callback mqtt
void callback(char* topic, byte* payload, unsigned int length) {
}

void readLora(){
  char lastRead = 0;  
  char* buffPoint = msg;
  while(lastRead != '\n'){
    if(Serial2.available()) {
      lastRead = char(Serial2.read());
      buffPoint += sprintf(buffPoint, "%c", lastRead);
      // Serial.print(lastRead);
    } else{    
      delay(1);
    }
  }
}


void writeLora(String cmd){
  Serial.print(cmd);
  Serial2.print(cmd);
}

int64_t xx_time_get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}
 
void setup() {
  Serial.begin(115200);
  Serial2.begin(57600, SERIAL_8N1, RXD2, TXD2); 

  delay(1000);
  readLora();
  writeLora("sys factoryRESET\r\n");
  readLora();
  writeLora("mac pause\r\n");
  readLora();
  writeLora("radio set freq 869525000\r\n");
  readLora();
  writeLora("radio set bw 250\r\n");
  readLora();
  writeLora("radio set sf sf7\r\n");
  readLora();

  setup_wifi();
  timeClient.begin();
  client.setServer(mqtt_server, 1983);
  client.setCallback(callback);
  client.setBufferSize(512);
}
 
void loop() { 
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  long now = millis();
  if (!client.connected()) {
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected    
    client.loop();    
  }
  writeLora("radio rx 0\r\n");
  readLora();
  readLora();
  if(strstr(msg, LORARX)){
    char * token = strtok(msg, " ");
    token = strtok(NULL, " ");
    // Serial.println(token);
    // char x[2], y[2], z[2];  
    // for(int i = 0; i<37; i++){
    //   for(int j = 0; j<2; j++){
    //     x[j]= token[j+i*6];
    //     y[j]= token[j+2+i*6];
    //     z[j]= token[j+4+i*6];
    //   }
    //   JsonObject obj[i] = doc.createNestedObject();
    //   obj[i]["ts"] =  timeClient.getEpochTime()*1000;

    //   JsonObject val[i] = obj[i].createNestedObject("values");
    //   val[i]["x"] = strtol(x,NULL,16);
    //   val[i]["y"] = strtol(y,NULL,16);
    //   val[i]["z"] = strtol(z,NULL,16);
    // }
    // serializeJson(doc, Serial);
    Serial.println("Rx received");
    client.publish("lora", token);
  }
}

