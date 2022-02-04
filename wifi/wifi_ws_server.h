#include <WiFi.h>
#include <WiFiMulti.h>

WiFiMulti wifiMulti;

const char* websocketUpgradeResponse =
	"HTTP/1.1 101 Switching Protocols\r\n"
	"Upgrade: websocket\r\n"
	"Connection: Upgrade\r\n"
	"Access-Control-Allow-Origin: *\r\n"
	"\r\n";

  enum class OPCODE {
	OC_CONTINUATION = 0x00,
	OC_TEXT = 0x01,
	OC_BINARY = 0x02,

	OC_RESERVED_X3 = 0x03,
	OC_RESERVED_X4 = 0x04,
	OC_RESERVED_X5 = 0x05,
	OC_RESERVED_X6 = 0x06,
	OC_RESERVED_X7 = 0x07,

	OC_CLOSE = 0x08,
	OC_PING = 0x09,
	OC_PONG = 0x0A,

	OC_RESERVED_XB = 0x0b,
	OC_RESERVED_XC = 0x0c,
	OC_RESERVED_XD = 0x0d,
	OC_RESERVED_XE = 0x0e,
	OC_RESERVED_XF = 0x0f
};


void getWebsocketTextHeader(int len, uint8_t& b0, uint8_t& b1) {
	//if(len>125) return; // not supported
	b0 = ((byte)OPCODE::OC_TEXT) << 4 | 0x01;
	b1 = len << 1;
}


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

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");
}

unsigned long lastMillis = 0;
const long intervalMs = 2000;


// TODO per client
const char* wsKeyStr = "WebSocket-Key: ";
int wsKeyStrLen = 15;
int wsKeyParsePos = 0;
int wsKeyValParsePos = 0;
bool wsKeyStarted = false;
bool wsKeyReady = false;
bool upgradeSent = false;
char wsKey[48];

void initWsKeyParse() {
  wsKeyParsePos = 0;
  wsKeyValParsePos = 0;
  wsKeyStarted = false;
  wsKeyReady = false;
  upgradeSent = false;
}

void handleIncoming(WiFiClient& client) {
  if(client.available()){
      auto c = (char)client.read();
      if(!wsKeyReady) {
        if(!wsKeyStarted) {
          if(c == wsKeyStr[wsKeyParsePos]) {
            ++wsKeyParsePos;
            if(wsKeyParsePos >= wsKeyStrLen) {
              wsKeyStarted = false;
              wsKeyValParsePos = 0;
            }
          } else {
            wsKeyValParsePos = 0;
            wsKey[wsKeyValParsePos++] = c;
            if(c=='\r' || wsKeyValParsePos>=47) {
              wsKey[wsKeyValParsePos] = 0;
              wsKeyReady = true;
              Serial.println(wsKey);
              client.write(websocketUpgradeResponse);
            }
          }
          
        } else {

        }
      }
      Serial.print((char)c);
    }
}

void handleClient(WiFiClient& client) {
  handleIncoming(client);
  if(!wsKeyReady) return;
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
  /*while(serverClients[i].available()) {
      auto c = serverClients[i].read();
      Serial.write(c);
      serverClients[i].write(c);
      //Serial2.write(c);
    }*/
}



void handleNewClient() {
  initWsKeyParse();
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
