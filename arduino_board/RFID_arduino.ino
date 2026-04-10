#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN          10
#define RST_PIN          9
#define STM_SIGNAL_PIN   6

// STM32로 보낼 승인 신호 펄스 폭
const unsigned long SIGNAL_PULSE_MS = 100;

// 승인된 카드 UID
byte authorizedCards[][4] = {
  {0xBE, 0xAF, 0x08, 0x02}
};

const int NUM_AUTH_CARDS = sizeof(authorizedCards) / sizeof(authorizedCards[0]);

MFRC522 rfid(SS_PIN, RST_PIN);

bool isAuthorized(byte *uid, byte uidSize);
void printUID(byte *uid, byte uidSize);
void sendSTM32Pulse(void);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  SPI.begin();
  rfid.PCD_Init();

  pinMode(STM_SIGNAL_PIN, OUTPUT);
  digitalWrite(STM_SIGNAL_PIN, LOW);

  Serial.println("==================================");
  Serial.println("RFID -> STM32 Signal System Start");
  Serial.println("- Authorized RFID -> D6 HIGH Pulse");
  Serial.println("==================================");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("[RFID] Card detected. UID: ");
  printUID(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

  if (isAuthorized(rfid.uid.uidByte, rfid.uid.size)) {
    Serial.println("[RFID] Authorized card");
    sendSTM32Pulse();
  } else {
    Serial.println("[RFID] Unauthorized card");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(500);  // 같은 카드 연속 인식 완화
}

void sendSTM32Pulse(void) {
  digitalWrite(STM_SIGNAL_PIN, HIGH);
  delay(SIGNAL_PULSE_MS);
  digitalWrite(STM_SIGNAL_PIN, LOW);

  Serial.println("[STM32] D6 HIGH pulse sent");
}

bool isAuthorized(byte *uid, byte uidSize) {
  if (uidSize != 4) {
    return false;
  }

  for (int i = 0; i < NUM_AUTH_CARDS; i++) {
    bool match = true;

    for (int j = 0; j < 4; j++) {
      if (uid[j] != authorizedCards[i][j]) {
        match = false;
        break;
      }
    }

    if (match) {
      return true;
    }
  }

  return false;
}

void printUID(byte *uid, byte uidSize) {
  for (byte i = 0; i < uidSize; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
    if (i < uidSize - 1) Serial.print(" ");
  }
}