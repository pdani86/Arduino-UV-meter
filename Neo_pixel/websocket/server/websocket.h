#include <SimpleSHA1.h>
#include <base64.hpp>


  // 0                   1                   2                   3
  // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 // +-+-+-+-+-------+-+-------------+-------------------------------+
 // |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 // |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 // |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 // | |1|2|3|       |K|             |                               |
 // +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 // |     Extended payload length continued, if payload len == 127  |
 // + - - - - - - - - - - - - - - - +-------------------------------+
 // |                               |Masking-key, if MASK set to 1  |
 // +-------------------------------+-------------------------------+
 // | Masking-key (continued)       |          Payload Data         |
 // +-------------------------------- - - - - - - - - - - - - - - - +
 // :                     Payload Data continued ...                :
 // + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 // |                     Payload Data continued ...                |
 // +---------------------------------------------------------------+

/*
struct Header {
	Header();
	union {
		struct {
			uint8_t OPCODE : 4;
			uint8_t reserved : 3;
			uint8_t FIN : 1;
			uint8_t length : 7;
			uint8_t mask : 1;
		};
		uint16_t data;
	};
};
*/
namespace WebSocket {
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

const char* websocketUpgradeResponse =
	"HTTP/1.1 101 Switching Protocols\r\n"
	"Upgrade: websocket\r\n"
	"Sec-WebSocket-Version: 13\r\n"
	"Connection: Upgrade\r\n"; // ezutan kell meg az accept key

const char* const WEBSOCKET_UUID("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
int WEBSOCKET_UUID_LEN = strlen(WEBSOCKET_UUID);
const char* const WEBSOCKET_VERSION("13");

void fixShaEndianness(byte* b) {
	byte tmp[4];
	for(int i=0;i<5;i++) {
		memcpy(tmp, b, 4);
		b[0] = tmp[3];
		b[1] = tmp[2];
		b[2] = tmp[1];
		b[3] = tmp[0];
		b += 4;
	}
}

String genWsAcceptKey(const String& key) {
  String keyStr = String(key) + String(WEBSOCKET_UUID);
  byte sha1[20];
  
  SimpleSHA1::generateSHA((uint8_t*)keyStr.c_str(), 8*keyStr.length(), (uint32_t*)sha1);
  fixShaEndianness(sha1);
  
  char b64_outbuf[40];
  auto b64_len = encode_base64((byte*)sha1, sizeof(sha1), (byte*)b64_outbuf);
  b64_outbuf[b64_len] = 0;
  return String(b64_outbuf);
}

void getWebsocketTextHeader(int len, uint8_t& b0, uint8_t& b1) {
	// if(len>125) return; // not supported
	b0 = ((byte)OPCODE::OC_TEXT) | (0x01 << 7); // FIN
	b1 = len /*| (0 << 7)*/;
}

template<typename T>
bool waitWsKeyStart(T& client) {
	const char* WS_KEY_STR = "WebSocket-Key: ";
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

template<typename T>
String readWsFrame(T& client) {
	while(client.connected() && !client.available());
	if(!client.available()) return {};
	auto b0 = client.read();
	while(client.connected() && !client.available());
	if(!client.available()) return {};
	auto b1 = client.read();
	auto opcode = b0 & 0x0f;
	bool isFinal = b0 & (1 << 7);
	int len7 = b1 & 0x7f;
	bool masked = b1 & (1 << 7);
	
	if(opcode != (int)OPCODE::OC_TEXT) {
		client.stop();
		Serial.println("opcode not text");
		return {}; // not supported
	}
	
	if(len7>125) {
		client.stop();
		Serial.println("ws frame length not supported");
		return {}; // not supported
	}
	byte mask[] = {0, 0, 0, 0};
	if(masked) {
		while(client.connected() && !client.available()); if(!client.available()) return {};
		mask[0] = client.read();
		while(client.connected() && !client.available()); if(!client.available()) return {};
		mask[1] = client.read();
		while(client.connected() && !client.available()); if(!client.available()) return {};
		mask[2] = client.read();
		while(client.connected() && !client.available()); if(!client.available()) return {};
		mask[3] = client.read();
	}
	
	char str[128];
	int maskIx = 0;
	for(int i=0; i<len7; ++i) {
		while(client.connected() && !client.available()); if(!client.available()) return {};
		str[i] = client.read() ^ (mask[maskIx]);
		maskIx = (maskIx+1) % 4;
	}
	str[len7] = 0;
	return String(str);
}


template<typename T>
String readWsKey(T& client) {
	int wsKeyValParsePos = 0;
	char wsKey[48];
	wsKey[0] = 0;
	while(client.connected()) {
		if(!client.available()) continue;
		auto c = client.read();
		if(c == '\r' || wsKeyValParsePos>=sizeof(wsKey)) {
			break;
		}
		wsKey[wsKeyValParsePos++] = c;
	}
	wsKey[wsKeyValParsePos] = 0;
	return String(wsKey);
}


template<typename T>
void waitEndOfHttpHeader(T& client) {
	const char* expect = "\r\n\r\n";
	int expectIx = 0;
	while(client.connected()) {
		while(client.connected() && !client.available());
		auto c = client.read();
		if(c == expect[expectIx]) {
			++expectIx;
			if(expectIx==4) return;
		} else {
			expectIx = 0;
		}
	}
}

template<typename T>
void acceptUpgrade(T& client) {
	if(!waitWsKeyStart(client)) {client.stop();}
	auto wsKey = readWsKey(client);
	auto acceptKey = genWsAcceptKey(wsKey);
	waitEndOfHttpHeader(client);
	client.write(websocketUpgradeResponse);
	client.write("Sec-WebSocket-Accept: ");
	client.write(acceptKey.c_str());
	client.write("\r\n\r\n");
}

}
