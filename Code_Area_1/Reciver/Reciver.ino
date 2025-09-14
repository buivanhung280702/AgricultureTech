#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

// Thông tin WiFi
const char* ssid = "siu";
const char* password = "99999999";

// Địa chỉ WebSocket server
const char* websocket_server = "bhung7001.site"; // Đổi địa chỉ server
const uint16_t websocket_port = 443; // Đổi cổng thành 443

// Khai báo WebSocket client
WebSocketsClient webSocket;

// Chân LoRa
#define SS 5
#define RST 14
#define DIO0 26

// Dữ liệu cảm biến
String nhiet_do = "";
String do_am = "";

// Prototype hàm
void sendDataViaWebSocket();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

void setup() {
  // Khởi tạo giao tiếp Serial
  Serial.begin(115200);

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối tới WiFi...");
  }
  Serial.println("Đã kết nối tới WiFi");

  // Khởi tạo WebSocket
  webSocket.beginSSL(websocket_server, websocket_port, "/"); // Sử dụng beginSSL cho HTTPS
  webSocket.onEvent(webSocketEvent);

  // Khởi tạo LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Khởi tạo LoRa thất bại!");
    while (1);
  }
  Serial.println("Khởi tạo LoRa thành công.");
}

void loop() {
  // Đọc dữ liệu từ LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    // Hiển thị dữ liệu nhận được từ LoRa
    Serial.print("Nhận được từ LoRa: ");
    Serial.println(incoming);

    // Giả sử dữ liệu được nhận theo định dạng: "25.0,60.0" (nhiệt độ và độ ẩm, phân tách bởi dấu phẩy)
    int commaIndex = incoming.indexOf(',');
    if (commaIndex > 0) {
      nhiet_do = incoming.substring(0, commaIndex);
      do_am = incoming.substring(commaIndex + 1);

      // Gửi dữ liệu qua WebSocket
      sendDataViaWebSocket();
    } else {
      Serial.println("Dữ liệu không hợp lệ.");
    }
  }

  // WebSocket xử lý sự kiện
  webSocket.loop();
}

// Hàm xử lý sự kiện WebSocket
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket Connected");
      break;
    case WStype_TEXT:
      Serial.printf("WebSocket Message: %s\n", payload);
      break;
  }
}

// Hàm gửi dữ liệu qua WebSocket
void sendDataViaWebSocket() {
  // Kiểm tra nếu dữ liệu hợp lệ
  if (!nhiet_do.isEmpty() && !do_am.isEmpty()) {
    String jsonData = "{\"name\": \"HungMatLon\", \"status\": \"on\", \"temp\": \"" + nhiet_do + "\", \"humi\": \"" + do_am + "\"}";

    if (webSocket.sendTXT(jsonData)) { // Kiểm tra gửi thành công
      Serial.println("Gửi qua WebSocket: " + jsonData);
    } else {
      Serial.println("Gửi dữ liệu qua WebSocket thất bại.");
    }
  } else {
    Serial.println("Không có dữ liệu để gửi.");
  }
}
