import numpy as np
from tflite_runtime.interpreter import Interpreter
import json
import sys

json_data = sys.argv[1]
data = json.loads(json_data)

temp = float(data['temp'])
humi = float(data['humi'])
soil = float(data['soil'])
lux = float(data['lux'])

# Tạo mảng từ các giá trị trích xuất
array_data = [temp, humi, lux, soil]

# Khởi tạo Interpreter
interpreter = Interpreter(model_path='clm11.tflite')

try:
    # Chuẩn bị mô hình
    interpreter.allocate_tensors()

    # Lấy thông tin về input và output của mô hình
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    new_sensor_data = np.array([array_data], dtype=np.float32)

    # Đưa dữ liệu vào mô hình
    interpreter.set_tensor(input_details[0]['index'], new_sensor_data)

    # Chạy mô hình
    interpreter.invoke()

    # Lấy kết quả dự đoán
    predictions = interpreter.get_tensor(output_details[0]['index'])

    light_status = 'on' if predictions[0][0] > 0.5 else 'off'
    fan_status = 'on' if predictions[0][1] > 0.5 else 'off'

    mix_output = {
        'light': {
            'name': 'light',
            'status': light_status
        },
        'fan': {
            'name': 'fan',
            'status': fan_status
        }
    }

    print(json.dumps(mix_output))

finally:
    # Giải phóng Interpreter (nếu cần)
    del interpreter
