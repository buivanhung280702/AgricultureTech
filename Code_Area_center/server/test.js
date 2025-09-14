const http = require('http');
const fs = require('fs');
const WebSocket = require('ws');
const path =require('path');
const { spawn } = require('child_process');

//const mysql = require('mysql2');
let clients = [];
let autoMode= false;
// Tạo kết nối với database
// const connection = mysql.createConnection({
//   host: 'localhost',      // Địa chỉ server database
//   user: 'root',           // Tên đăng nhập MySQL
//   password: '1234', // Mật khẩu MySQL
//   database: 'data' // Tên database bạn muốn kết nối
// });

// Mở kết nối
// connection.connect((err) => {
//   if (err) {
//     console.error('Error connecting to the database:', err.stack);
//     return;
//   }
//   console.log('Connected to the database as id', connection.threadId);
// });

// Tạo HTTP server để phục vụ file HTML
const server = http.createServer((req, res) => {
  let filePath = '.' + req.url;
  if(filePath === './'){
    filePath = './new.html';  //File HTML mặc định
  }
  const extname = String(path.extname(filePath)).toLowerCase();
  console.log(extname);
  let contentType = 'text/html'; //defaut content
  switch(extname){
    case '.css':
      contentType ='text/css';
      break;
    case '.js':
      contentType = 'application/javascript';
      break;
    case '.ico':
      contentType = 'image/x-icon';
      break;
    case '.png':
      contentType = 'image/png';
      break;
    case '.jpg':
    case '.jpeg':
      contentType = 'image/jpeg';
      break;
  }
  fs.readFile(filePath,(err,data)=>{
    if(err){
      if(err.code == 'ENOENT'){
        res.writeHead(404,{'Content-Type': 'text/html'});
        res.end("Not Found",'utf-8');
      }else{
        res.writeHead(500);
        res.end(err.code);
      }
    }else{
      res.writeHead(200,{ 'Content-Type':contentType});
      res.end(data,'utf-8');
    }
  })
});

// Tạo WebSocket server chạy cùng HTTP server
const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
  console.log('Client đã kết nối');
  clients.push(ws);
//   connection.query('SELECT * FROM data.data_1', (err, results) => {
//     if (err) throw err;
//     ws.send(JSON.stringify(results)); // Gửi dữ liệu thiết bị hiện tại
  
//   });
  ws.on('message', (message) => {
    console.log('Nhận được từ client:', message.toString());
    //{'name':' light','status':'on'}
    const {name,status,temp,humi,soil,lux} = JSON.parse(message.toString());
    //nhận được message từ client sau đó cập nhật database
    // connection.query('update data.data_1 set status = ? where name =?',[status,name],(err,results) => {
    //   if(err){
    //     console.error('lỗi cập nhật trạng thái',err);
    //     return;
    //   }
    //   console.log(`Cập nhật trạng thái thiết bị ${name} thành ${status}`);
    // })
   // ws.send(message.toString());
   
     const json_str = JSON.stringify({temp,humi,lux,soil});
     if(name =="auto"){
       autoMode=!autoMode
     }else{
       if(autoMode && temp &&name =="Area_2"){ // Nêu la tu node 2 thi phan tich du lieu
         auto(json_str,clients);
       }
     }
    

    clients.forEach((client) => {
      if (client !== ws && client.readyState === WebSocket.OPEN){
      client.send(JSON.stringify({name,status,temp,humi,soil,lux}));
      }
    })
  });

  ws.on('close', () => {
    console.log('Client đã ngắt kết nối');
    const index = clients.indexOf(ws);
    if (index !== -1) {
      clients.splice(index, 1);
    }
  });
});

// Lắng nghe trên cổng 8080 (hoặc một cổng khác nếu bạn muốn)
server.listen(8080, () => {
  console.log('Server đang chạy tại http://<IP-nội-bộ>:8080');
});


function runAIModel(data, callback) {
  const python = spawn('python', ['test_ai.py', data]);

  python.stdout.on('data', (results) => {
      console.log(`stdout: ${results}`);
      if (callback) callback(null, results.toString());
  });

  python.stderr.on('data', (results) => {
      console.error(`stderr: ${results}`);
      if (callback) callback(results.toString(), null);
  });

  python.on('close', (code) => {
      console.log(`Child process exited with code ${code}`);
  });
}

function auto(data,ws){ //data nhận từ esp32
  runAIModel(data,(err,result)=>{
    console.log(result);

    if(err){
      console.log("lỗi hàm auto",err);
    }else{
      const parseRes = JSON.parse(result);
      const lightResponse = {
        name : parseRes.light.name,
        status : parseRes.light.status,
      };
      const fanResponse = {
        name: parseRes.fan.name,
        status: parseRes.fan.status,
      };
      clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN){
        client.send(JSON.stringify(lightResponse));
        client.send(JSON.stringify(fanResponse));
        }
      })

    }
  });
}
