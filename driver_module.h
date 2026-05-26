/* =====================================================
   driver_module.h — Abstraction driver BLDC
   Sélection par #define dans config.h :
     DRIVER_VESC   → commande UART VESC
     DRIVER_ODRIVE → commande UART ODrive
     DRIVER_STUB   → mode test (pas de driver réel)

   Interface unique : driver_set_couple(float kg)
   L'appelant passe une consigne en kg →
   le module convertit en unité driver
   ===================================================== */
#pragma once
#include "config.h"

/* Conversion kg → Newton */
#define KG_TO_N(kg) ((kg) * 9.81f)

/* Rayon tambour en mètres */
#define TAMBOUR_RAYON_M  (TAMBOUR_DIAM_MM / 2000.0f)

/* Couple moteur = Force_câble × Rayon_tambour / Réducteur */
#define FORCE_TO_COUPLE_NM(kg) \
  (KG_TO_N(kg) * TAMBOUR_RAYON_M / REDUCTEUR_RATIO)

/* --- UART driver --- */
static HardwareSerial _driver_serial(2); // UART2 ESP32

/* ========== VESC ========== */
#ifdef DRIVER_VESC

void driver_init() {
  _driver_serial.begin(DRIVER_BAUD, SERIAL_8N1,
                       PIN_DRIVER_RX, PIN_DRIVER_TX);
  Serial.println("[DRV] VESC init OK");
}

/* VESC UART — commande courant en mA
   Trame : 0x02 | LEN | 0x61 (SET_CURRENT) | current_mA (int32 BE) | CRC | 0x03 */
void driver_set_couple(float kg) {
  float couple_nm  = FORCE_TO_COUPLE_NM(kg);
  // Kt moteur approximatif — à remplacer par la valeur datasheet
  // Kt ≈ 60 / (2π × Kv)  avec Kv en tr/min/V
  const float KT_NM_PER_A = 0.05f; // placeholder — À CALIBRER
  float courant_a  = couple_nm / KT_NM_PER_A;
  int32_t courant_ma = (int32_t)(courant_a * 1000.0f);

  // Trame VESC SET_CURRENT
  uint8_t payload[5];
  payload[0] = 0x61; // commande SET_CURRENT
  payload[1] = (courant_ma >> 24) & 0xFF;
  payload[2] = (courant_ma >> 16) & 0xFF;
  payload[3] = (courant_ma >>  8) & 0xFF;
  payload[4] = (courant_ma      ) & 0xFF;

  uint16_t crc = 0;
  for (int i = 0; i < 5; i++) {
    crc ^= ((uint16_t)payload[i] << 8);
    for (int b = 0; b < 8; b++)
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
  }

  _driver_serial.write(0x02);
  _driver_serial.write(0x05);
  _driver_serial.write(payload, 5);
  _driver_serial.write((crc >> 8) & 0xFF);
  _driver_serial.write(crc & 0xFF);
  _driver_serial.write(0x03);
}

#endif /* DRIVER_VESC */

/* ========== ODRIVE ========== */
#ifdef DRIVER_ODRIVE

void driver_init() {
  _driver_serial.begin(DRIVER_BAUD, SERIAL_8N1,
                       PIN_DRIVER_RX, PIN_DRIVER_TX);
  // Mettre l'axe 0 en mode couple
  _driver_serial.println("w axis0.controller.config.control_mode 1");
  _driver_serial.println("w axis0.requested_state 8"); // CLOSED_LOOP
  Serial.println("[DRV] ODrive init OK");
}

void driver_set_couple(float kg) {
  float couple_nm = FORCE_TO_COUPLE_NM(kg);
  // ODrive : commande couple ASCII
  _driver_serial.print("c 0 ");
  _driver_serial.println(couple_nm, 3);
}

#endif /* DRIVER_ODRIVE */

/* ========== STUB (test sans driver) ========== */
#ifdef DRIVER_STUB

void driver_init() {
  Serial.println("[DRV] STUB init — pas de driver reel");
}

void driver_set_couple(float kg) {
  // Affiche uniquement — utile pour valider la logique
  // sans matériel connecté
  static float _last = -1;
  if (abs(kg - _last) > 0.05f) {
    char buf[40];
    snprintf(buf, sizeof(buf), "[DRV-STUB] Couple → %.2f kg\n", kg);
    Serial.print(buf);
    _last = kg;
  }
}

#endif /* DRIVER_STUB */
