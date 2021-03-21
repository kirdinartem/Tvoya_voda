/*
  Project: Tvoya Voda. Data exchange with server via GSM GPRS GPS module A7
  Function: Requesting available quantity of water for user. Sending conditions of water machine.
  Using Arduino Mega and GSM GPRS GPS module A7
  GND           GND
  GSM_RX        D10 (Software Serial RX)
  GSM_TX        D11 (Software Serial TX)
*/

#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <String.h>

#define OK 1
#define NOTOK 2
#define TIMEOUT 3
#define RST 2
#define BUFFLENGTH 256
#define FLOW_SENSOR 2
#define MOTOR 6
#define WATER_VALVE 7

volatile int  flow_frequency = 0;
unsigned long currentTime = 0;
unsigned long cloopTime = 0;
unsigned int liters = 0;

SoftwareSerial GSMmodule(10, 11); // RX, TX
char serverResponse[BUFFLENGTH];  // Buffer for a server response 

void setup()
{
  /*SETUP COMMUNICATION*/
  Serial.begin(9600);       
  GSMmodule.begin(9600);    
  delay(2000);                    // Give time to GSM module logon to GSM network
  Serial.println("GSMmodule ready...");
  /*SETUP HARDWARE*/
  Sensors_init();
  Devices_init();
  /*SETUP GSM*/
  GSMmodule_init();
}

void loop()
{
  //sendToServer("/Thingworx/Things/SmartLabIotT/Services/SmartLabIot/", "");
  task_list();
  //task_update();
  //system_update();
  //system_error();
}

void Sensors_init()
{
  pinMode(FLOW_SENSOR, INPUT);
  unsigned char flow_pin = digitalPinToInterrupt(FLOW_SENSOR);
  attachInterrupt(flow_pin, flow, RISING);
  sei();
  currentTime = millis();
  cloopTime = currentTime;
}

void Devices_init()
{
  pinMode(MOTOR, OUTPUT);
  pinMode(WATER_VALVE, OUTPUT);
  digitalWrite(MOTOR, LOW);
  digitalWrite(WATER_VALVE, LOW);  
}

void GSMmodule_init() 
{
  /*PINCODE ENTERING*/  
  Serial.println("Entering pincode");
  GSMmodule.write("AT+CPIN=0301\r");
  if (A7waitFor("OK", "yy", 10000) == OK)
    Serial.println("pincode accepted");
  else
    Serial.println("some shit happened with pincode");
  
  /*REQUEST OF CURRENT STATE OF GSM CONNECTION*/
  if (A7command("AT+CIPSTATUS", "OK", "yy", 10000, 2) == OK) {  
    Serial.println("CIPSTATUS OK");
  }
  Serial.println();

  /*SETUP GPRS CONNECTION*/
  if (A7command("AT+CGATT=1", "OK", "yy", 10000, 2) == OK) {  
    Serial.println("GPRS connection OK");
  }
  Serial.println();  

  /*SETUP PARAMETERS OF PDP CONTEXT, BRING UP WIRELESS CONNECTION (RETURNS OK ONLY IN THE FIRST CALL)*/
  if (A7command("AT+CGDCONT=1,\"IP\",\"internet\"", "OK", "yy", 10000, 1) == OK) { 
    Serial.println("PDP context setup OK");
  }
  Serial.println();

  /*ACTIVATION OF PDP CONTEXT*/
  if (A7command("AT+CGACT=1,1", "OK", "yy", 10000, 2) == OK) {  
    Serial.println("PDP context activation OK");
  }
  Serial.println();
}


/*Function for sending HTTP POST request to server*/
void sendToServer(String post_dst, String post_body) 
{
  GSMmodule.flush();
  char end_c[2];
  end_c[0] = 0x1a;
  end_c[1] = '\0';
  String host   = "194.85.169.93";
  String appKey = "564ad60c-111f-4720-8ffb-1004d7022ba4";
  
  if (A7command("AT+CIPSTART=\"TCP\",\"" + host + "\",80", "CONNECT OK", "yy", 25000, 2) == OK) { // Start up the connection
    Serial.println("Connect to server OK");
  }
  Serial.println();

  /*POST request for transmitting*/
  A7command("AT+CIPSEND", ">", "yy", 10000, 1); 
  delay(500);
  GSMmodule.print("POST ");
  GSMmodule.print(post_dst);
  GSMmodule.print("?appKey=");
  GSMmodule.print(appKey);
  GSMmodule.print("&method=post&x-thingworx-session=true");
  GSMmodule.print("&temp=");
  GSMmodule.print(666);
  GSMmodule.print(" HTTP/1.1");
  GSMmodule.print("\r\n");
  GSMmodule.print("Accept: application/json");
  GSMmodule.print("\r\n");
  GSMmodule.print("HOST: ");
  GSMmodule.print(host);
  GSMmodule.print("\r\n");
  GSMmodule.print("Content-Type: application/json");
  GSMmodule.print("\r\n");
  GSMmodule.print("Conection: closed");
  GSMmodule.print("\r\n");
  GSMmodule.print("\r\n");
  GSMmodule.print(post_body);

  Serial.print("POST /Thingworx/Things/SmartLabIotT/Services/SmartLabIot/");
  Serial.print("?appKey=");
  Serial.print(appKey);
  Serial.print("&method=post&x-thingworx-session=true");
  Serial.print("&temp=");
  Serial.print(666);
  Serial.print(" HTTP/1.1");
  Serial.print("\r\n");
  Serial.print("Accept: application/json");
  Serial.print("\r\n");
  Serial.print("HOST: ");
  Serial.print(host);
  Serial.print("\r\n");
  Serial.print("Content-Type: application/json");
  Serial.print("\r\n");
  Serial.print("Conection: closed");
  Serial.print("\r\n");
  Serial.print("\r\n");
  Serial.print(post_body);
    
  if (A7command(end_c, "HTTP/1.1", "yy", 30000, 1) == OK){ // Begin send data to remote server using symbol ctrl+C
    Serial.println("THERE IS RESPONSE");
    Serial.println(serverResponse);
  }
  else
    Serial.println("THERE IS NO RESPONSE");
  
  parseJsonResponse();
  
  unsigned long   entry = millis();
  A7command("AT+CIPCLOSE", "OK", "yy", 15000, 1); 
  delay(100);
  Serial.println("-------------------------End------------------------------");
}

String getTime()
{
  String reply;
  GSMmodule.println("AT+CCLK?");
  reply = A7read();
}

void startPour(int amount)
{
  /*Checking current amount of poured water*/
  while (liters < amount){
    /*Open water valve and start motor*/
    digitalWrite(WATER_VALVE, HIGH); 
    digitalWrite(MOTOR, HIGH);
  }
  /*Close water valve and off motor*/
  digitalWrite(WATER_VALVE, LOW); 
  digitalWrite(MOTOR, LOW);
}

/*Function which parse Json response from server*/
void parseJsonResponse()
{
  int request_type = 1;
  StaticJsonDocument<BUFFLENGTH> root;
  auto error = deserializeJson(root, serverResponse);
  if (error) Serial.println("Failure");
  switch(request_type)
  {
  case 1:
    /*Check if there's no new task*/
    if (root["task_water"] == "null"){
      break;
    }
    /*Else we have a new task*/
    String id = root["task_water"]["id"];
    int amount = root["task_water"]["amount"];
    String sys_status = root["task_water"]["status"];
    startPour(amount);
    Serial.println(id);
    Serial.println(amount);
    Serial.println(sys_status);
    break;  
  case 2:
    int u = root["d4"];
    break;
  }  
}

void task_list()
{
  //String JSON_msg = "request:{\"id\": \"controller1\", \"token\": \"333333dddd3333\", \"timestamp\": \"2020-12-10T03:33:33.000\", \"request_id\": \"ksjfklsdjlkfsf\" }"
  char json_msg[BUFFLENGTH];
  StaticJsonDocument<BUFFLENGTH> doc;
  doc["id"] = "controller1";
  doc["token"] = "333333dddd3333";
  doc["timestamp"] = "2020-12-10T03:33:33.000";
  doc["request_id"] = "ksjfklsdjlkfsf";
  auto error = serializeJson(doc, json_msg, BUFFLENGTH);
  if(error) Serial.println("Error in task_list"); 

  sendToServer("/Thingworx/Things/SmartLabIotT/Services/SmartLabIot/", json_msg);

}

void task_update()
{
  char json_msg[BUFFLENGTH];
  String timestamp;
  
  
  StaticJsonDocument<BUFFLENGTH> doc;
  doc["id"] = "controlller1";
  doc["token"] = "333333dddd3333";
  doc["timestamp"] = "2020-12-10T03:33:33.000";
  doc["request_id"] = "ksjfklsdjlkfsf";
  
  JsonObject task_water = doc.createNestedObject("task_water");
  task_water["id"] = 55;                    
  task_water["amount"] = 5000;              
  task_water["status"] = "complete/error";  
  task_water["error"] = "not enough water"; 

  auto error = serializeJson(doc, json_msg, BUFFLENGTH);
  if(error) Serial.println("Error in task_list"); 

  sendToServer("/Thingworx/Things/SmartLabIotT/Services/SmartLabIot/", json_msg);
}

void system_update()
{
  char json_msg[BUFFLENGTH];
  
  StaticJsonDocument<BUFFLENGTH> doc;
  doc["id"] = "controlller1";
  doc["token"] = "333333dddd3333";
  doc["timestamp"] = "2020-12-10T03:33:33.000";
  doc["request_id"] = "ksjfklsdjlkfsf";

  JsonArray sensors = doc.createNestedArray("sensors");
  
  JsonObject sensors1 = sensors.createNestedObject();
  sensors1["name"] = "manometer1";
  JsonObject data1 = sensors1.createNestedObject("data");
  data1["pressure"] = 20;
  
  JsonObject sensors2 = sensors.createNestedObject();
  sensors2["name"] = "tank1";
  JsonObject data2 = sensors2.createNestedObject("data");
  data2["water"] = 20;
  
  auto error = serializeJson(doc, json_msg, BUFFLENGTH);
  if(error) Serial.println("Error in task_list"); 

  sendToServer("/Thingworx/Things/SmartLabIotT/Services/SmartLabIot/", json_msg);
}

void system_error()
{
  char json_msg[BUFFLENGTH];
  
  StaticJsonDocument<BUFFLENGTH> doc;
  doc["id"] = "controlller1";
  doc["token"] = "333333dddd3333";
  doc["timestamp"] = "2020-12-10T03:33:33.000";
  doc["request_id"] = "ksjfklsdjlkfsf";

  JsonObject error1 = doc.createNestedObject("error");
  error1["pressure"] = 20;
  
  auto error = serializeJson(doc, json_msg, BUFFLENGTH);
  if(error) Serial.println("Error in task_list"); 

  sendToServer("/Thingworx/Things/SmartLabIotT/Services/SmartLabIot/", json_msg);
}
