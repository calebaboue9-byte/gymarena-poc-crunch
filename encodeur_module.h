/* =====================================================
   encodeur_module.h — Encodeur moteur ABI
   Rôle UNIQUE : détection du sens de déplacement

   NB : la position câble réelle vient de odometrie_module.h
        (roue de contact LPD3806 — plus fiable pour POS_MAX)
        Cet encodeur ne sert QU'au sens CON / EXC.

   Pins : PIN_ENC_MOT_A / PIN_ENC_MOT_B (config.h)
   ===================================================== */
#pragma once
#include "config.h"

/* --- Compteur encodeur moteur --- */
volatile int32_t _enc_mot_count = 0;
static   int32_t _enc_mot_prec  = 0;

/* --- Fenêtre glissante anti-bruit --- */
static int32_t _enc_hist[ENC_MOT_N_MOYS] = {0};
static int      _enc_idx = 0;

/* ─── ISR encodeur moteur ─── */
void IRAM_ATTR _isr_enc_mot() {
  if (digitalRead(PIN_ENC_MOT_A) == digitalRead(PIN_ENC_MOT_B))
    _enc_mot_count--;
  else
    _enc_mot_count++;
}

/* ─── API publique ─── */

void encodeur_init() {
  pinMode(PIN_ENC_MOT_A, INPUT_PULLUP);
  pinMode(PIN_ENC_MOT_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_MOT_A),
                  _isr_enc_mot, CHANGE);
  Serial.println("[ENC] Init OK — sens uniquement");
}

/* Retourne true si câble descend (phase excentrique) */
bool encodeur_phase_retour() {
  int32_t delta = _enc_mot_count - _enc_mot_prec;
  _enc_mot_prec = _enc_mot_count;

  _enc_hist[_enc_idx] = delta;
  _enc_idx = (_enc_idx + 1) % ENC_MOT_N_MOYS;

  int32_t somme = 0;
  for (int i = 0; i < ENC_MOT_N_MOYS; i++) somme += _enc_hist[i];

  if (abs(somme) < ENC_MOT_SEUIL * ENC_MOT_N_MOYS) return false;
  return (somme < 0);
}

/* Reset (appelé par ENC_RESET série) */
void encodeur_reset() {
  _enc_mot_count = 0;
  _enc_mot_prec  = 0;
  memset(_enc_hist, 0, sizeof(_enc_hist));
  _enc_idx = 0;
}
