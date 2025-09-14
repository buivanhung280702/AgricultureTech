#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>

#define SS 5
#define RST 14
#define DIO0 26
#define DHTPIN 4        // Chân của DHT11 được kết nối
#define DHTTYPE DHT11   // Định nghĩa loại cảm biến DHT

DHT dht(DHTPIN, DHTTYPE);  // Khởi tạo đối tượng DHT

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Khởi tạo DHT11
  dht.begin();

  // Khởi tạo LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Khởi tạo LoRa thất bại!");
    while (1);
  }
  Serial.println("Khởi tạo LoRa thành công.");
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Lỗi đọc cảm biến DHT11!");
    return;
  }

  // Hiển thị dữ liệu cảm biến trên Serial
  Serial.print("Nhiệt độ: ");
  Serial.print(temperature);
  Serial.print(" °C, Độ ẩm: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Gửi dữ liệu cảm biến qua LoRa
  LoRa.beginPacket();
  LoRa.print(temperature);
  LoRa.print(",");  // Thêm dấu phẩy làm dấu phân tách giữa hai giá trị
  LoRa.print(humidity);
  LoRa.endPacket();

  delay(2000);  // Gửi dữ liệu mỗi 2 giây
}
