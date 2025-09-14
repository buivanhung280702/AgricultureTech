const http = require('http');
const fs = require('fs');
const WebSocket = require('ws');
const mongoose = require('mongoose'); // Import Mongoose

let clients = [];

// Kết nối đến MongoDB
mongoose.connect('mongodb+srv://bhung7001:Hung28072002@cluster0.s0le1.mongodb.net/greentech?retryWrites=true&w=majority>
    .then(() => {
        console.log('Đã kết nối đến MongoDB');
    })
    .catch(err => {
        console.error('Lỗi kết nối đến MongoDB:', err);
    });

// Tạo schema và model cho collection 'sensor_data'
// Tạo schema và model cho collection 'sensor_data'
const Sensor = mongoose.model('sensor_data', new mongoose.Schema({
    name: String,
    //status: String,
    temp: String,
    humi: String,
    soil: String,
    lux: String
}), 'sensor_data'); // Thêm tên collection vào đây
// Tạo HTTP server để phục vụ file HTML
const server = http.createServer((req, res) => {
    fs.readFile('index.html', (err, data) => {
        if (err) {
            res.writeHead(404);
            res.end('File not found');
        } else {
            res.writeHead(200, { 'Content-Type': 'text/html' });
            res.end(data);
        }
    });
});

// Tạo WebSocket server chạy cùng HTTP server
const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
    console.log('Client đã kết nối');
    clients.push(ws);

    ws.on('message', async (message) => { // Đánh dấu hàm này là async
        console.log('Nhận được từ client:', message.toString());

        // Phân tích chuỗi JSON và trích xuất các trường
        const { name, status, temp, humi, soil, lux } = JSON.parse(message.toString());

        try {
            // Cập nhật trạng thái vào MongoDB
            await Sensor.updateOne(
                { name: name },
                { temp: temp, humi: humi, soil: soil, lux: lux },
                { upsert: true }
            );
            console.log(`Cập nhật trạng thái thiết bị ${name} thành ${status}`);
        } catch (err) {
            console.error('Lỗi cập nhật trạng thái:', err);
        }
    });

    ws.on('close', () => {
        console.log('Client đã ngắt kết nối');
        const index = clients.indexOf(ws);
        if (index !== -1) {
            clients.splice(index, 1);
        }
    });
});


// Lắng nghe trên cổng 8080
server.listen(8080, () => {
    console.log('Server đang chạy tại http://<IP-nội-bộ>:8080');
});
