# Gym Arena — POC ESP32
Tcholak Industries · UTT Crunch 2026

## Modules
- `main.ino` — boucle principale
- `config.h` — paramètres matériels
- `hx711_module.h` — cellule de charge
- `encodeur_module.h` — sens CON/EXC
- `odometrie_module.h` — position câble LPD3806
- `pid_module.h` — correcteur PID anti-windup
- `securite_module.h` — machine à états
- `driver_module.h` — abstraction VESC/ODrive
- `serial_module.h` — interface série

## Démarrage
Décommenter le driver dans `config.h` :
// #define DRIVER_VESC
// #define DRIVER_ODRIVE
