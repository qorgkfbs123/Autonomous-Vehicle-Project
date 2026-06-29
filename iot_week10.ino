#include <Wire.h>
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

/* =====================================================
   WiFi 설정
===================================================== */
char ssid[] = "asus-fx의 iPhone";
char pass[] = "12201741";

char serverAddress[] = "165.246.241.45";
int port = 3000;

WiFiClient wifi;
HttpClient client(wifi, serverAddress, port);

/* =====================================================
   DS18B20 설정
===================================================== */
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

/* =====================================================
   MPU6050 설정
===================================================== */
const int MPU_ADDR = 0x68;

float accX_offset = 0;
float accY_offset = 0;

float velocityX = 0;
float velocityY = 0;
float speed = 0;

unsigned long lastTime = 0;

/* 노이즈 & 드리프트 방지 */
const float ACC_THRESHOLD = 0.15;     // m/s²
const float SPEED_THRESHOLD = 0.05;   // m/s
int stillCount = 0;
const int STILL_COUNT_MAX = 3;
const float MAX_SPEED = 10.0;

/* =====================================================
   MPU6050 함수
===================================================== */
void initMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

int16_t readAccXraw() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2, true);
  return (Wire.read() << 8) | Wire.read();
}

int16_t readAccYraw() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3D);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2, true);
  return (Wire.read() << 8) | Wire.read();
}

void calibrateMPU() {
  long sumX = 0, sumY = 0;
  Serial.println("MPU6050 보정 중... 가만히 두세요 (2초)");

  for (int i = 0; i < 2000; i++) {
    sumX += readAccXraw();
    sumY += readAccYraw();
    delay(1);
  }

  accX_offset = sumX / 2000.0;
  accY_offset = sumY / 2000.0;

  Serial.print("X offset = ");
  Serial.println(accX_offset);
  Serial.print("Y offset = ");
  Serial.println(accY_offset);
}

/* =====================================================
   SETUP
===================================================== */
void setup() {
  Serial.begin(9600);
  delay(2000);
  Serial.println("===== SETUP START =====");

  Wire.begin();

  tempSensor.begin();
  Serial.println("DS18B20 OK");

  initMPU();
  calibrateMPU();
  lastTime = micros();

  Serial.println("WIFI START");
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    Serial.print("WiFi status: ");
    Serial.println(status);
    delay(2000);
  }

  Serial.println("WIFI CONNECTED");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("===== SETUP DONE =====");
}

/* =====================================================
   LOOP
===================================================== */
void loop() {

  /* ---------- 온도 ---------- */
  tempSensor.requestTemperatures();
  float temperature = tempSensor.getTempCByIndex(0);

  /* ---------- 가속도 ---------- */
  int16_t rawAccX = readAccXraw();
  int16_t rawAccY = readAccYraw();

  float accX = (rawAccX - accX_offset) / 16384.0 * 9.81;
  float accY = (rawAccY - accY_offset) / 16384.0 * 9.81;

  if (abs(accX) < ACC_THRESHOLD) accX = 0;
  if (abs(accY) < ACC_THRESHOLD) accY = 0;

  /* ---------- 시간 ---------- */
  unsigned long now = micros();
  float dt = (now - lastTime) / 1000000.0;
  lastTime = now;

  /* ---------- 적분 ---------- */
  velocityX += accX * dt;
  velocityY += accY * dt;

  /* ---------- 3틱 감쇠 정지 ---------- */
  if (accX == 0 && accY == 0) {
    stillCount++;

    if (stillCount == 1) {
      velocityX *= 0.66;
      velocityY *= 0.66;
    }
    else if (stillCount == 2) {
      velocityX *= 0.33;
      velocityY *= 0.33;
    }
    else if (stillCount >= STILL_COUNT_MAX) {
      velocityX = 0;
      velocityY = 0;
    }
  } else {
    stillCount = 0;
  }

  /* ---------- 속력 ---------- */
speed = sqrt(velocityX * velocityX + velocityY * velocityY);

/* ---------- 최대 속도 제한 ---------- */
if (speed > MAX_SPEED) {
  float scale = MAX_SPEED / speed;
  velocityX *= scale;
  velocityY *= scale;
  speed = MAX_SPEED;
}

/* ---------- 최소 속도 컷 ---------- */
if (speed < SPEED_THRESHOLD) {
  speed = 0;
  velocityX = 0;
  velocityY = 0;
}

  /* ---------- JSON ---------- */
  String json = "{";
  json += "\"speed\":" + String(speed, 3) + ",";
  json += "\"temperature\":" + String(temperature, 2);
  json += "}";

  /* ---------- 서버 전송 ---------- */
  if (WiFi.status() == WL_CONNECTED) {
    int statusCode = client.post("/api/data", "application/json", json);
    Serial.print("POST status: ");
    Serial.println(statusCode);
    client.stop();
  }

  /* ---------- 시리얼 출력 ---------- */
  Serial.println(json);
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" C | AccX: ");
  Serial.print(accX);
  Serial.print(" | AccY: ");
  Serial.print(accY);
  Serial.print(" | Speed: ");
  Serial.print(speed);
  Serial.println(" m/s");

  delay(100);
}
