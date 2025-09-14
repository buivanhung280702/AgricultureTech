#!/usr/bin/env python

import device_patches  # Device specific patches for Jetson Nano (needs to be before importing cv2)

import cv2
import os
import sys, getopt
import signal
import time
from edge_impulse_linux.image import ImageImpulseRunner
import RPi.GPIO as GPIO  # Thêm thư viện GPIO
import requests

# Thiết lập GPIO
#GPIO.setmode(GPIO.BCM)  # Sử dụng số chân BCM
#LED_PIN = 26  # Chân GPIO mà LED sẽ được kết nối
#GPIO.setup(LED_PIN, GPIO.OUT)  # Đặt chân LED làm đầu ra

#Thong tin cua tele 
bot_token = '6910494038:AAEODR6-qyCEnDwlYSUjMikNDPfdTCD6G5U'
chat_id  = '6646617936'

fire_alert_sent = False
last_alert_time = 0
fire_clear_frames = 2
no_fire_count = 0

runner = None
show_camera = True
if (sys.platform == 'linux' and not os.environ.get('DISPLAY')):
    show_camera = False

def now():
    return round(time.time() * 1000)  # Trả về thời gian theo giây

def get_webcams():
    port_ids = []
    for port in range(5):
        print("Looking for a camera in port %s:" % port)
        camera = cv2.VideoCapture(port)
        if camera.isOpened():
            ret = camera.read()[0]
            if ret:
                backendName = camera.getBackendName()
                w = camera.get(3)
                h = camera.get(4)
                print("Camera %s (%s x %s) found in port %s " % (backendName, h, w, port))
                port_ids.append(port)
            camera.release()
    return port_ids

def sigint_handler(sig, frame):
    print('Interrupted')
    if (runner):
        runner.stop()
    GPIO.cleanup()  # Làm sạch GPIO
    sys.exit(0)

signal.signal(signal.SIGINT, sigint_handler)

def help():
    print('python classify.py <path_to_model.eim> <Camera port ID, only required when more than 1 camera is present>')

def turn_on_led():
    GPIO.output(LED_PIN, GPIO.HIGH)  # Bật LED

def turn_off_led():
    GPIO.output(LED_PIN, GPIO.LOW)  # Tắt LED



def send_telegram_photo(photo_path):
    url = f'https://api.telegram.org/bot{bot_token}/sendPhoto'
    with open(photo_path, 'rb') as photo:
        data = {'chat_id': chat_id}
        files = {'photo': photo}
        response = requests.post(url, data=data, files=files)
    if response.status_code == 200:
        print("Photo sent to Telegram successfully.")
    else:
        print("Failed to send photo:", response.text)


# Hàm phát hiện cháy và gửi ảnh
def detect_fire_and_notify(img, fire_detected):
    global fire_alert_sent, no_fire_count, last_alert_time

    if fire_detected and (not fire_alert_sent or time.time() - last_alert_time > 10):
        # Gửi ảnh khi phát hiện lửa lần đầu
        photo_path = 'fire_detected.jpg'
        cv2.imwrite(photo_path, cv2.cvtColor(img, cv2.COLOR_BGR2RGB))
        send_telegram_photo(photo_path)
        fire_alert_sent = True
        last_alert_time = time.time()  # Cập nhật thời gian gửi ảnh
        no_fire_count = 0  # Reset bộ đếm khi phát hiện lửa
    elif not fire_detected:
        no_fire_count += 1
        # Đặt lại fire_alert_sent nếu không phát hiện lửa trong `fire_clear_frames`
        if no_fire_count >= fire_clear_frames:
            fire_alert_sent = False





def main(argv):
    global runner, LED_PIN  # Để sử dụng runner và LED_PIN trong hàm này
    try:
        # Thiết lập GPIO
        GPIO.setmode(GPIO.BCM)  # Sử dụng số chân BCM
        LED_PIN = 26  # Chân GPIO mà LED sẽ được kết nối
        GPIO.setup(LED_PIN, GPIO.OUT)  # Đặt chân LED làm đầu ra

        opts, args = getopt.getopt(argv, "h", ["--help"])
    except getopt.GetoptError:
        help()
        sys.exit(2)

    for opt, arg in opts:
        if opt in ('-h', '--help'):
            help()
            sys.exit()

    if len(args) == 0:
        help()
        sys.exit(2)

    model = args[0]
    dir_path = os.path.dirname(os.path.realpath(__file__))
    modelfile = os.path.join(dir_path, model)

    print('MODEL: ' + modelfile)

    with ImageImpulseRunner(modelfile) as runner:
        try:
            model_info = runner.init()
            print('Loaded runner for "' + model_info['project']['owner'] + ' / ' + model_info['project']['name'] + '"')
            labels = model_info['model_parameters']['labels']
            if len(args) >= 2:
                videoCaptureDeviceId = int(args[1])
            else:
                port_ids = get_webcams()
                if len(port_ids) == 0:
                    raise Exception('Cannot find any webcams')
                if len(args) <= 1 and len(port_ids) > 1:
                    raise Exception("Multiple cameras found. Add the camera port ID as a second argument to use to this script")
                videoCaptureDeviceId = int(port_ids[0])

            camera = cv2.VideoCapture(videoCaptureDeviceId)
            ret = camera.read()[0]
            if ret:
                backendName = camera.getBackendName()
                w = camera.get(3)
                h = camera.get(4)
                print("Camera %s (%s x %s) in port %s selected." % (backendName, h, w, videoCaptureDeviceId))
                camera.release()
            else:
                raise Exception("Couldn't initialize selected camera.")
            next_frame = 0 
            fps_limit = 10
            frame_time = 1000 / fps_limit
            next_frame += frame_time
            next_frame = now()

            #next_frame = 0  # limit to ~10 fps here

            detected_count = 0 #thoi gian phat hien lua
            detected_threshold = 5 # can phat hien lua trong 5 khung hinh lien tiep
            # thoi gian bat led tinh bang giay
            led_on_time = 3
            led_timer = 0

            for res, img in runner.classifier(videoCaptureDeviceId):
                if (next_frame > now()):
                    time.sleep((next_frame - now()) / 1000)

                fire_detected = False # khoi tao trang thai ban dau la khong co lua
                #print('Raw response:', res)
                if "classification" in res["result"].keys():
                    print('Result (%d ms.) ' % (res['timing']['dsp'] + res['timing']['classification']), end='')
                    print('Classification results:')
                    for label in labels:
                        score = res['result']['classification'][label]
                        print('%s: %.2f\t' % (label, score), end='')
                    print('', flush=True)

                elif "bounding_boxes" in res["result"].keys():
                    #bien tam de kiem tra co lua o khung hinh nay
                    current_frame_has_fire = False
                    #print('Found %d bounding boxes (%d ms.)' % (len(res["result"]["bounding_boxes"]), res['timing']['dsp'] + res['timing']['classification']))
                    for bb in res["result"]["bounding_boxes"]:
                        #print('\t%s (%.2f): x=%d y=%d w=%d h=%d' % (bb['label'], bb['value'], bb['x'], bb['y'], bb['width'], bb['height']))
                        #img = cv2.rectangle(img, (bb['x'], bb['y']), (bb['x'] + bb['width'], bb['y'] + bb['height']), (255, 0, 0), 1)
                        #kiem tra o day test
                        if bb['label'] == 'fire'and bb['value'] > 0.8:
                            #print('WARNING : Fire detected!') 1
                            #turn_on_led() 1
                            current_frame_has_fire = True
                            fire_detected = True # phat hien lua thi o day
                            #detect_fire_and_notify(img, fire_detected)
                            #detected_count += 1 # tang thoi gian phat hien lua

                    #Cap nhat so lan phat hien lua 
                    if current_frame_has_fire:
                        detected_count += 1
                    else:
                        detected_count = 0

                    #bat led khi co lua trong khoang thoi gian du lau
                    if detected_count >= detected_threshold:
                        print('WARNING : Fire detected!')
                        #turn_on_led()
                        detect_fire_and_notify(img, fire_detected)
                        led_timer = led_on_time # khoi dong bo dem thoi gian cho led
                    else:
                        if led_timer > 0:
                            led_timer -= frame_time / 1000 # giam thoi gian da bat
                            turn_on_led()
                        else:
                            turn_off_led()

                #tat led khi khong co lua 1        
                #if not fire_detected: 1
                    #print('No Smoking') 1
                #    turn_off_led() 1

                if (show_camera):
                    cv2.imshow('edgeimpulse', cv2.cvtColor(img, cv2.COLOR_RGB2BGR))
                    if cv2.waitKey(1) == ord('q'):
                        break

                next_frame = now() + 100
        finally:
            if (runner):
                runner.stop()
            GPIO.cleanup()  # Làm sạch GPIO trước khi thoát

if __name__ == "__main__":
    main(sys.argv[1:])
