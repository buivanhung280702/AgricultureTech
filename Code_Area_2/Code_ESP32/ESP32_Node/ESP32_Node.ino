#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

// Thông tin WiFi
const char* ssid = "a";
const char* password = "123456789";

// Địa chỉ WebSocket server
const char* websocket_server = "bhung7001.site";
const uint16_t websocket_port = 443;

const char* websocket_server_local = "192.168.147.216";    // Địa chỉ IP local server
const uint16_t websocket_port_local = 8080;

// Khai báo WebSocket client
WebSocketsClient webSocket;
WebSocketsClient webSocketLocal;

#define RX_BUFFER_SIZE 50
#define TEMP_BUFFER_SIZE 15

// UART Buffer Variables
char arr_received[RX_BUFFER_SIZE];
char count_string = 0;
unsigned int Split_count = 0;
char temp_char;
char *temp[TEMP_BUFFER_SIZE];

// Data Variables
String nhiet_do = "";
String do_am = "";
String do_am_dat = "";
String anh_sang = "";

// Function Prototypes
void DocUART();
void sendDataViaWebSocket();

// Setup function
void setup() {
  Serial.begin(9600);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  // Kết nối đến server public
  webSocket.beginSSL(websocket_server, websocket_port, "/");
  webSocket.onEvent(webSocketEvent);

  // Kết nối đến server local
  webSocketLocal.begin(websocket_server_local, websocket_port_local, "/");
  webSocketLocal.onEvent(webSocketEventLocal);
  
}

// Main loop
void loop() {
  DocUART(); 
  webSocket.loop(); 
  delay(100);
  webSocketLocal.loop();  // Duy trì kết nối với server local
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 5000) {
    lastTime = millis();
    sendDataViaWebSocket();
  }
}

// WebSocket event handler
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

// WebSocket event handler cho server local
void webSocketEventLocal(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Local Disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket Local Connected");
      break;
    case WStype_TEXT:
      Serial.printf("WebSocket Local Message: %s\n", payload);
      break;
  }
}

// Read data from UART
void DocUART() {
  while (Serial.available()) {
    temp_char = Serial.read();
    if (temp_char == '!') {
      arr_received[count_string] = '\0'; 
      Split_count = 0;
      count_string = 0;

      temp[Split_count] = strtok(arr_received, " ");
      while (temp[Split_count] != NULL && Split_count < TEMP_BUFFER_SIZE - 1) {
        Split_count++;
        temp[Split_count] = strtok(NULL, " ");
      }

      if (Split_count >= 4) {
        nhiet_do = String(temp[0]);
        do_am = String(temp[1]);
        do_am_dat = String(temp[2]);
        anh_sang = String(temp[3]);

        Serial.println("Received Data:");
        Serial.println("Nhiệt độ: " + nhiet_do);
        Serial.println("Độ ẩm: " + do_am);
        Serial.println("Độ ẩm đất: " + do_am_dat);
        Serial.println("Ánh sáng: " + anh_sang);
      }
    } else {
      if (count_string < RX_BUFFER_SIZE - 1) {
        arr_received[count_string++] = temp_char;
      } else {
        Serial.println("Buffer overflow. Resetting buffer.");
        count_string = 0;
      }
    }
  }
}

// Send data via WebSocket
void sendDataViaWebSocket() {
  if (!nhiet_do.isEmpty() && !do_am.isEmpty() && !do_am_dat.isEmpty() && !anh_sang.isEmpty()) {
    String jsonData = "{\"name\": \"Area_2\", \"status\": \"on\", \"temp\": \"" + nhiet_do + "\", \"humi\": \"" + do_am + "\", \"soil\": \"" + do_am_dat + "\", \"lux\": \"" + anh_sang + "\"}";

    if (webSocket.sendTXT(jsonData)) {
      Serial.println("Sent via WebSocket: " + jsonData);
    } else {
      Serial.println("Failed to send data via WebSocket.");
    }

        // Gửi dữ liệu tới server local
    if (webSocketLocal.sendTXT(jsonData)) {
      Serial.println("Sent via WebSocket Local: " + jsonData);
    } else {
      Serial.println("Failed to send data via WebSocket Local.");
    }
  }
}
