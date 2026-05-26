/* =====================================================
   hx711_module.h — Lecture cellule de charge
   Matériel : HX711 + cellule 50 kg
   Pins     : PIN_HX711_DT / PIN_HX711_SCK (config.h)
   ===================================================== */
#pragma once
#include "config.h"

/* --- Variables internes --- */
static long   _hx_offset  = HX711_OFFSET;
static float  _hx_facteur = HX711_FACTEUR;
static float  _force_filtre = 0.0f;
#define        HX_ALPHA   0.2f   // filtre passe-bas (0=lent, 1=brut)

/* ---------- Lecture brute HX711 (bit-bang) ---------- */
static long _hx711_lecture_brute() {
  // Attendre que DOUT passe bas (donnée prête)
  unsigned long t0 = millis();
  while (digitalRead(PIN_HX711_DT)) {
    if (millis() - t0 > 100) return _hx_offset; // timeout
  }
  long val = 0;
  for (int i = 0; i < 24; i++) {
    digitalWrite(PIN_HX711_SCK, HIGH);
    delayMicroseconds(1);
    val = (val << 1) | digitalRead(PIN_HX711_DT);
    digitalWrite(PIN_HX711_SCK, LOW);
    delayMicroseconds(1);
  }
  // Pulse 25 → gain 128, canal A
  digitalWrite(PIN_HX711_SCK, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_HX711_SCK, LOW);
  // Complétion signe (24 bits signé)
  if (val & 0x800000) val |= 0xFF000000;
  return val;
}

/* ---------- API publique ---------- */

void hx711_init() {
  pinMode(PIN_HX711_DT,  INPUT);
  pinMode(PIN_HX711_SCK, OUTPUT);
  digitalWrite(PIN_HX711_SCK, LOW);
  Serial.println("[HX711] Init OK");
}

/* Tare — appeler au démarrage sans charge */
void hx711_tare(int n_moy = 10) {
  long somme = 0;
  for (int i = 0; i < n_moy; i++) {
    somme += _hx711_lecture_brute();
    delay(10);
  }
  _hx_offset = somme / n_moy;
  Serial.print("[HX711] Tare offset = ");
  Serial.println(_hx_offset);
}

/* Calibration — poser une masse connue après tare */
void hx711_calibrer(float masse_kg_connue) {
  long brut = _hx711_lecture_brute();
  _hx_facteur = (float)(brut - _hx_offset) / masse_kg_connue;
  Serial.print("[HX711] Facteur = ");
  Serial.println(_hx_facteur);
}

/* Lecture principale avec filtre passe-bas */
float hx711_lire_kg() {
  long brut = _hx711_lecture_brute();
  float kg_brut = (float)(brut - _hx_offset) / _hx_facteur;
  kg_brut = max(0.0f, kg_brut);  // pas de force négative
  // Filtre exponentiel : lisse sans trop retarder
  _force_filtre = HX_ALPHA * kg_brut
                + (1.0f - HX_ALPHA) * _force_filtre;
  return _force_filtre;
}
