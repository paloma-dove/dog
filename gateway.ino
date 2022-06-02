/*
  LoRaNow Simple Gateway with ESP32 setPins
  This code creates a webServer show the Message and use mqtt publish the data.
  created 27 04 2019
  by Luiz H. Cassettari
  modified 2020.09.19
  by bloomlj
*/

#include <LoRaNow.h>
#include <WiFi.h>
#include <WebServer.h>
#include <StreamString.h>
#include <PubSubClient.h>

//vspi
#define MISO 19
#define MOSI 23
#define SCK 18
#define SS 5

//hspi,only for ESP-32-Lora-Shield-Ra02
//#define SCK 14
//#define MISO 12
//#define MOSI 13
//#define SS 15

#define DIO0 4

const char *ssid = "H3C_5774EA";
const char *password = "t125001t125001";
//const char *mqtt_server = "broker.hivemq.com";
// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
//const char *mqtt_broker = "127.0.0.1";
const char *topic = "esp32/test";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;
//for our data output loop
char buff[50];
int car_speed=10;
 
WebServer server(80);
WiFiClient espClient;
//PubSubClient mqttclient(espClient);
PubSubClient client(espClient);

const char *script = "<script>function loop() {var resp = GET_NOW('loranow'); var area = document.getElementById('area').value; document.getElementById('area').value = area + resp; setTimeout('loop()', 1000);} function GET_NOW(get) { var xmlhttp; if (window.XMLHttpRequest) xmlhttp = new XMLHttpRequest(); else xmlhttp = new ActiveXObject('Microsoft.XMLHTTP'); xmlhttp.open('GET', get, false); xmlhttp.send(); return xmlhttp.responseText; }</script>";

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (60)
char msg[MSG_BUFFER_SIZE];

int value = 0;

void handleRoot()
{
  String str = "";
  str += "<html>";
  str += "<head>";
  str += "<title>Temperature monitor</title>";
  str += "<style type=\"text/css\">body{background: url(https://img.zcool.cn/community/015ea357afda5f0000018c1b2f966d.jpg@1280w_1l_2o_100sh.jpg) repeat-x center top fixed}</style>";
  str += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  str += script;
  str += "</head>";
  str += "<body onload='loop()'>";
  str += "<center>";
  str += "<textarea id='area' style='width:800px; height:400px;'></textarea>";
  str += "</center>";
  str += "</body>";
  str += "</html>";
  server.send(200, "text/html", str);
}

static StreamString string;

void handleLoRaNow()
{
  server.send(200, "text/plain", string);
  while (string.available()) // clear
  {
    string.read();
  }
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("有新消息[");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    //digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    //digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void setup(void)
{

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  //if (ssid != "")
    WiFi.begin(ssid, password);
  WiFi.begin();
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("已经成功连接到：");
  Serial.println(ssid);
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/loranow", handleLoRaNow);
  server.begin();
  Serial.println("已启用HTTP服务器");

  LoRaNow.setFrequencyCN(); // Select the frequency 486.5 MHz - Used in China
  // LoRaNow.setFrequencyEU(); // Select the frequency 868.3 MHz - Used in Europe
  // LoRaNow.setFrequencyUS(); // Select the frequency 904.1 MHz - Used in USA, Canada and South America
  // LoRaNow.setFrequencyAU(); // Select the frequency 917.0 MHz - Used in Australia, Brazil and Chile

  // LoRaNow.setFrequency(frequency);
  // LoRaNow.setSpreadingFactor(sf);
  // LoRaNow.setPins(ss, dio0);

  LoRaNow.setPinsSPI(SCK, MISO, MOSI, SS, DIO0); // Only works with ESP32

  if (!LoRaNow.begin())
  {
    Serial.println("LoRa加载失败。检查你的连接。");
    while (true)
      ;
  }

  LoRaNow.onMessage(onMessage);
  LoRaNow.gateway();
  //mqtt
  //mqttclient.setServer(mqtt_broker, 1883);
  //mqttclient.setCallback(callback);
 client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
         Serial.println("Public emqx mqtt broker connected");
     } else {
         Serial.println("failed with state ");
         Serial.println(client.state());
         delay(2000);
     }
 }
   // publish and subscribe
 client.publish(topic, "Hi EMQ X I'm ESP32 ^^");
 client.subscribe(topic);
}

void loop(void)
{
  LoRaNow.loop();
  server.handleClient();
  client.loop();
  //mqttloop();
}

void onMessage(uint8_t *buffer, size_t size)
{
  int i=35,j=0,k=0;
  char tmp[6];
  unsigned long id = LoRaNow.id();
  byte count = LoRaNow.count();
  Serial.print("节点ID: ");
  Serial.println(id, HEX);
  Serial.print("接收次数: ");
  Serial.println(count);
  Serial.print("信息内容: ");
  Serial.write(buffer, size);
  while ('0'>=buffer[i]||'9'<=buffer[i])i++;
  j=i+5;
  for(i;i<=j;i++,k++) tmp[k]=buffer[i];
  tmp[k-1]='\0';
  Serial.println();
  Serial.println();
   //此处通过mqtt发送信息。
   snprintf (msg, MSG_BUFFER_SIZE, "#%d#%s", count,buffer);
   Serial.print("发布信息: ");
   Serial.println(msg);
   client.publish("Topic", msg);

  if (string.available() > 512)
  {
    while (string.available())
    {
      string.read();
    }
  }

  string.print("节点ID: ");
  string.println(id, HEX);
  string.print("接收次数: ");
  string.println(count);
  string.print("信息内容: ");
  string.write(buffer, size);
  string.println();
  string.println();

  // Send data to the node
  LoRaNow.clear();
  LoRaNow.print("LoRaNow 网关信息");
  LoRaNow.print(millis());
  LoRaNow.send();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("尝试与MQTT服务器进行连接");
    // Create a random mqttclient ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("连接成功");
      // Once connected, publish an announcement...
      client.publish("Topic", "hello world");
      // ... and resubscribe
      client.subscribe("Topic");
    } else {
      Serial.println("失败, 错误代码：");
      Serial.println(client.state());
      Serial.println("。5秒后再次尝试。");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*void loop() {

  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

//  unsigned long now = millis();
//  if (now - lastMsg > 2000) {
//    lastMsg = now;
//    ++value;
//    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    mqttclient.publish("C2TLOutTopic", msg);
//  }
}*/
