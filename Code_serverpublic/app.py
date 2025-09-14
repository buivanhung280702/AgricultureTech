from flask import Flask, jsonify
from flask_pymongo import PyMongo
from flask_cors import CORS


app = Flask(__name__)
CORS(app)
app.config["MONGO_URI"] = "mongodb+srv://************@cluster0.s0le1.mongodb.net/greentech?retryWrites=true&w>

mongo = PyMongo(app)

sensor_data_collection = mongo.db.sensor_data

@app.route('/sensor_data', methods=['GET'])
def get_data():
    sensor_datas = sensor_data_collection.find()
    result = []
    for sensor_data in sensor_datas:
        result.append({
            'name': sensor_data.get('name', 'unknowm'),
            'temp': sensor_data.get('temp', 'unknowm'),
            'humi': sensor_data.get('humi', 'unknowm'),
            'lux': sensor_data.get('lux', 'unknowm'),
            'soil': sensor_data.get('soil', 'unknown')
        })
    return jsonify(result), 200
if __name__ == '__main__':
    app.run(debug=True)
