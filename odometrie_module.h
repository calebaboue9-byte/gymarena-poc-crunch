/* =====================================================
   odometrie_module.h — Roue de contact LPD3806
   Source principale de position câble réelle

   Rôle dans le projet :
     - Mesure la distance réelle de câble déroulé (mm)
     - Comparée à pos_max_mm pour détecter fin de course
     - Indépendante du réducteur et du tambour
     - Plus fiable que l'encodeur moteur pour POS_MAX

   Pins : PIN_ODO_A / PIN_ODO_B (config.h)
   Mode : quadrature ×2 → 1200 pulses/tour
   Résolution : ODO_MM_PAR_PULSE ≈ 0.157 mm
   ===================================================== */
#pragma once
#include "config.h"

/* --- Compteur odomètre --- */
volatile long _odo_count = 0;

/* --- Anti-rebond bouton reset --- */
static bool          _last_btn_state = HIGH;
static bool          _btn_state      = HIGH;
static unsigned long _last_debounce  = 0;

/* ─── ISR quadrature ×2 — logique standard ─── */
void IRAM_ATTR _isr_odo_A() {
  // A == B → sens positif (câble monte)
  // A != B → sens négatif (câble descend)
  if (digitalRead(PIN_ODO_A) == digitalRead(PIN_ODO_B)) _odo_count--;
  else                                                    _odo_count++;
}

void IRAM_ATTR _isr_odo_B() {
  if (digitalRead(PIN_ODO_A) == digitalRead(PIN_ODO_B)) _odo_count++;
  else                                                    _odo_count--;
}

/* ─── API publique ─── */

void odometrie_init() {
  pinMode(PIN_ODO_A, INPUT_PULLUP);
  pinMode(PIN_ODO_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ODO_A), _isr_odo_A, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ODO_B), _isr_odo_B, CHANGE);

  pinMode(PIN_BTN_RESET, INPUT_PULLUP);

  Serial.println("[ODO] Init OK");
  Serial.printf("[ODO] Roue Ø%.0fmm | %dPPR | %.4fmm/pulse\n",
                ODO_DIAMETRE_MM, ODO_PPR, ODO_MM_PAR_PULSE);
}

/* Lecture thread-safe de la position en mm */
float odometrie_position_mm() {
  noInterrupts();
  long count = _odo_count;
  interrupts();
  return count * ODO_MM_PAR_PULSE;
}

/* Lecture brute du compteur */
long odometrie_count() {
  noInterrupts();
  long count = _odo_count;
  interrupts();
  return count;
}

/* Reset manuel (bouton physique ou commande série ENC_RESET) */
void odometrie_reset() {
  noInterrupts();
  _odo_count = 0;
  interrupts();
  Serial.println("[ODO] Position remise a zero");
}

/* Gestion bouton reset physique — appeler dans loop()
   Retourne true si reset vient d'être déclenché */
bool odometrie_check_bouton() {
  bool reading = digitalRead(PIN_BTN_RESET);

  if (reading != _last_btn_state)
    _last_debounce = millis();

  if ((millis() - _last_debounce) > DEBOUNCE_MS) {
    if (reading != _btn_state) {
      _btn_state = reading;
      if (_btn_state == LOW) {   // front descendant = appui
        odometrie_reset();
        return true;
      }
    }
  }
  _last_btn_state = reading;
  return false;
}
