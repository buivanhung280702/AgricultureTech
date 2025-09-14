let ws;
function connectWebSocket() {
    ws = new WebSocket(`ws://192.168.147.216:8080`); // Địa chỉ WebSocket server của bạn

    ws.onmessage = (event) => {
        console.log(event.data);
        const data = JSON.parse(event.data);
        console.log(data);
        
        if (Array.isArray(data)) {
            data.forEach((device) => {
                updateStatus(device.name, device.status);
            });
        } else {
            updateSensors(data);
        }
    };
}

function sendtoServer(name, status) {
    const message = JSON.stringify({ name: name, status: status });
    ws.send(message);
}

function updateStatus(name, status) {
    const element = document.getElementById(`toggle-${name}`);
    if (status === 'on') {
        element.checked = true; // Bật checkbox
    } else {
        element.checked = false; // Tắt checkbox
    }
}

function toggle(){
    const devices =['light','fan','alarm','auto'];
    devices.forEach(device=>{
        const checkbox = document.getElementById(`toggle-${device}`);
        checkbox.addEventListener('change',(event)=>{
            const status = event.target.checked ?'on':'off';
            sendtoServer(device,status);
            console.log(status);
        })
    })
}
let node1,node2;
function displayChart() {
    const ctx = document.getElementById('node1').getContext('2d');
    node2 = new Chart(ctx, {
    type: 'line', 
    data: {
        labels: [],  // Bắt đầu với nhãn trống
        datasets: [{
            label: 'Nhiệt độ',
            data: [],  // Bắt đầu với mảng dữ liệu trống
            borderColor: 'rgba(255, 99, 132, 1)',
            borderWidth: 2,
            fill: false
        }, {
            label: 'Độ ẩm',
            data: [],
            borderColor: 'rgba(54, 162, 235, 1)',
            borderWidth: 2,
            fill: false
        }, {
            label: 'Độ ẩm đất',
            data: [],
            borderColor: 'rgba(75, 192, 192, 1)',
            borderWidth: 2,
            fill: false
        },
        {
            label: 'Ánh sáng',
            data: [],  // Bắt đầu với mảng dữ liệu trống
            borderColor: 'rgba(255, 206, 86, 1)',
            borderWidth: 2,
            fill: false
        }]
    },
    options: {
        scales: {
            x: {
                display: true,
                title: {
                    display: true,
                    text: 'NODE2'
                }
            },
            y: {
                beginAtZero: true,
                max: 200  // Giới hạn trục y cho độ ẩm và nhiệt độ
            }
        }
    }
});
const ctx2 = document.getElementById('node2').getContext('2d');
node1 = new Chart(ctx2, {
    type: 'line',
    data: {
        labels: [],
        datasets: [
            {
                label: 'Nhiệt độ',
                data: [],
                borderColor: 'rgba(255, 99, 132, 1)',
                borderWidth: 2,
                fill: false
            },
            {
                label: 'Độ ẩm',
                data: [],
                borderColor: 'rgba(54, 162, 235, 1)',
                borderWidth: 2,
                fill: false
            }
        ]
    },
    options: {
        scales: {
            x: {
                display: true,
                title: {
                    display: true,
                    text: 'NODE1'
                }
            },
            y: {
                beginAtZero: true,
                max: 100 // Giới hạn trục y cho độ ẩm và nhiệt độ
            }
        }
    }
});
}
function updateSensors(data) {
    let temp1, humi1, temp2, humi2, soil2, lux2;
    if (data.name !='Area_2' && data.name !='Area_1' && data.status) {
        updateStatus(data.name, data.status);
    }
    if (data.temp && data.name == 'Area_2') {
        document.getElementById("temperature").innerText = `Nhiệt độ: ${data.temp}`;
        temp2=data.temp;
    }else if(data.temp && data.name == 'Area_1'){
        temp1 = data.temp;
    }
        
    if (data.humi && data.name == 'Area_2') {
        document.getElementById("humidity").innerText = `Độ ẩm: ${data.humi}`;
        humi2 = data.humi;
    }else if(data.humi && data.name == 'Area_1'){
        humi1 = data.humi
    }
    if (data.soil) {
        document.getElementById("soilMoisture").innerText = `Độ ẩm đất: ${data.soil}`;
        soil2 = data.soil;
    }
    if (data.lux) {
        document.getElementById("lightLevel").innerText = `Ánh sáng: ${data.lux}`;
        lux2 = data.lux;
    }
    if(data.name == 'Area_1'){
        updateNode(node1,temp1,humi1,null,null);
    }
    if(data.name == 'Area_2'){
        updateNode(node2,temp2,humi2,soil2,lux2);
    }
}

function updateNode(node,temp,humi,soil,lux){
    const currentTime= new Date().toLocaleTimeString();
    if(node.data.labels.length >10 ){
        node.data.labels.shift();
        node.data.datasets[0].data.shift();  // Xóa dữ liệu nhiệt độ đầu tiên
        node.data.datasets[1].data.shift();  // Xóa dữ liệu độ ẩm đầu tiên
        node.data.datasets[2].data.shift();
        node.data.datasets[3].data.shift();
    }
    node.data.labels.push(currentTime);
    node.data.datasets[0].data.push(temp);
    node.data.datasets[1].data.push(humi);
    if(soil){
        node.data.datasets[2].data.push(soil);
    }
    if(lux){
        node.data.datasets[3].data.push(lux);
    }
    //Update
    node.update();
}

function chatCoze(){
    const script = document.createElement('script');
    script.src = "https://sf-cdn.coze.com/obj/unpkg-va/flow-platform/chat-app-sdk/0.1.0-beta.7/libs/oversea/index.js";
    script.onload = () => {
        new CozeWebSDK.WebChatClient({
            config: {
                bot_id: '7371488485146066962',
            },
            componentProps: {
                title: 'Assistant Nhóm 1',
                icon: "https://giaoduc247.vn/uploads/082021/images/PTIT.png",
            },
        });
    };
    document.head.appendChild(script);
}
// Gọi hàm kết nối WebSocket
function main(){
    displayChart();
    toggle();
    connectWebSocket();
    chatCoze();
}
main();

