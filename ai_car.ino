/************************************************
 * 핀 정의
 ************************************************/
// 적외선 (IR) 센서 핀 정의
#define RIGHT_SENSOR_PIN 4
#define LEFT_SENSOR_PIN 7

// 초음파 (Sonic) 센서 핀 정의
#define TRIG_PIN 3
#define ECHO_PIN 2

// 모터 제어 핀 정의 (L9110H)
#define MOTOR1_IA 11
#define MOTOR1_IB 10
#define MOTOR2_IA 5
#define MOTOR2_IB 6

// 부저 핀 정의
#define BUZZER_PIN 12


/************************************************
 * 제어 모드 정의
 ************************************************/
enum ControlMode { AUTO, MANUAL };
ControlMode mode = AUTO;

String btCommand = "";


/************************************************
 * 전역 변수 (장애물 안정화)
 ************************************************/
int obstacleCount = 0;
const int OBSTACLE_LIMIT = 3;


/************************************************
 * 함수 선언
 ************************************************/
void motorForward();
void motorLeft();
void motorRight();
void motorStop();
void motorBack();

void lineTracingTwoSensors();
void avoidObstacle();
long measureDistance();


/************************************************
 * setup
 ************************************************/
void setup() {
  // 센서 핀 설정
  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // 모터 및 부저 핀 설정
  pinMode(MOTOR1_IA, OUTPUT);
  pinMode(MOTOR1_IB, OUTPUT);
  pinMode(MOTOR2_IA, OUTPUT);
  pinMode(MOTOR2_IB, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // 블루투스 + 디버깅 시리얼
  Serial.begin(9600);
  Serial.println("RC Car System Initialized");
  Serial.println("Waiting for Bluetooth commands...");
}


/************************************************
 * loop
 ************************************************/
void loop() {

  /********** 블루투스 명령 수신 **********/
  if (Serial.available()) {
    btCommand = Serial.readStringUntil('\n');
    btCommand.trim();

    Serial.print("BT Command: ");
    Serial.println(btCommand);

    mode = MANUAL;
  }

  /********** 수동 제어 모드 **********/
  if (mode == MANUAL) {

    if (btCommand == "GO") {
      motorForward();
    }
    else if (btCommand == "LEFT") {
      motorLeft();
    }
    else if (btCommand == "RIGHT") {
      motorRight();
    }
    else if (btCommand == "BACK") {
      motorBack();
    }
    else if (btCommand == "STOP") {
      motorStop();
    }
    else if (btCommand == "AUTO") {
      Serial.println("Switching to AUTO mode");
      mode = AUTO;
      btCommand = "";
    }

    delay(30);
    return;   // AUTO 로직 실행 차단
  }

  /********** AUTO 모드 (기존 자율주행) **********/
  long distance = measureDistance();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm, Count: ");
  Serial.println(obstacleCount);

  // 장애물 안정화 카운트
  if (distance > 0 && distance <= 10) {
    obstacleCount++;
  } else {
    obstacleCount = 0;
  }

  // 진짜 장애물
  if (obstacleCount >= OBSTACLE_LIMIT) {
    Serial.println("!! OBSTACLE DETECTED - STOPPING !!");
    avoidObstacle();
    obstacleCount = 0;
    return;
  }

  // 라인트레이싱
  lineTracingTwoSensors();
  delay(30);
}


/************************************************
 * 초음파 거리 측정
 ************************************************/
long measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  return distance;
}


/************************************************
 * 라인트레이서
 ************************************************/
void lineTracingTwoSensors() {
  bool leftSensor = digitalRead(LEFT_SENSOR_PIN);
  bool rightSensor = digitalRead(RIGHT_SENSOR_PIN);

  if (leftSensor == LOW && rightSensor == LOW) {
    motorForward();
  }
  else if (leftSensor == LOW && rightSensor == HIGH) {
    motorLeft();
  }
  else if (leftSensor == HIGH && rightSensor == LOW) {
    motorRight();
  }
  else {
    motorStop();
  }
}


/************************************************
 * 장애물 회피 (정지 + 대기)
 ************************************************/
void avoidObstacle() {
  Serial.println("Holding position, waiting for obstacle clearance...");
  motorStop();

  const int CLEAR_DISTANCE = 15;

  while (true) {
    long currentDistance = measureDistance();

    if (currentDistance > CLEAR_DISTANCE || currentDistance == 0) {
      break;
    }

    tone(BUZZER_PIN, 1000, 100);
    delay(100);
    noTone(BUZZER_PIN);
    delay(100);

    Serial.print("Still Blocked! Distance: ");
    Serial.print(currentDistance);
    Serial.println(" cm");
  }

  noTone(BUZZER_PIN);
  Serial.println("Obstacle Cleared. Resuming AUTO mode.");
}


/************************************************
 * 모터 제어 함수
 ************************************************/
void motorForward() {
  digitalWrite(MOTOR1_IA, HIGH);
  digitalWrite(MOTOR1_IB, LOW);
  digitalWrite(MOTOR2_IA, HIGH);
  digitalWrite(MOTOR2_IB, LOW);
}

void motorBack() {
  digitalWrite(MOTOR1_IA, LOW);
  digitalWrite(MOTOR1_IB, HIGH);
  digitalWrite(MOTOR2_IA, LOW);
  digitalWrite(MOTOR2_IB, HIGH);
}

void motorRight() {
  digitalWrite(MOTOR1_IA, HIGH);
  digitalWrite(MOTOR1_IB, LOW);
  digitalWrite(MOTOR2_IA, LOW);
  digitalWrite(MOTOR2_IB, LOW);
}

void motorLeft() {
  digitalWrite(MOTOR1_IA, LOW);
  digitalWrite(MOTOR1_IB, LOW);
  digitalWrite(MOTOR2_IA, HIGH);
  digitalWrite(MOTOR2_IB, LOW);
}

void motorStop() {
  digitalWrite(MOTOR1_IA, LOW);
  digitalWrite(MOTOR1_IB, LOW);
  digitalWrite(MOTOR2_IA, LOW);
  digitalWrite(MOTOR2_IB, LOW);
}
