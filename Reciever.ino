#include <SPI.h>
#include <mcp_can.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- PINES ----------
#define CAN_CS_PIN  10
#define CAN_INT_PIN 2

// ---------- OBJETOS ----------
MCP_CAN CAN(CAN_CS_PIN);           // MCP2515 en CS = 10
const unsigned long CAN_ID = 0x100; // ID que esperamos

// Dirección típica 0x27, cambia si tu LCD es diferente
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Esperando CAN");

  // CAN
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
  // ¿Hay mensaje nuevo en el MCP2515?
  if (CAN_MSGAVAIL == CAN.checkReceive()) {
    byte len = 0;
    byte buf[8];
    unsigned long canId = 0;

    // Leer ID + longitud + datos
    CAN.readMsgBuf(&canId, &len, buf);

    if (canId == CAN_ID && len >= 6) {
      // -------- Desempaquetar --------
      int16_t temp10  = (int16_t)((buf[0] << 8) | buf[1]); // T x10
      uint16_t hum10  = (uint16_t)((buf[2] << 8) | buf[3]); // H x10
      uint8_t percent = buf[4];                            // % apertura
      uint8_t mode    = buf[5];                            // 0 ECO, 1 SPORT

      float temperature = temp10 / 10.0;
      float humidity    = hum10 / 10.0;

      // -------- Mostrar en LCD --------
      lcd.clear();

      // Línea 1: modo + porcentaje
      lcd.setCursor(0, 0);
      if (mode == 0) {
        lcd.print("ECO ");
      } else {
        lcd.print("SPORT");
      }
      lcd.print(" ");
      lcd.print(percent);
      lcd.print("%");

      // Línea 2: T y H
      lcd.setCursor(0, 1);
      lcd.print("T:");
      lcd.print(temperature, 1); // 1 decimal
      lcd.print("C ");
      lcd.print("H:");
      lcd.print((int)(humidity + 0.5)); // redondear
      lcd.print("%");
    }
  }
}
