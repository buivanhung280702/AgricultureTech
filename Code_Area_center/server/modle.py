import numpy as np
from tflite_runtime.interpreter import Interpreter



# Tải mô hình TensorFlow Lite
interpreter = Interpreter(model_path='clm11.tflite')
interpreter.allocate_tensors()

# Lấy thông tin về input và output của mô hình
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# In ra kích thước đầu vào mong đợi của mô hình
print("Expected input shape:", input_details[0]['shape'])
data_1=[35,	79,	85,	31
]

# Ví dụ về dữ liệu cảm biến mới
new_sensor_data = np.array([data_1], dtype=np.float32)

# Kiểm tra kích thước của đầu vào yêu cầu
expected_shape = input_details[0]['shape']  # Giả sử là (1, 4)

# Reshape dữ liệu nếu cần thiết
if len(new_sensor_data.shape) == 2 and new_sensor_data.shape[0] != expected_shape[0]:
    new_sensor_data = np.expand_dims(new_sensor_data[0], axis=0)  # Lấy dòng đầu tiên và reshape thành (1, 4)

# Đưa dữ liệu vào mô hình
interpreter.set_tensor(input_details[0]['index'], new_sensor_data)

# Chạy mô hình
interpreter.invoke()

# Lấy kết quả dự đoán
predictions = interpreter.get_tensor(output_details[0]['index'])
print(predictions)

# Xử lý kết quả dự đoán
device_1_status = 'On' if predictions[0][0] > 0.5 else 'Off'
device_2_status = 'On' if predictions[0][1] > 0.5 else 'Off'

print(f"Trạng thái thiết bị 1: {device_1_status}")
print(f"Trạng thái thiết bị 2: {device_2_status}")
