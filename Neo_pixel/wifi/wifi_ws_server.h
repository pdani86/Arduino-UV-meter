#include <WiFi.h>
#include <WiFiMulti.h>

#include "../websocket/server/websocket.h"

WiFiMulti wifiMulti;

//how many clients should be able to telnet to this ESP32
#define MAX_SRV_CLIENTS 1
const char* ssid = "UPC347917B";
const char* password = "jR88uymryCjz";

WiFiServer server(9876);
WiFiClient serverClients[MAX_SRV_CLIENTS];

void setup() {
  Serial.begin(115200);
  Serial.println("\nConnecting");

  wifiMulti.addAP(ssid, password);
  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting Wifi ");
  for (int loops = 10; loops > 0; loops--) {
    if (wifiMulti.run() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("WiFi connected ");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      break;
    }
    else {
      Serial.println(loops);
      delay(1000);
    }
  }
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi connect failed");
    delay(1000);
    ESP.restart();
  }

  //start UART and the server
  Serial2.begin(9600);
  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready!");
  Serial.print(WiFi.localIP());
  //Serial.println(" 23' to connect");
}

unsigned long lastMillis = 0;
const long intervalMs = 2000;

// TODO per client
const char* WS_KEY_STR = "WebSocket-Key: ";

void handleIncoming(WiFiClient& client) {
  if(client.available()){
      auto c = (char)client.read();
      Serial.print((char)c);
    }
}

bool waitWsKeyStart(WiFiClient& client) {
	int wsKeyStrLen = strlen(WS_KEY_STR);
	int wsKeyParsePos = 0;
	
	// auto startTime = millis();
	// const int waitTimeMs = 3000;
	
	while(client.connected()) {		
		if(!client.available()) continue;
		auto c = client.read();
		if(c == WS_KEY_STR[wsKeyParsePos]) {
			++wsKeyParsePos;
			if(wsKeyParsePos >= wsKeyStrLen) {
			  return true;
			}
		} else {
			wsKeyParsePos = 0;
		}
		
	}
	return false;
}

String readWsKey(WiFiClient& client) {
	Serial.println("readWsKey");
	int wsKeyValParsePos = 0;
	char wsKey[48];
	wsKey[0] = 0;
	while(client.connected()) {
		if(!client.available()) continue;
		auto c = client.read();
		if(c == '\r' || wsKeyValParsePos>=sizeof(wsKey)) {
			break;
		}
		Serial.print("new char for ws key: ");
		Serial.print((char)c);
		wsKey[wsKeyValParsePos++] = c;
	}
	wsKey[wsKeyValParsePos] = 0;
	return String(wsKey);
}

void wsAcceptUpgrade(WiFiClient& client) {
	Serial.println("wsAcceptUpgrade");
	
	if(!waitWsKeyStart(client)) {client.stop();}
	auto wsKey = readWsKey(client);
	auto acceptKey = genWsAcceptKey(wsKey);
	
	Serial.println("Request Key: ");
	Serial.println(wsKey);
	Serial.println("Accept Key: ");
	Serial.println(acceptKey);

	client.write(websocketUpgradeResponse);
	client.write("Sec-WebSocket-Accept: ");
	client.write(acceptKey.c_str());
	client.write("\r\n\r\n");
}

void handleClient(WiFiClient& client) {
  handleIncoming(client);
  auto now = millis();
  long dtMs = now - lastMillis;
  if(dtMs >= intervalMs) {
    lastMillis = now;
    byte b0, b1;
    const char* text = "Hello";
    int textLen = 5;
    getWebsocketTextHeader(5, b0, b1);
    client.write(&b0, 1);
    client.write(&b1, 1);
    client.write(text, textLen);
  }
}

void handleNewClient() {
  
  int i;
for(i = 0; i < MAX_SRV_CLIENTS; i++){
    //find free/disconnected spot
    if (!serverClients[i] || !serverClients[i].connected()){
      if(serverClients[i]) serverClients[i].stop();
      serverClients[i] = server.available();
      if (!serverClients[i]) Serial.println("available broken");
      Serial.print("New client: ");
      Serial.print(i); Serial.print(' ');
      Serial.println(serverClients[i].remoteIP());
	  
	  wsAcceptUpgrade(serverClients[i]);
      break;
    }
  }
  if (i >= MAX_SRV_CLIENTS) {
    //no free/disconnected spot so reject
    server.available().stop();
  }
}

void loop() {
  uint8_t i;
  if (wifiMulti.run() == WL_CONNECTED) {
    //check if there are any new clients
    if (server.hasClient()){
      handleNewClient();
    }
    //check clients for data
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
        handleClient(serverClients[i]);
      }
      else {
        if (serverClients[i]) {
          serverClients[i].stop();
        }
      }
    }
  } else {
    Serial.println("WiFi not connected!");
    for(i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i]) serverClients[i].stop();
    }
    delay(1000);
  }
}
