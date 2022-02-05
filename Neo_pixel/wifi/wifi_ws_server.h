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
  //wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  //wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

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

void handleIncoming(WiFiClient& client) {
  if(client.available()){
      auto c = (char)client.read();
      Serial.print((char)c);
    }
}

void handleClient(WiFiClient& client) {
  static unsigned long lastMillis = 0;
  const long intervalMs = 2000;

  handleIncoming(client);
  auto now = millis();
  long dtMs = now - lastMillis;
  if(dtMs >= intervalMs) {
    lastMillis = now;
    byte b0, b1;
    const char* text = "Hello";
    int textLen = 5;
    WebSocket::getWebsocketTextHeader(5, b0, b1);
    client.write(&b0, 1);
    client.write(&b1, 1);
    client.write(text, textLen);
  }
}

void handleNewClient() {
  int i;
	for(i = 0; i < MAX_SRV_CLIENTS; i++){
    //find free/disconnected spot
		if(!serverClients[i] || !serverClients[i].connected()){
			if(serverClients[i]) serverClients[i].stop();
			serverClients[i] = server.available();
			if (!serverClients[i]) Serial.println("available broken");
			Serial.print("New client: ");
			Serial.print(i); Serial.print(' ');
			Serial.println(serverClients[i].remoteIP());

			WebSocket::acceptUpgrade(serverClients[i]);
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
      } else {
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
