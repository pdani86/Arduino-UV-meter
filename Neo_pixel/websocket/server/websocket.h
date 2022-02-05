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

const char* websocketUpgradeResponse =
	"HTTP/1.1 101 Switching Protocols\r\n"
	"Upgrade: websocket\r\n"
	"Sec-WebSocket-Version: 13\r\n"
	"Connection: Upgrade\r\n"; // ezutan kell meg az accept key

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

const char* const WEBSOCKET_UUID("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
int WEBSOCKET_UUID_LEN = strlen(WEBSOCKET_UUID);
const char* const WEBSOCKET_VERSION("13");

/*
void genWsAcceptKey(const char* key, char* dst, int dstLen) {
	/*std::string accept(key);
	accept += WEBSOCKET_UUID;
	asl::security::SHA1 sha;
	sha.update(accept.c_str(), accept.size());
	asl::security::SHA1::hash_t hash = sha.final();
	std::stringstream ss;
	asl::security::Base64Encoder base(ss);
	base.write((const char*)&hash[0], hash.size());
	base.close();
	return ss.str();
}
*/

String genWsAcceptKey(const String& key) {
  String keyStr = String(key) + String(WEBSOCKET_UUID);
  char sha1[20];
  SimpleSHA1::generateSHA((uint8_t*)keyStr.c_str(), 8*keyStr.length(), (uint32_t*)sha1);
  char b64_outbuf[40];
  auto b64_len = encode_base64((byte*)sha1, sizeof(sha1), (byte*)b64_outbuf);
  b64_outbuf[b64_len] = 0;
  return String(b64_outbuf);
}

void getWebsocketTextHeader(int len, uint8_t& b0, uint8_t& b1) {
	//if(len>125) return; // not supported
	b0 = ((byte)OPCODE::OC_TEXT) << 4 | 0x01;
	b1 = len << 1;
}

