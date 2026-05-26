/* =====================================================
   POC GYM ARENA — Tcholak Industries / UTT Crunch
   Version unifiée — Un seul ESP32

   Sources de mesure synchronisées :
     odometrie_module  → position câble réelle (mm)
                         source principale pour POS_MAX
     encodeur_module   → sens déplacement (CON / EXC)
     hx711_module      → force câble (kg)

   Workflow kiné début de séance :
     1. TARE        → calibrer cellule à vide
     2. ENC_RESET   → position et sens à 0
     3. AMP=80      → amplitude patient en cm
     4. CT=3.0      → consigne traction (kg)
     5. CF=1.5      → consigne frein (kg)
     → Séance lancée

   Fins de mouvement :
     FIN-CON : ODO >= AMP  OU  relâchement en montée
     FIN-EXC : ODO <= 0    OU  relâchement en descente
   ===================================================== */

#include "config.h"
#include "hx711_module.h"
#include "encodeur_module.h"
#include "odometrie_module.h"
#include "driver_module.h"
#include "pid_module.h"
#include "securite_module.h"
#include "serial_module.h"

float consigne_traction_kg = 3.0f;
float consigne_frein_kg    = 1.5f;

unsigned long t_prec = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("=== POC GYM ARENA — Tcholak Industries ===");
  Serial.println("Workflow debut seance :");
  Serial.println("  TARE → ENC_RESET → AMP=80 → CT=3.0 → CF=1.5");
  Serial.println("Affichage : [ETAT][PHASE] CT CF M Err CMD | ODO/AMP");
  Serial.println("---------------------------------------------------");

  hx711_init();
  encodeur_init();    // encodeur moteur — sens uniquement
  odometrie_init();   // roue de contact — position câble réelle
  driver_init();
  pid_init();
  securite_init();

  t_prec = millis();
  Serial.println("[OK] Systeme pret.");
}

void loop() {
  unsigned long now = millis();

  /* ── Bouton reset physique odomètre ── */
  if (odometrie_check_bouton()) {
    encodeur_reset();   // sync : reset aussi l'encodeur moteur
  }

  /* ── Boucle PID toutes les PERIODE_MS ── */
  if (now - t_prec >= PERIODE_MS) {
    float dt = (now - t_prec) / 1000.0f;
    t_prec = now;

    /* 1. Lecture capteurs */
    float mesure   = hx711_lire_kg();
    bool  retour   = encodeur_phase_retour(); // sens via encodeur moteur

    /* 2. Détection état
          Source position : odomètre (roue câble) → fiable
          Source sens     : encodeur moteur → CON/EXC            */
    Etat etat = securite_detecter(mesure, now, retour);

    /* 3. Calcul commande */
    float cmd_kg = 0.0f;

    if (etat == ETAT_ACTIF) {
      float consigne = retour ? consigne_frein_kg
                              : consigne_traction_kg;
      cmd_kg = pid_calculer(consigne, mesure, dt, retour);
    } else {
      pid_reset();
      cmd_kg = 0.0f;
    }

    /* 4. Envoi driver */
    driver_set_couple(cmd_kg);

    /* 5. Affichage — position odomètre visible en temps réel */
    serial_afficher(etat, retour, mesure,
                    consigne_traction_kg,
                    consigne_frein_kg,
                    cmd_kg);
  }

  /* ── Commandes série ── */
  serial_lire_commandes(&consigne_traction_kg,
                         &consigne_frein_kg);
}
