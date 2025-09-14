import asyncio
import websockets
import json
import logging
from telethon import TelegramClient, events

# Cấu hình logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Cấu hình
api_id = '25616146'  # ID của bạn
api_hash = '2a9896803ebea0b5adeff92643b7ad7a'  # Hash của bạn
bot_token = '6910494038:AAEODR6-qyCEnDwlYSUjMikNDPfdTCD6G5U'  # Token của bot
WEBSOCKET_URL = 'ws://192.168.147.150:8080'  # Địa chỉ WebSocket server của bạn

# Khởi tạo client Telegram
client = TelegramClient('bot', api_id, api_hash)

# Khởi tạo vòng lặp sự kiện chính
loop = asyncio.get_event_loop()

async def websocket_connect():
    """Tạo kết nối WebSocket và giữ kết nối liên tục"""
    try:
        ws = await websockets.connect(WEBSOCKET_URL)
        logger.info(f"Connected to WebSocket server: {WEBSOCKET_URL}")
        return ws
    except Exception as e:
        logger.error(f"Failed to connect to WebSocket server: {e}")
        return None

async def send_to_websocket(ws, data):
    """Gửi dữ liệu đến WebSocket server"""
    try:
        # Thử gửi dữ liệu. Nếu kết nối bị đóng hoặc lỗi, sẽ gây ra ngoại lệ
        if ws:
            await ws.send(json.dumps(data))
            logger.info(f"Sent data to WebSocket: {data}")
        else:
            logger.warning("WebSocket connection is closed. Reconnecting...")
            ws = await websocket_connect()  # Thử kết nối lại
            if ws:
                await send_to_websocket(ws, data)

    except websockets.exceptions.ConnectionClosed:
        logger.error("WebSocket connection closed unexpectedly.")
        # Nếu kết nối bị đóng, thử kết nối lại
        ws = await websocket_connect()
        if ws:
            await send_to_websocket(ws, data)
    except Exception as e:
        logger.error(f"Error sending data to WebSocket: {e}")
        await ws.close()  # Đảm bảo đóng kết nối nếu có lỗi

async def handle_message(event, ws):
    """Xử lý tin nhắn từ Telegram và gửi đến WebSocket"""
    try:
        message_text = event.message.text
        logger.info(f"Received message: {message_text}")

        # Kiểm tra xem tin nhắn có phải là chuỗi JSON hợp lệ không
        try:
            json_data = json.loads(message_text)
            message_data = {
                'name': json_data.get('name'),
                'status': json_data.get('status')
            }

            # Gửi tin nhắn đến WebSocket nếu JSON hợp lệ
            if json_data:
                await send_to_websocket(ws, message_data)

        except json.JSONDecodeError:
            logger.warning(f"Received message is not valid JSON: {message_text}")
            #await event.respond("Error: The message is not a valid JSON.")

    except Exception as e:
        logger.error(f"Error handling message: {e}")
        await event.respond("Error processing message")

async def main():
    """Hàm chính chạy bot và giữ kết nối WebSocket liên tục"""
    global ws

    # Kết nối WebSocket đầu tiên
    ws = await websocket_connect()
    if ws:
        # Bắt đầu lắng nghe tin nhắn Telegram
        @client.on(events.NewMessage)
        async def telegram_listener(event):
            """Lắng nghe tin nhắn Telegram và xử lý"""
            await handle_message(event, ws)

        try:
            logger.info("Bot started. Waiting for messages...")
            await client.start(bot_token=bot_token)
            await client.run_until_disconnected()
        except Exception as e:
            logger.error(f"Bot stopped: {e}")
        finally:
            await ws.close()
            logger.info("WebSocket connection closed.")
    else:
        logger.error("Could not establish WebSocket connection.")

if __name__ == '__main__':
    try:
        # Sử dụng một vòng lặp sự kiện chính duy nhất
        loop.run_until_complete(main())
    except KeyboardInterrupt:
        logger.info("Bot stopped by user.")
    finally:
        loop.close()
