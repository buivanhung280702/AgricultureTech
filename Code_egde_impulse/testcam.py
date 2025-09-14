import cv2

# Mở camera
cap = cv2.VideoCapture(0)  # Thay đổi chỉ số nếu cần

if not cap.isOpened():
    print("Không thể mở camera.")
    exit()

while True:
    ret, frame = cap.read()
    if not ret:
        print("Không thể nhận dữ liệu từ camera.")
        break

    cv2.imshow('Camera', frame)

    # Thoát nếu nhấn phím 'q'
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Giải phóng camera và đóng cửa sổ
cap.release()
cv2.destroyAllWindows()

