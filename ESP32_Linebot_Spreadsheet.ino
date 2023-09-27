/*
ESP32 LineBot remote (using google spreadsheet, google apps script)
Author : ChungYi Fu (Kaohsiung, Taiwan)   2022/8/5 23:30
https://www.facebook.com/francefu

apps Script
https://github.com/fustyles/webduino/blob/gs/Linebot_Spreadsheet.gs


spreadsheet
https://docs.google.com/spreadsheets

linebot
https://developers.line.biz/en/

google apps script
https://script.google.com/home/
*/
 
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

char wifi_ssid[] = "teacher";
char wifi_pass[] = "87654321";
String spreadsheetID = "*****";
String spreadsheetName = "*****";
String appsScriptID = "*****";

String spreadsheetQueryData = "{\"values\":[]}";

void setup()
{
  Serial.begin(115200);
  initWiFi();
}

void loop()
{
  spreadsheetQueryData = Spreadsheet_query("select A, B limit 1 offset 0", spreadsheetID, spreadsheetName);
  String message = Spreadsheet_getcell(0, 0);
  String replyToken = Spreadsheet_getcell(0, 1);
  if (message != "") {
    Serial.println(message);
    Serial.println(replyToken);
    if (message == "on") {
      message = "Led on";
    } else if (message == "off") {
      message = "Led off";
    } else {
      message = "Command is not defined.";
    }
	
    tcp_https("POST", "script.google.com", "/macros/s/"+appsScriptID+"/exec?response="+urlencode(message)+"&token="+replyToken, 443, 3000);
  }
  delay(1000);
}

void initWiFi() {

  for (int i=0;i<2;i++) {
    WiFi.begin(wifi_ssid, wifi_pass);

    delay(1000);
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.println(wifi_ssid);

    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if ((StartTime+5000) < millis()) break;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("STAIP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("");

      break;
    }
  }
}

String Spreadsheet_query(String sql, String mySpreadsheetid, String mySpreadsheetname) {
  sql = urlencode(sql);
  mySpreadsheetname = urlencode(mySpreadsheetname);
  const char* myDomain = "docs.google.com";
  String getAll="", getBody = "", getData = "";
  //Serial.println("Connect to " + String(myDomain));
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();
  if (client_tcp.connect(myDomain, 443)) {
    //Serial.println("Connection successful");
    String url = "https://docs.google.com/spreadsheets/d/"+mySpreadsheetid+"/gviz/tq?tqx=out:json&sheet="+mySpreadsheetname+"&tq="+sql;
    client_tcp.println("GET "+url+" HTTP/1.1");
    client_tcp.println("Host: " + String(myDomain));
    client_tcp.println("Content-Type: application/json");
    client_tcp.println();
    int waitTime = 10000;
    long startTime = millis();
    boolean state = false;
    boolean start = false;

    while ((startTime + waitTime) > millis()) {
      //Serial.print(".");
      delay(100);
      while (client_tcp.available()) {
          char c = client_tcp.read();
          if (getBody.indexOf("\"rows\":[")!=-1) start = true;
          if (getData.indexOf("],")!=-1) start = false;
          if (state==true&&c!='\n'&&c!='\r') getBody += String(c);
          if (start==true&&c!='\n'&&c!='\r') getData += String(c);
          if (c == '\n')
          {
            if (getAll.length()==0) state=true;
            getAll = "";
          }
          else if (c != '\r')
            getAll += String(c);
          startTime = millis();
       }
       if (getBody.length()>0) break;
    }
    //Serial.println("");
    if (getBody.indexOf("error")!=-1||getData=="")
    	return "{\"values\":[]}";
    getData = "{\"values\":[" + getData.substring(0, getData.length()-2) + "]}";
    return getData;
  }
  else {
    Serial.println("Connected to " + String(myDomain) + " failed.");
    return "{\"values\":[]}";
  }
}

String Spreadsheet_getcell(int row, int col) {
    if (spreadsheetQueryData!="") {
    	JsonObject obj;
    	DynamicJsonDocument doc(1024);
    	deserializeJson(doc, spreadsheetQueryData);
    	obj = doc.as<JsonObject>();
  		if ((obj["values"].size()<row+1)||(obj["values"][0]["c"].size()<col+1))
  			return "";
      	return obj["values"][row]["c"][col]["v"].as<String>();
    }
    else
	return "";
}

String tcp_https(String type,String domain,String request,int port,int waittime) {
  String getAll="", getBody="";
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();
  if (client_tcp.connect(domain.c_str(), port)) {
    //Serial.println("Connected to "+domain+" successfully.");
    client_tcp.println(type + " " + request + " HTTP/1.1");
    client_tcp.println("Host: " + domain);
    client_tcp.println("Connection: close");
    client_tcp.println("Content-Length: 0");
    client_tcp.println();
    boolean state = false;
    long startTime = millis();
    while ((startTime + waittime) > millis()) {
      while (client_tcp.available()) {
        char c = client_tcp.read();
        if (c == '\n') {
          if (getAll.length()==0) state=true;
           getAll = "";
        }
        else if (c != '\r')
          getAll += String(c);
          if (state==true) getBody += String(c);
          startTime = millis();
        }
        if (getBody.length()!= 0) break;
      }
      client_tcp.stop();
      //Serial.println(getBody);
  }
  else {
    getBody="Connected to "+domain+" failed.";
    Serial.println("Connected to "+domain+" failed.");
  }
  return getBody;
}

//https://www.arduino.cc/reference/en/libraries/urlencode/
String urlencode(String str) {
  const char *msg = str.c_str();
  const char *hex = "0123456789ABCDEF";
  String encodedMsg = "";
  while (*msg != '\0') {
    if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9') || *msg == '-' || *msg == '_' || *msg == '.' || *msg == '~') {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[(unsigned char)*msg >> 4];
      encodedMsg += hex[*msg & 0xf];
    }
    msg++;
  }
  return encodedMsg;
}
