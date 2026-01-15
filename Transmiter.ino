#include <SPI.h>
#include <mcp_can.h>
#include <DHT.h>

// ---------- PINES ----------
#define CAN_CS_PIN       10
#define CAN_INT_PIN      2

#define DHTPIN           8
#define DHTTYPE          DHT11

#define ECO_BUTTON_PIN   3
#define SPORT_BUTTON_PIN 4

#define POT_PIN          A6

// L298N (Motor A -> cuerpo de aceleración)
#define L298N_ENA_PIN    5   // PWM
#define L298N_IN1_PIN    6
#define L298N_IN2_PIN    7

// ---------- CAN ----------
MCP_CAN CAN(CAN_CS_PIN);
const unsigned long CAN_ID = 0x100;

// ---------- DHT ----------
DHT dht(DHTPIN, DHTTYPE);

// ---------- MODOS ----------
enum {
  MODE_ECO = 0,
  MODE_SPORT = 1
};

uint8_t currentMode = MODE_ECO;

// Para no saturar el bus
const unsigned long SEND_INTERVAL_MS = 100;
unsigned long lastSendTime = 0;

// ---------- SETUP ----------
void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(ECO_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPORT_BUTTON_PIN, INPUT_PULLUP);

  pinMode(L298N_ENA_PIN, OUTPUT);
  pinMode(L298N_IN1_PIN, OUTPUT);
  pinMode(L298N_IN2_PIN, OUTPUT);

  digitalWrite(L298N_ENA_PIN, LOW);
  digitalWrite(L298N_IN1_PIN, LOW);
  digitalWrite(L298N_IN2_PIN, LOW);

  // Inicializar CAN a 500 kbps con cristal de 8 MHz
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN OK");
  } else {
    Serial.println("CAN ERROR");
  }
  CAN.setMode(MCP_NORMAL);

  pinMode(CAN_INT_PIN, INPUT);
}

// ---------- LOOP ----------
void loop() {
  // 1) Leer botones y definir modo
  readModeButtons();

  // 2) Leer potenciómetro (0–100%)
  float pedalPercent = readPedalPercent();

  // 3) Porcentaje "lógico" que queremos reportar (ECO máx 50, SPORT máx 100)
  float logicalPercent = applyModeLimit(pedalPercent, currentMode);

  // 4) Porcentaje que realmente usaremos para PWM del motor
  float pwmPercent = logicalPercent;

  // En ECO: 0–50% lógico -> 0–75% de PWM físico
  if (currentMode == MODE_ECO && logicalPercent > 0.0) {
    pwmPercent = logicalPercent * 1.5;  // 50 * 1.5 = 75
    if (pwmPercent > 75.0) pwmPercent = 75.0;
  }

  // 5) Mover motor del cuerpo de aceleración
  driveThrottleBody(pwmPercent);

  // 6) Leer DHT
  float temperature = dht.readTemperature(); // °C
  float humidity    = dht.readHumidity();    // %

  if (isnan(temperature) || isnan(humidity)) {
    return; // falló lectura, reintentamos luego
  }

  // 7) Enviar por CAN cada cierto tiempo
  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;
    // Mandamos el porcentaje LÓGICO (ECO máx 50, SPORT máx 100)
    sendCanFrame(temperature, humidity, logicalPercent, currentMode);
  }

  // Debug en serial (opcional)
  Serial.print("Mode: ");
  Serial.print(currentMode == MODE_ECO ? "ECO" : "SPORT");
  Serial.print("  Pedal: ");
  Serial.print(pedalPercent);
  Serial.print("%  Logical: ");
  Serial.print(logicalPercent);
  Serial.print("%  PWM: ");
  Serial.print(pwmPercent);
  Serial.println("%");
}

// ---------- FUNCIONES ----------

// Leer botones y actualizar modo (botones a GND + INPUT_PULLUP)
void readModeButtons() {
  bool ecoPressed   = (digitalRead(ECO_BUTTON_PIN)   == LOW);
  bool sportPressed = (digitalRead(SPORT_BUTTON_PIN) == LOW);

  if (ecoPressed && !sportPressed) {
    currentMode = MODE_ECO;
  } 
  else if (sportPressed && !ecoPressed) {
    currentMode = MODE_SPORT;
  }
  // Si ambos o ninguno están presionados, se mantiene currentMode
}

// Leer potenciómetro y convertir a porcentaje 0–100
float readPedalPercent() {
  int raw = analogRead(POT_PIN);     // 0–1023
  float percent = (raw * 100.0) / 1023.0;
  if (percent < 0)   percent = 0;
  if (percent > 100) percent = 100;
  return percent;
}

// Aplicar límite según modo
float applyModeLimit(float pedalPercent, uint8_t mode) {
  float result = pedalPercent;
  if (mode == MODE_ECO && result > 50.0) {
    result = 50.0; // ECO limita a 50 %
  }
  // SPORT permite hasta 100 %
  return result;
}

// Controlar L298N y el cuerpo de aceleración
void driveThrottleBody(float percent) {
  // Umbral para dejarlo cerrado
  if (percent < 1.0) {
    // Motor apagado, resorte cierra
    digitalWrite(L298N_IN1_PIN, LOW);
    digitalWrite(L298N_IN2_PIN, LOW);
    analogWrite(L298N_ENA_PIN, 0);
    return;
  }

  // Giro en un solo sentido
  digitalWrite(L298N_IN1_PIN, HIGH);
  digitalWrite(L298N_IN2_PIN, LOW);

  int pwm = map((int)percent, 0, 100, 0, 255);

  // --- MÍNIMO PWM ÚTIL (ajusta a gusto) ---
  const int MIN_PWM = 160;  // prueba 150–190
  if (pwm > 0 && pwm < MIN_PWM) {
    pwm = MIN_PWM;
  }
  if (pwm > 255) pwm = 255;

  // Si ya estamos casi al 100%, mejor ponemos FULL ON (sin PWM)
  if (pwm >= 250) {
    digitalWrite(L298N_ENA_PIN, HIGH);  // equivalente a 255
  } else {
    analogWrite(L298N_ENA_PIN, pwm);
  }
}

// Empaquetar y mandar frame CAN
void sendCanFrame(float temperature, float humidity, float throttlePercent, uint8_t mode) {
  int16_t temp10 = (int16_t)(temperature * 10.0);   // T x10
  uint16_t hum10 = (uint16_t)(humidity * 10.0);     // H x10
  uint8_t  percentByte = (uint8_t)(throttlePercent + 0.5);

  byte data[6];
  data[0] = (temp10 >> 8) & 0xFF;
  data[1] = temp10 & 0xFF;
  data[2] = (hum10 >> 8) & 0xFF;
  data[3] = hum10 & 0xFF;
  data[4] = percentByte;
  data[5] = mode;  // 0 = ECO, 1 = SPORT

  byte result = CAN.sendMsgBuf(CAN_ID, 0, 6, data);
  if (result != CAN_OK) {
    Serial.println("Error enviando CAN");
  }
}
