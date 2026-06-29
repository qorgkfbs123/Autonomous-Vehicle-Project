<!DOCTYPE html>
<html lang="ko">
<head>
  <meta charset="UTF-8">
  <title>IoT Dashboard</title>

  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script src="/socket.io/socket.io.js"></script>

  <style>
    body {
      font-family: Arial;
      background: #f4f6f8;
      margin: 0;
      padding: 20px;
      text-align: center;
    }

    h1 { margin-bottom: 20px; }

    .info-container {
      display: flex;
      justify-content: center;
      gap: 30px;
      flex-wrap: wrap;
    }

    .card {
      background: white;
      border-radius: 16px;
      padding: 20px 30px;
      width: 260px;
      box-shadow: 0 6px 16px rgba(0,0,0,0.1);
    }

    .card-title { font-size: 20px; margin-bottom: 10px; color: #555; }

    .value { font-size: 56px; font-weight: bold; }

    .temp { color: #e74c3c; }
    .speed { color: #2980b9; }

    .status { margin-top: 15px; font-size: 26px; font-weight: bold; }
    .stopped { color: #c0392b; }
    .moving  { color: #27ae60; }

    .charts {
      max-width: 1100px;
      margin: 30px auto;
      display: grid;
      grid-template-columns: 1fr;
      gap: 30px;
    }

    .chart-box {
      background: white;
      border-radius: 16px;
      padding: 20px;
      box-shadow: 0 6px 16px rgba(0,0,0,0.1);
    }

    canvas {
      width: 100% !important;
      height: 350px !important;
    }
  </style>
</head>
<body>

<h1>📡 Arduino Sensor Dashboard</h1>

<div class="info-container">

  <div class="card">
    <div class="card-title">🌡 Temperature</div>
    <div class="value temp">
      <span id="temp">--</span> °C
    </div>
  </div>

  <div class="card">
    <div class="card-title">🚀 Speed</div>
    <div class="value speed">
      <span id="speed">--</span> m/s
    </div>
    <div id="status" class="status stopped">🚗🛑 정지</div>
  </div>

</div>

<div class="charts">

  <div class="chart-box">
    <h2>🚀 Speed (m/s)</h2>
    <canvas id="speedChart"></canvas>
  </div>

  <div class="chart-box">
    <h2>🌡 Temperature (°C)</h2>
    <canvas id="tempChart"></canvas>
  </div>

</div>

<script>
  const socket = io();

  const labels = [];
  const speedData = [];
  const tempData = [];

  const speedChart = new Chart(document.getElementById('speedChart'), {
    type: 'line',
    data: { labels, datasets: [{ label: 'Speed (m/s)', data: speedData, borderWidth: 3, tension: 0.3 }] },
    options: { animation: false, scales: { y: { beginAtZero: true } } }
  });

  const tempChart = new Chart(document.getElementById('tempChart'), {
    type: 'line',
    data: { labels, datasets: [{
      label: 'Temperature (°C)',
      data: tempData,
      borderColor: '#e74c3c',
      backgroundColor: 'rgba(231,76,60,0.15)',
      borderWidth: 3,
      tension: 0.3
    }]},
    options: { animation: false, scales: { y: { beginAtZero: false } } }
  });

  // 🔹 DB에서 초기 데이터 불러오기
  async function loadHistory() {
    const res = await fetch('/api/history?limit=50');
    const history = await res.json();

    history.forEach(item => {
      const time = new Date(item.timestamp).toLocaleTimeString();
      labels.push(time);
      speedData.push(item.speed);
      tempData.push(item.temperature);
    });

    speedChart.update();
    tempChart.update();

    // 최신 값 카드에도 표시
    if (history.length > 0) {
      const last = history[history.length - 1];
      updateCards(last.speed, last.temperature);
    }
  }

  function updateCards(speed, temp) {
    document.getElementById('speed').innerText = speed.toFixed(3);
    document.getElementById('temp').innerText = temp.toFixed(2);

    const status = document.getElementById('status');
    if (speed === 0) {
      status.innerText = "🚗🛑 정지";
      status.className = "status stopped";
    } else {
      status.innerText = "🚗💨 주행 중";
      status.className = "status moving";
    }
  }

  // 🔹 실시간 데이터 반영
  socket.on('sensor-data', (data) => {
    updateCards(data.speed, data.temperature);

    const time = new Date().toLocaleTimeString();
    labels.push(time);
    speedData.push(data.speed);
    tempData.push(data.temperature);

    if (labels.length > 60) {
      labels.shift(); speedData.shift(); tempData.shift();
    }

    speedChart.update();
    tempChart.update();
  });

  loadHistory(); // 페이지 로드 시 초기 DB 데이터 불러오기
</script>

</body>
</html>
