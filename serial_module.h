/* =====================================================
   serial_module.h — Interface moniteur série

   Affichage :
     [ETAT][PHASE] CT CF Fmes Err CMD | ODO/AMP

   Commandes :
     CT=3.0      consigne traction (kg)
     CF=1.5      consigne frein (kg)
     AMP=80      amplitude patient (cm) → pos_max_mm
     Kp= Ki= Kd= gains PID
     TARE        tare cellule de charge
     CAL=2.0     calibration masse connue (kg)
     ENC_RESET   reset odomètre + encodeur moteur
     STOP        arrêt d'urgence logiciel
     RESET       lève arrêt d'urgence
   ===================================================== */
#pragma once
#include "config.h"
#include "securite_module.h"
#include "hx711_module.h"
#include "pid_module.h"
#include "encodeur_module.h"
#include "odometrie_module.h"

void serial_afficher(Etat etat, bool retour,
                     float mesure, float ct, float cf,
                     float cmd) {
  float pos_mm = odometrie_position_mm();

  char buf[120];
  snprintf(buf, sizeof(buf),
    "[%s][%s] CT=%.1f CF=%.1f M=%.2f Err=%+.2f"
    " CMD=%.2f | ODO=%.0fmm/%.0fmm\n",
    securite_label(etat),
    retour ? "EXC" : "CON",
    ct, cf,
    mesure,
    (retour ? cf : ct) - mesure,
    cmd,
    pos_mm,        // position odomètre réelle
    pos_max_mm);   // amplitude paramétrée
  Serial.print(buf);
}

void serial_lire_commandes(float* ct, float* cf) {
  if (!Serial.available()) return;

  String s = Serial.readStringUntil('\n');
  s.trim();
  s.toUpperCase();

  if (s == "STOP") {
    _urgence_active = true;
    Serial.println("[CMD] STOP — urgence activee");

  } else if (s == "RESET") {
    securite_reset_urgence();

  } else if (s == "TARE") {
    hx711_tare();

  } else if (s == "ENC_RESET") {
    /* Reset odomètre (position câble) + encodeur moteur (sens) */
    odometrie_reset();
    encodeur_reset();
    Serial.printf("[CMD] ENC_RESET — AMP maintenue %.0f mm\n",
                  pos_max_mm);

  } else if (s.startsWith("AMP=")) {
    float amp_cm = s.substring(4).toFloat();
    if (amp_cm > 5.0f && amp_cm < 300.0f)
      securite_set_amplitude_cm(amp_cm);
    else
      Serial.println("[ERR] AMP hors plage (5–300 cm)");

  } else if (s.startsWith("CAL=")) {
    float masse = s.substring(4).toFloat();
    if (masse > 0.1f) hx711_calibrer(masse);
    else Serial.println("[ERR] Masse invalide pour CAL");

  } else if (s.startsWith("CT=")) {
    *ct = constrain(s.substring(3).toFloat(),
                    FORCE_MIN_KG, FORCE_MAX_KG);

  } else if (s.startsWith("CF=")) {
    *cf = constrain(s.substring(3).toFloat(),
                    FORCE_MIN_KG, FORCE_MAX_KG);

  } else if (s.startsWith("KP=")) {
    Kp = s.substring(3).toFloat();

  } else if (s.startsWith("KI=")) {
    Ki = s.substring(3).toFloat();

  } else if (s.startsWith("KD=")) {
    Kd = s.substring(3).toFloat();

  } else if (s.length() > 0) {
    Serial.println("[ERR] Commandes : CT= CF= AMP= Kp= Ki= Kd="
                   " TARE CAL= ENC_RESET STOP RESET");
    return;
  }

  char buf[90];
  snprintf(buf, sizeof(buf),
    "[CFG] CT=%.1f CF=%.1f Kp=%.1f Ki=%.1f Kd=%.1f"
    " AMP=%.0fmm\n",
    *ct, *cf, Kp, Ki, Kd, pos_max_mm);
  Serial.print(buf);
}
