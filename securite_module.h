/* =====================================================
   securite_module.h — Machine à états + sécurités

   Source de position : odometrie_module.h (roue câble)
     → pos_max_mm réglée par AMP= (cm)
     → comparée à odometrie_position_mm()

   Fins de mouvement différenciées :
     FIN_CONCENTRIQUE : odo >= pos_max_mm  OU  lâcher
     FIN_EXCENTRIQUE  : odo <= 0           OU  lâcher
   ===================================================== */
#pragma once
#include "config.h"
#include "odometrie_module.h"

/* ─── États ─── */
enum Etat {
  ETAT_ACTIF,
  ETAT_FIN_MVT,
  ETAT_SURCHARGE,
  ETAT_URGENCE
};

/* ─── Fins différenciées ─── */
enum FinMvt {
  FIN_AUCUNE,
  FIN_CONCENTRIQUE,  // bout de course montée
  FIN_EXCENTRIQUE    // retour position initiale
};

/* ─── Amplitude adaptable par le kiné ─── */
float pos_max_mm = POS_MAX_MM_DEFAULT;  // mm — modifiée par AMP=

/* ─── État interne ─── */
static unsigned long _t_fin_debut    = 0;
static bool          _urgence_active = false;
FinMvt               derniere_fin    = FIN_AUCUNE; // public pour serial

/* ─── Init ─── */
void securite_init() {
  pinMode(PIN_ARRET_URG, INPUT_PULLUP);
  Serial.println("[SEC] Init OK");
  Serial.printf("[SEC] POS_MAX = %.0f mm (defaut)\n", pos_max_mm);
}

/* ─── Mise à jour amplitude (commande AMP= en cm) ─── */
void securite_set_amplitude_cm(float amp_cm) {
  pos_max_mm = amp_cm * 10.0f;
  Serial.printf("[AMP] %.0f cm → %.0f mm\n", amp_cm, pos_max_mm);
}

/* ─── Détection état ─── */
Etat securite_detecter(float mesure, unsigned long now,
                        bool phase_retour) {

  /* 1. Urgence physique — priorité absolue */
  if (!digitalRead(PIN_ARRET_URG) || _urgence_active) {
    _urgence_active = true;
    return ETAT_URGENCE;
  }

  /* 2. Surcharge */
  if (mesure > SEUIL_SURCHARGE_KG) {
    Serial.println("[SEC] Surcharge !");
    return ETAT_SURCHARGE;
  }

  /* Lire position odomètre (source fiable — roue câble) */
  float pos_mm = odometrie_position_mm();

  /* 3. Fin CONCENTRIQUE
        → câble tiré jusqu'à l'amplitude paramétrée par le kiné */
  if (!phase_retour && pos_mm >= pos_max_mm) {
    if (derniere_fin != FIN_CONCENTRIQUE) {
      Serial.printf("[FIN-CON] %.0f mm atteints (AMP=%.0f mm)\n",
                    pos_mm, pos_max_mm);
      derniere_fin = FIN_CONCENTRIQUE;
    }
    _t_fin_debut = 0;
    return ETAT_FIN_MVT;
  }

  /* 4. Fin EXCENTRIQUE
        → câble revenu à la position initiale */
  if (phase_retour && pos_mm <= 0.0f) {
    if (derniere_fin != FIN_EXCENTRIQUE) {
      Serial.println("[FIN-EXC] Position initiale atteinte");
      derniere_fin = FIN_EXCENTRIQUE;
    }
    _t_fin_debut = 0;
    return ETAT_FIN_MVT;
  }

  /* 5. Relâchement — filet de sécurité (les deux phases)
        → patient lâche avant d'atteindre la limite physique */
  if (mesure < SEUIL_FIN_MVT_KG) {
    if (_t_fin_debut == 0) _t_fin_debut = now;
    if (now - _t_fin_debut >= DUREE_FIN_MVT_MS) {
      FinMvt fin = phase_retour ? FIN_EXCENTRIQUE : FIN_CONCENTRIQUE;
      if (derniere_fin != fin) {
        Serial.printf("[FIN-%s] Relachement detecte\n",
                      phase_retour ? "EXC" : "CON");
        derniere_fin = fin;
      }
      return ETAT_FIN_MVT;
    }
  } else {
    _t_fin_debut = 0;
    derniere_fin = FIN_AUCUNE;
  }

  derniere_fin = FIN_AUCUNE;
  return ETAT_ACTIF;
}

/* ─── Reset urgence ─── */
void securite_reset_urgence() {
  _urgence_active = false;
  derniere_fin    = FIN_AUCUNE;
  Serial.println("[SEC] Urgence levee");
}

/* ─── Labels pour affichage ─── */
const char* securite_label(Etat e) {
  switch (e) {
    case ETAT_ACTIF:     return "ACT";
    case ETAT_FIN_MVT:
      switch (derniere_fin) {
        case FIN_CONCENTRIQUE: return "FIN-CON";
        case FIN_EXCENTRIQUE:  return "FIN-EXC";
        default:               return "FIN";
      }
    case ETAT_SURCHARGE: return "SUR";
    case ETAT_URGENCE:   return "URG";
    default:             return "???";
  }
}
