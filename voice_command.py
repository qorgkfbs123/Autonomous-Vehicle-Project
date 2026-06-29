const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const Datastore = require('nedb');

const app = express();
const server = http.createServer(app);
const io = new Server(server);
const port = 3000;

app.use(express.json());
app.use(express.static('public'));

// NeDB DB 파일 생성
const db = new Datastore({ filename: 'iotdata.db', autoload: true });

// 최신 데이터 캐시
let latestData = { speed: 0, temperature: 0 };

// 🚀 Arduino → 서버
app.post('/api/data', (req, res) => {
  latestData = req.body;

  // NeDB에 저장
  db.insert({ ...latestData, timestamp: new Date() }, (err, newDoc) => {
    if (err) console.error(err);
    else console.log('📡 Data saved:', newDoc);
  });

  // 실시간 전송
  io.emit('sensor-data', latestData);

  res.sendStatus(200);
});

// 🔹 최신 데이터 요청
app.get('/api/data', (req, res) => {
  res.json(latestData);
});

// 🔹 DB에서 최근 N개 데이터
app.get('/api/history/:limit', (req, res) => {
  const limit = parseInt(req.params.limit) || 50;
  db.find({}).sort({ timestamp: -1 }).limit(limit).exec((err, docs) => {
    if (err) return res.status(500).json({ error: err.message });
    res.json(docs.reverse()); // 오래된 순으로 정렬
  });
});

server.listen(port, () => {
  console.log(`Server running on port ${port}`);
});
