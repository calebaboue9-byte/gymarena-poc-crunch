/* =====================================================
   config.h — Paramètres centraux GYM ARENA
   Un seul ESP32 — tous les modules sur la même carte

   Sources de mesure :
     Roue de contact LPD3806 → position câble réelle
       → source principale pour POS_MAX (fin de course)
     Encodeur moteur ABI     → sens déplacement uniquement
       → source pour phase CON / EXC
     Cellule de charge HX711 → force réelle
       → source pour PID + sécurités force
   ===================================================== */
#pragma once

/* ─── HX711 (cellule de charge) ─── */
#define PIN_HX711_DT      32
#define PIN_HX711_SCK     33

/* ─── Encodeur moteur ABI (sens uniquement) ─── */
#define PIN_ENC_MOT_A     25
#define PIN_ENC_MOT_B     26

/* ─── Roue de contact LPD3806 (position câble) ─── */
#define PIN_ODO_A         18
#define PIN_ODO_B         19

/* ─── Bouton reset odomètre ─── */
#define PIN_BTN_RESET     23

/* ─── Arrêt d'urgence ─── */
#define PIN_ARRET_URG     27

/* ─── Driver BLDC UART2 ─── */
// #define DRIVER_VESC
// #define DRIVER_ODRIVE
#define DRIVER_STUB

#define PIN_DRIVER_TX     17
#define PIN_DRIVER_RX     16
#define DRIVER_BAUD    115200

/* ─── Mécanique moteur ─── */
#define REDUCTEUR_RATIO      10.0f
#define TICKS_MOT_PAR_TOUR   2048   // PPR encodeur moteur (AMT102-V)

/* ─── Roue de contact ─── */
#define ODO_DIAMETRE_MM      30.0f  // Ø roue LPD3806
#define ODO_PPR              600    // PPR LPD3806
#define ODO_MM_PAR_PULSE     (PI * ODO_DIAMETRE_MM / ODO_PPR) // ≈ 0.157 mm

/* ─── Boucle PID ─── */
#define PERIODE_MS           100

/* ─── Plage opérationnelle ─── */
#define FORCE_MIN_KG         2.0f
#define FORCE_MAX_KG         5.0f

/* ─── Sécurités force ─── */
#define SEUIL_FIN_MVT_KG     0.3f
#define SEUIL_SURCHARGE_KG   6.0f
#define DUREE_FIN_MVT_MS     500

/* ─── Amplitude par défaut (conservateur) ─── */
// Remplacée par AMP=xx (cm) via série
#define POS_MAX_MM_DEFAULT   500.0f // mm

/* ─── Anti-rebond bouton ─── */
#define DEBOUNCE_MS          50

/* ─── Anti-bruit encodeur moteur ─── */
#define ENC_MOT_SEUIL        50
#define ENC_MOT_N_MOYS        5

/* ─── Calibration cellule ─── */
#define HX711_FACTEUR      2280.0f
#define HX711_OFFSET          0L
