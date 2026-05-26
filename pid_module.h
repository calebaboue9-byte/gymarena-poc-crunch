/* =====================================================
   pid_module.h — Correcteur PID force
   Anti-windup + reset au changement de phase
   ===================================================== */
#pragma once
#include "config.h"

/* --- Gains (modifiables via série) --- */
float Kp = 50.0f;
float Ki =  5.0f;
float Kd =  2.0f;

/* --- État interne --- */
static float _integrale   = 0.0f;
static float _erreur_prec = 0.0f;
static bool  _phase_prec  = false; // suivi changement phase

void pid_init() {
  _integrale   = 0.0f;
  _erreur_prec = 0.0f;
  Serial.println("[PID] Init OK");
}

void pid_reset() {
  _integrale   = 0.0f;
  _erreur_prec = 0.0f;
}

/* Retourne une consigne de force en kg à envoyer au driver */
float pid_calculer(float consigne, float mesure,
                   float dt, bool phase_retour) {
  /* Reset intégrateur au changement de phase
     (concentrique → excentrique ou inverse)
     pour éviter la contamination entre les deux PIDs */
  if (phase_retour != _phase_prec) {
    pid_reset();
    _phase_prec = phase_retour;
  }

  consigne = constrain(consigne, FORCE_MIN_KG, FORCE_MAX_KG);
  float err = consigne - mesure;

  /* Anti-windup : on n'intègre que si ça ne sature pas */
  float integ_new = _integrale + err * dt;
  float out_test  = Kp * err + Ki * integ_new;
  const float SAT = 127.0f;
  if (out_test > -SAT && out_test < SAT)
    _integrale = integ_new;

  /* Dérivée */
  float d = (err - _erreur_prec) / dt;
  _erreur_prec = err;

  float out = Kp * err + Ki * _integrale + Kd * d;

  /* Rampe de sortie : on borne en kg
     (le driver_module convertit en couple réel) */
  return constrain(out * 0.1f + consigne, 0.0f, FORCE_MAX_KG);
}
