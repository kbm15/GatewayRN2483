#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <ArduinoJson.h>

#define RXD2 16
#define TXD2 17
#define LORARX "radio_rx"

char* msg = (char *)malloc(sizeof(char) * 512);;
char* json = (char *)malloc(sizeof(char) * 2010);
const char* ssid = "FOSCAM";
const char* password = "HOWLAB2020";
const char* mqtt_server = "155.210.139.83";

//Variables WiFi
WiFiClient espClient;
PubSubClient client(espClient);
long lastReconnectAttempt = 0;

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
  //Serial.print(msg);
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
  setup_wifi();
  timeClient.begin();
  client.setServer(mqtt_server, 1983);
  client.setCallback(callback);
  client.setBufferSize(2048);
  if(Serial2.available()) {
    readLora();
  }
  // writeLora("sys factoryRESET\r\n");
  // readLora();
  writeLora("mac pause\r\n");
  readLora();
  writeLora("radio set freq 869525000\r\n");
  readLora();
  writeLora("radio set bw 250\r\n");
  readLora();
  writeLora("radio set sf sf7\r\n");
  readLora();
  writeLora("radio set sync 12\r\n");
  readLora();

  
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
    
    char* jsonPnt = json;
    char c[5];
    int16_t values[3];    
    long epoch = timeClient.getEpochTime();
    int mSec = 0;
    jsonPnt+=sprintf(jsonPnt, "[");
    for(int i = 0; i < 20; i++){
      mSec = i*10;
      token+=sprintf( c, "%c%c%c%c", token[0], token[1], token[2], token[3] );
      values[0] = strtol(c,NULL,16);
      token+=sprintf( c, "%c%c%c%c", token[0], token[1], token[2], token[3] );
      values[1] =strtol(c,NULL,16);
      token+=sprintf( c, "%c%c%c%c", token[0], token[1], token[2], token[3] );
      values[2] =strtol(c,NULL,16);
      jsonPnt+=sprintf(jsonPnt, "{\"ts\":%ld%d,\"data\":{\"x\":%d,\"y\":%d,\"z\":%d}},",  epoch, mSec, values[0], values[1], values[2]);
    }  
    sprintf(jsonPnt-1, "]");    
    Serial.println("Rx received");
    Serial.println(client.publish("lora", json));
    Serial.println(client.getBufferSize());
  }
}

