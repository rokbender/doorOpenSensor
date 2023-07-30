#include <MQTT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
#include <EEPROM.h>

const char *ssid =  "";  
const char *pass =  ""; 

const char *mqtt_server = ""; 
const int mqtt_port = 1883; 
const char *mqtt_user = ""; 
const char *mqtt_pass = ""; 

#define BUFFER_SIZE 100
#define pinWake 5
#define InputPin 4

int attempsWifi=0;
int attempsMQTT=0;
int attemps=2;
int State1; // 0-закрита,1-відкрита на недовгий час,2-відкрита

float vout = 0.0;           //
float vin = 0.0;            //
float R1 = 330000.0;          // сопротивление R1 (10K)
float R2 = 99200.0;         // сопротивление R2 (1,5K)
int val=0;
#define analogvolt A0

int adress=1; //номер комірки памяті
int numNewMessages;
bool flagVolt=false;

WiFiClient wclient;      
PubSubClient client(wclient, mqtt_server, mqtt_port);


#define BOTtoken ""  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID ""  ////-720404657-групи 523186785


#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client2;
UniversalTelegramBot bot(BOTtoken, client2);

// Checks for new messages every 1 second.
int botRequestDelay = 1500;

unsigned long lastTimeBotRan;


int detectRequestDelay = 2000;
unsigned long lastTimedetect=0;

bool alarm;

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/startgatebot") {
      String welcome = "Привіт, " + from_name + ".\n";
      welcome += "Чим я можу допомогти ? \n";
       welcome += "Це набір команд для керування: \n";
      welcome += "/gatedoor_on включити сигналізацію \n";
      welcome += "/gatedoor_off виключити сигналізацію \n";
      welcome += "/gatedoor_state  статус сигналізації  \n";
      welcome += "/gatedoor_volt  Напруга акумулятора  \n";
      bot.sendMessage(chat_id, welcome, "");
    }
   if (text == "/gatedoor_off") {
      bot.sendMessage(chat_id, "Сигналізацію виключено", "");
      alarm=false;
      EEPROM.put(adress, alarm);   //запишем спочатку true в коміру памяті потім закомкентуєм
      EEPROM.commit();
         }
 if (text == "/gatedoor_on") {
      bot.sendMessage(chat_id, "Сигналізацію включено", "");
      alarm=true;
      EEPROM.put(adress, alarm);   //запишем спочатку true в коміру памяті потім закомкентуєм
      EEPROM.commit();
         }
       if (text == "/all_off") {
      bot.sendMessage(chat_id, "Всю сигналізацію виключено", "");
      alarm=false;
      EEPROM.put(adress, alarm);   //запишем спочатку true в коміру памяті потім закомкентуєм
      EEPROM.commit();
         }
         if (text == "/all_on") {
      bot.sendMessage(chat_id, "Всю сигналізацію виключено", "");
      alarm=false;
      EEPROM.put(adress, alarm);   //запишем спочатку true в коміру памяті потім закомкентуєм
      EEPROM.commit();
         }
    if (text == "/gatedoor_volt") {
      bot.sendMessage(chat_id, "Надсилаю напругу акумулятора", "");
      flagVolt=true;
      Volt();
         }
     
    
    if (text == "/state") {
      if (alarm){
      bot.sendMessage(chat_id, "Сигналізація включена", "");
      }
      else{
          bot.sendMessage(chat_id, "Сигналізація виключена", "");
      }
      
     
    }
  }
}

// Функция получения данных от сервера

void callback( const MQTT ::Publish& pub)
{
  Serial.print(pub.topic());   // выводим в сериал порт название топика
  Serial.print(" => ");
  Serial.print(pub.payload_string()); // выводим в сериал порт значение полученных данных
  
  String payload = pub.payload_string();
  
   
}




void setup() {
  
  pinMode(pinWake,OUTPUT);
  digitalWrite(pinWake,HIGH);
  pinMode(InputPin,INPUT);
  //pinMode(analogvolt,INPUT);
  pinMode(LED_BUILTIN, OUTPUT); 
  digitalWrite(LED_BUILTIN, LOW);
  delay(10);
  Serial.begin(115200);
  EEPROM.begin(512);
  //EEPROM.put(adress, 1);   //запишем спочатку true в коміру памяті потім закомкентуєм
 // EEPROM.commit();
  alarm= EEPROM.read(adress);
  delay(100);
  
 
   configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
   client2.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
   WiFi.mode(WIFI_STA);
   delay(500);
  // Connect to Wi-Fi
 
  client2.setInsecure(); 
  // Print ESP32 Local IP Address
  
}

void loop() {
  Serial.print("alarm=");
  Serial.println(alarm);
  // подключаемся к wi-fi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, pass);
     
    if (attempsWifi>attemps){
    Sleep1();
    }
    if (WiFi.waitForConnectResult() != WL_CONNECTED){
       delay(1000);
       attempsWifi+=1;
       return;
      }
      
    Serial.println("WiFi connected");
    
    Serial.println(WiFi.localIP());
  }
   if (millis() > lastTimeBotRan + botRequestDelay)  {
    
   delay(100);
   int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      
      
    }
      
    lastTimeBotRan = millis();
   }
  if(alarm){
      bot.sendMessage(CHAT_ID, "Виявлено відкриття хвіртки.", "");
      Serial.println("here////");
      alarm=false;
    }
  Volt();
  // подключаемся к MQTT серверу
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.println("Connecting to MQTT server");
      if (client.connect(MQTT::Connect("GATE2") // назва пристрою що підкл тобто ESp8266
                         .set_auth(mqtt_user, mqtt_pass))) {
        Serial.println("Connected to MQTT server");
        client.set_callback(callback);
      } else {
        Serial.println("Could not connect to MQTT server");
        attempsMQTT+=1;
        delay(500);
        if (attempsMQTT>attemps){
           Sleep1();
          }   
      }
    }
    
    if (client.connected()){
      client.loop();
      State1=1;
      SendData();
      delay(4500);
      Serial.println(vin);
      client.publish("GateDoor/voltage",String(vin)); // отправляем в топик для термодатчика значение температуры
      Serial.println("SENDVolt...");
      if(digitalRead(InputPin)==LOW){
        State1=0;    //Фіртка закрита
        Serial.println("close");
      }
      else{
        State1=2; //Фіртка відкрита
        Serial.println("open");
      }
      
      SendData();
      delay(1500);
      Sleep1();
  }
  
}
  
} // конец основного цикла


// Функция отправки показаний с термодатчика
void SendData(){
    Serial.println(State1);
    client.publish("GateDoor/state",String(State1)); // отправляем в топик для термодатчика значение температуры
    Serial.println("SEND...");
    
   
}
void Sleep1(){
  Serial.println("SleepNow");
  digitalWrite(pinWake,LOW);
  delay(100);
  ESP.deepSleep(0);
  //delay(3000);
  }

  void Volt(){
  delay(100);
  val = analogRead(analogvolt);
  Serial.println(val);
  if (val>0){
     vout = (val * 1.0) / 1024.0;
     vin = (vout / (R2 / (R1 + R2)))*0.984;
    }
   else{
    vin=0.0;
    } 
    //vin=3.3;
  if (vin<3.3){
     bot.sendMessage(CHAT_ID, "Акумулятор розряджений.Потрібна підзарядка!", "");
    }
  if (flagVolt){
    String text="Напруга U=" + String(vin, 2);;
    bot.sendMessage(CHAT_ID, text, "");
    Serial.println("SENDVolt...");
    flagVolt=false;
  }
  
}
