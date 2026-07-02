void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(26, OUTPUT); // IN1 - Grid
  pinMode(27, OUTPUT); // IN2 - Battery
  pinMode(14, OUTPUT); // IN3 - Load 1 (10Ω)
  pinMode(13, OUTPUT); // IN4 - Load 2 (50Ω)

  // Старт — всё выключено
  digitalWrite(26, HIGH);
  digitalWrite(27, HIGH);
  digitalWrite(14, HIGH);
  digitalWrite(13, HIGH);

  delay(200);

  // Включаем Grid и обе нагрузки
  digitalWrite(26, LOW);  // Grid ON
  digitalWrite(14, LOW);  // Load 1 ON
  digitalWrite(13, LOW);  // Load 2 ON

  Serial.println("ATS BOOT OK");
  Serial.println("ts,source,vGrid,vBat,iLoad,loads");
}

// Пороги напряжения
float GRID_TRIP    = 9.5;
float GRID_RECOVER = 10.5;
float BAT_LOW      = 9.8;
float BAT_CRIT     = 8.5;

String activeSource = "GRID";
String loadState    = "FULL";
unsigned long gridLostAt = 0;

float readVoltage(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += analogRead(pin);
    delayMicroseconds(50);
  }
  float vADC = (sum / 16.0) * 3.3 / 4095.0;
  return vADC * (10.0 + 2.0) / 2.0;
}

float readCurrent(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += analogRead(pin);
    delayMicroseconds(50);
  }
  float vADC = (sum / 16.0) * 3.3 / 4095.0;
  return (vADC - 1.24) / 0.110;
}

void loop() {
  float vGrid = readVoltage(34);
  float vBat  = readVoltage(35);
  float iLoad = readCurrent(4);

  // ── Transfer Switch логика ──
  if (activeSource == "GRID") {
    if (vGrid < GRID_TRIP) {
      if (gridLostAt == 0) gridLostAt = millis();
      if (millis() - gridLostAt > 500) {
        digitalWrite(26, HIGH); // Grid OFF
        delay(50);
        digitalWrite(27, LOW);  // Battery ON
        activeSource = "BATTERY";
        gridLostAt = 0;
        Serial.println("# EVENT: TRANSFER→BATTERY");
      }
    } else {
      gridLostAt = 0;
    }
  } else if (activeSource == "BATTERY") {
    if (vGrid >= GRID_RECOVER) {
      digitalWrite(27, HIGH); // Battery OFF
      delay(50);
      digitalWrite(26, LOW);  // Grid ON
      digitalWrite(14, LOW);  // Load 1 ON
      digitalWrite(13, LOW);  // Load 2 ON
      activeSource = "GRID";
      loadState = "FULL";
      Serial.println("# EVENT: TRANSFER→GRID");
    }
  }

  // ── Load Shedding логика ──
  if (activeSource == "BATTERY") {
    if (vBat <= BAT_CRIT) {
      digitalWrite(14, HIGH); // Load 1 OFF
      digitalWrite(13, HIGH); // Load 2 OFF
      loadState = "SHED_ALL";
      Serial.println("# EVENT: BATTERY_CRITICAL");
    } else if (vBat <= BAT_LOW) {
      digitalWrite(13, HIGH); // Load 2 OFF
      loadState = "SHED_L2";
      Serial.println("# EVENT: SHED_L2");
    }
  }

  // ── Serial вывод ──
  Serial.print(millis());
  Serial.print(",");
  Serial.print(activeSource);
  Serial.print(",");
  Serial.print(vGrid, 2);
  Serial.print(",");
  Serial.print(vBat, 2);
  Serial.print(",");
  Serial.print(iLoad, 3);
  Serial.print(",");
  Serial.println(loadState);

  delay(500);
}