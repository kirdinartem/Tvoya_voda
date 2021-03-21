/*Function for reading of input stream from GSM module*/
String A7read() {
  String reply = "";
  while(GSMmodule.available())  {
    reply = GSMmodule.readString();
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
  
  while(GSMmodule.available())  
  {
    c = char(GSMmodule.read());
    
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
    GSMmodule.println(command);
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
