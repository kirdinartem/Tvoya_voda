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

SoftwareSerial GPSmodule(10, 11); // RX, TX
char serverResponse[BUFFLENGTH];  // Buffer for a server response 


void setup()
{
  /*SETUP COMMUNICATION*/
  Serial.begin(9600);       
  GPSmodule.begin(9600);    
  delay(2000);                    // Give time to GSM module logon to GSM network
  Serial.println("GPSmodule ready...");
  
  GPSmodule_init();
}

void loop()
{
  //sendToServer("/Thingworx/Things/SmartLabIotT/Services/SmartLabIot/", "");
  //task_list();
  //task_update();
  //system_update();
  system_error();
}

/*FIRST INIT*/
void GPSmodule_init() 
{
  /*PINCODE ENTERING*/  
  Serial.println("Entering pincode");
  GPSmodule.write("AT+CPIN=0301\r");
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

/*Function for reading of input stream from GSM module*/
String A7read() {
  String reply = "";
  while(GPSmodule.available())  {
    reply = GPSmodule.readString();
  }
  return reply;
}


/*Function for reading of HTTP response from server*/
byte A7readResponse() {
  char c;
  byte startJSON = 0;
  char response_status[3];  // for example 200, 400 or 404
  bool response_flag = false;
  int server_status = 0;
  char http_word[9] = {'H', 'T', 'T', 'P', '/', '1', '.', '1', ' '};
  int i = 0;
  int j = 0;
  int k = 0;
  
  while(GPSmodule.available())  
  {
    c = char(GPSmodule.read());
    
    /*CHECKING RESPONSE STATUS FROM SERVER*/
    if (c == http_word[j] && startJSON == 0)
    {
      j++;
      if (j == sizeof(http_word)/sizeof(http_word[0])){ // It's place to catch response status from server (HTTP/1.1 ...)
        j = 0;
        response_flag = true;
      }
      continue;
    }
    else j = 0;
 
    if (response_flag)
    {
      response_status[k] = c;
      k++;
      if (k == 3){
        response_flag = false;  // status has been recieved
        server_status = atoi(response_status);
        Serial.print("SERVER RESPONSE -> ");
        Serial.println(response_status);
        delay(10);
      }
    }
    /**************************************/
    
    /*SEARCHING FOR JSON MESSAGE IN SERVER RESPONSE*/
    if (c == '{') 
    {
      startJSON++;
    }
    if (c == '}') 
    {
      startJSON--;
      if (startJSON == 0)
      {
        serverResponse[i] = '}';
        serverResponse[i+1] = '\0';
        return OK;
      }
    }
    if (startJSON)
    {
      serverResponse[i] = c;
      i++;
    }
    /************************************************/
    delay(1);
  }
  return NOTOK; // if we didn't catch JSON
}

/*Function which waiting for response from GSM module*/
byte A7waitFor(String response1, String response2, int timeOut) {
  unsigned long entry = millis();
  int count = 0;
  String reply;
  byte retVal = 99;
  do {
    if (response1 == "HTTP/1.1")   // reading server request
    {
      if (A7readResponse() == OK){
        return OK;  
      }
    }
    else
    {
      reply = A7read();   // reading usual messages of GSM
    }
    if (reply != "") {
      Serial.print((millis() - entry));
      Serial.println(" ms ");
      Serial.print("Reply: ");
      Serial.println(reply);
      Serial.println("****************");
    }
  } while ((reply.indexOf(response1) + reply.indexOf(response2) == -2) && millis() - entry < timeOut );
  if ((millis() - entry) >= timeOut) {
    retVal = TIMEOUT;
  } else {
    if (reply.indexOf(response1) + reply.indexOf(response2) > -2) retVal = OK;
    else retVal = NOTOK;
  }
  return retVal;
}

/*Function which sends a command to GSM module*/
byte A7command(String command, String response1, String response2, int timeOut, int repetitions) {
  byte returnValue = NOTOK;
  byte count = 0;
  while (count < repetitions && returnValue != OK) 
  {
    GPSmodule.println(command);
    Serial.print("Command: ");
    Serial.println(command);
    
    if (A7waitFor(response1, response2, timeOut) == OK) {
      returnValue = OK;
    }
    else { 
      returnValue = NOTOK;
    }
    count++;
  }
  return returnValue;
}

/*Function for sending HTTP POST request to server*/
void sendToServer(String post_dst, String post_body) 
{
  GPSmodule.flush();
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
  GPSmodule.print("POST ");
  GPSmodule.print(post_dst);
  GPSmodule.print("?appKey=");
  GPSmodule.print(appKey);
  GPSmodule.print("&method=post&x-thingworx-session=true");
  GPSmodule.print("&temp=");
  GPSmodule.print(666);
  GPSmodule.print(" HTTP/1.1");
  GPSmodule.print("\r\n");
  GPSmodule.print("Accept: application/json");
  GPSmodule.print("\r\n");
  GPSmodule.print("HOST: ");
  GPSmodule.print(host);
  GPSmodule.print("\r\n");
  GPSmodule.print("Content-Type: application/json");
  GPSmodule.print("\r\n");
  GPSmodule.print("Conection: closed");
  GPSmodule.print("\r\n");
  GPSmodule.print("\r\n");
  GPSmodule.print(post_body);

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
  GPSmodule.println("AT+CCLK?");
  reply = A7read();
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
    String id = root["task_water"]["id"];
    int amount = root["task_water"]["amount"];
    String sys_status = root["task_water"]["status"];
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
