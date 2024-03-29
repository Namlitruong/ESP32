#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define mqttServer "192.168.137.69"
//#define mqttServer "broker.hivemq.com "
#define WiFiSSID "thao"
#define WiFiPass "thao12345"
#define PubTopic "ESP32"

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

MFRC522 mfrc522(5, 13); 

void TaskBlink( void *pvParameters );
void TaskMQTT( void *pvParameters );

TaskHandle_t Task2Blink;
TaskHandle_t Task1MQTT;
 
WiFiClient espClient;
PubSubClient client(espClient);

//#######################################
//----------GLOBAL VARIABLES------------
char Data[10] = "";
//#######################################

void setup() {
  
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  // Now set up two tasks to run independently.
  xTaskCreatePinnedToCore(
    TaskBlink
    ,  "TaskBlink"   // A name just for humans
    ,  16384  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &Task2Blink 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskMQTT
    ,  "TaskMQTT"
    ,  16384  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  &Task1MQTT 
    ,  ARDUINO_RUNNING_CORE);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop()
{
  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskBlink(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  SPI.begin(); 
  mfrc522.PCD_Init();
  SemaphoreHandle_t RFIDMutex;
  RFIDMutex = xSemaphoreCreateMutex();
  for (;;) // A Task shall never return or exit.
  {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    xSemaphoreTake(RFIDMutex,portMAX_DELAY);
    sprintf(Data, "%u", RFIDCard ());
    Serial.println (PublishMQTT (PubTopic, 0,Data, "/0", "/0")); 
    xSemaphoreGive(RFIDMutex);
    }
  }
}

void TaskMQTT(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  SemaphoreHandle_t RFIDMutex;
  RFIDMutex = xSemaphoreCreateMutex();
  vTaskSuspend(Task2Blink);
  ConnectToWiFi (WiFiSSID, WiFiPass);
  client.setServer(mqttServer, 1883);
  client.setCallback (callback);
  ConnectMQTT(PubTopic, "Center610", 1);
  vTaskResume(Task2Blink);
  for (;;)
  {
    if (!client.connected()) {
      ConnectMQTT(PubTopic, "Center610", 1);
    }
    client.loop();
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String arrivedData;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    arrivedData += (char)payload[i];
  }
  Serial.println();
  Serial.println("########################");
  Serial.println(arrivedData);
  Serial.println("########################");
  Serial.println();
}

void ConnectMQTT(const char* CliendID, const char* SubTopic, int qos) {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println ("Attempting MQTT connection...");
    // Attempt to connect, just a name to identify the client
    if (client.connect(CliendID)) {
      Serial.println("connected");
      client.subscribe (SubTopic, qos);
      // Once connected, publish an announcement...
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      vTaskDelay(5000);
    }
  }
}
int ConnectToWiFi (const char* ssid, const char* password){
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(100);
    Serial.print (".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  return WiFi.status();
}

const char* PublishMQTT (const char *ClientID,int SendingInterval ,char *str1, char *str2, char *str3){
  char *Buffer = (char *) malloc(1 + strlen(str1)+ strlen(str2)+ strlen(str3));
  strcpy(Buffer, str1);
  strcat(Buffer, ";");
  strcat(Buffer, str2);
  strcat(Buffer, ";");
  strcat(Buffer, str3);
  client.publish( ClientID, Buffer);
  vTaskDelay(SendingInterval);
  Buffer = NULL;
  free (Buffer);
  return "Sending Data";
}

unsigned long RFIDCard (){
  unsigned long UID = 0, UIDtemp = 0;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    UIDtemp = mfrc522.uid.uidByte[i];
    UID = UID*256+UIDtemp;
  }
  mfrc522.PICC_HaltA(); 
  return UID;
}
