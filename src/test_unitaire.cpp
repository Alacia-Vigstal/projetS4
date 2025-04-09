#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <vector>
#include <math.h>
#include "data.hpp"

// ======================= Booléens de contrôle global ====================
volatile bool emergencyStop = false;
volatile bool isPaused = false;
volatile bool isStarted = false;
bool gcodeFinished = false;

//std::vector<String> gcode_command = {};
int gcodeIndex = 0;

// ======================= CONFIGURATION DES LIMIT SWITCH =================
#define LIMIT_X_MIN    4
#define LIMIT_X_MAX    16
#define LIMIT_Y_MIN_L  15
#define LIMIT_Y_MIN_R  23
#define LIMIT_Y_MAX_L  25
#define LIMIT_Y_MAX_R  14
#define LIMIT_Z_MAX    13
#define LIMIT_ZRot     17

// ======================= Load Cell ======================================
#define LOAD_CELL_PIN  12

// Fonction pour lire la valeur de la cellule de charge, retour en kg [0.9122, 10.65]
float readLoadCell() {
    int potValue = analogRead(LOAD_CELL_PIN);
    Serial.print("Raw ADC: "); Serial.print(potValue);
    Serial.print(" | Converted: "); Serial.println(readLoadCell(), 3);
    return(0.9122*exp(0.0006*potValue));
}

// ======================= CONFIGURATION DES MOTEURS =======================
#define STEP_X  18
#define DIR_X   19
#define STEP_Y1 21
#define DIR_Y1  22
#define STEP_Y2 2
#define DIR_Y2  5
#define STEP_ROTATION_OUTIL  26
#define DIR_ROTATION_OUTIL   27
#define STEP_Z  32
#define DIR_Z   33

AccelStepper moteurDeplacementX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper moteurDeplacementY1(AccelStepper::DRIVER, STEP_Y1, DIR_Y1);
AccelStepper moteurDeplacementY2(AccelStepper::DRIVER, STEP_Y2, DIR_Y2);
AccelStepper moteurRotationOutil(AccelStepper::DRIVER, STEP_ROTATION_OUTIL, DIR_ROTATION_OUTIL);
AccelStepper moteurHauteurOutil(AccelStepper::DRIVER, STEP_Z, DIR_Z);
MultiStepper multiStepper;

// ======================= CONFIGURATION DES PARAMETRES =======================
#define PAS_PAR_MM_X   80
#define PAS_PAR_MM_Y   80
#define PAS_PAR_MM_Z   25
#define VITESSE_MAX    2000
#define VITESSE_COUPE  1200
#define VITESSE_HOME   1000
#define ACCELERATION   3000
#define PAS_PAR_DEGREE 10
#define ERREUR_MAX_Y   10

// ======================= MOUVEMENT DES MOTEURS =======================
void moveXYZ(float x, float y, float z, float zRot) {
    long stepsX = x * PAS_PAR_MM_X;
    long stepsY = y * PAS_PAR_MM_Y;
    long stepsZ = z * PAS_PAR_MM_Z;
    long stepsRotationOutil = zRot * PAS_PAR_DEGREE;

    // Tableau contenant les positions cibles pour les moteurs gérés par MultiStepper.
    // L'ordre des valeurs doit correspondre à l'ordre des moteurs ajoutés à multiStepper.
    long positions[5] = { stepsX, stepsY, stepsY, stepsZ, stepsRotationOutil };


    Serial.println("Commande reçue : X=" + String(x) + " Y=" + String(y) + " Z=" + String(z) + " ZRot=" + String(zRot));
    multiStepper.moveTo(positions);

    while (
        moteurDeplacementX.distanceToGo() ||
        moteurDeplacementY1.distanceToGo() ||
        moteurDeplacementY2.distanceToGo() ||
        moteurRotationOutil.distanceToGo() ||
        moteurHauteurOutil.distanceToGo()
    ) {
        if (emergencyStop) {
            Serial.println("Arrêt d'urgence déclenché ! Appuyez sur le bouton RESET.");
            moteurDeplacementX.stop();
            moteurDeplacementY1.stop();
            moteurDeplacementY2.stop();
            moteurRotationOutil.stop();
            moteurHauteurOutil.stop();
            while (true);
        }

        // Pause manuelle
        if (isPaused) {
            Serial.println("Système en pause.");
            while (isPaused) {
                delay(100);
                yield();
            }
            Serial.println("Reprise du mouvement.");
        }

        // Avancer les moteurs
        multiStepper.run();
    }
}

// ======================= HOMING FUNCTION ====================================
void homeAxes() {
    Serial.println("Homing X, Y, Z et ZRot...");

    // Move X towards MIN limit switch
    moteurDeplacementX.setSpeed(-VITESSE_HOME);  // Move in negative direction
    while (digitalRead(LIMIT_X_MIN) != HIGH) {
        if (digitalRead(LIMIT_X_MIN) == HIGH) Serial.println("X Min Switch Pressed!");
        moteurDeplacementX.runSpeed();
        delayMicroseconds(100); // Ajoute un petit délai
    }
    moteurDeplacementX.stop();  
    moteurDeplacementX.setCurrentPosition(0);  // Set home position
    Serial.println("Homing X");

    // Move Y towards MAX limit switch
    moteurDeplacementY1.setSpeed(-VITESSE_HOME);  // Move in negative direction
    moteurDeplacementY2.setSpeed(-VITESSE_HOME);
    while (digitalRead(LIMIT_Y_MAX_R) != HIGH) {
        if (digitalRead(LIMIT_Y_MAX_R) == HIGH) Serial.println("Y Max Switch Pressed!");
        moteurDeplacementY1.runSpeed();
        moteurDeplacementY2.runSpeed();
        delayMicroseconds(100); // Ajoute un petit délai
    }
    moteurDeplacementY1.stop(); 
    moteurDeplacementY2.stop();  
    moteurDeplacementY1.setCurrentPosition(0);
    moteurDeplacementY2.setCurrentPosition(0);  // Set home position
    Serial.println("Homing Y");

    // Move Z towards MIN limit switch
    moteurHauteurOutil.setSpeed(VITESSE_HOME);
    while (digitalRead(LIMIT_Z_MAX) != HIGH) {
        moteurHauteurOutil.runSpeed();
        delayMicroseconds(100); // Ajoute un petit délai
    }
    moteurHauteurOutil.stop();
    //moteurHauteurOutil.setCurrentPosition(100*PAS_PAR_MM_Z);
    Serial.println("Homing Z");

    // Move ZRot towards MIN limit switch
    moteurRotationOutil.setSpeed(-VITESSE_HOME);
    while (digitalRead(LIMIT_ZRot) != HIGH) {
        moteurRotationOutil.runSpeed();
        delayMicroseconds(100); // Ajoute un petit délai
    }
    moteurRotationOutil.stop();
    moteurRotationOutil.setCurrentPosition(0);
    Serial.println("Homing ZRot");

    Serial.println("Homing terminé !");
}

// ======================= EXÉCUTION DU G-CODE =======================
void executeGCodeCommand(const String& command) {
    // Variables pour extraire les valeurs du G-code
    float g = 0, x = 0, y = 0, z = 0, zRot = 0;
    char gcode[20];
    int readCount = sscanf(command.c_str(), "G%f X%f Y%f Z%f Zrot%f", &g, &x, &y, &z, &zRot);

    if ((int)g == 0) {  // G00 : rapide
        Serial.println("G00 → vitesse max");
        moteurDeplacementX.setMaxSpeed(VITESSE_MAX);
        moteurDeplacementY1.setMaxSpeed(VITESSE_MAX);
        moteurDeplacementY2.setMaxSpeed(VITESSE_MAX);
        moteurHauteurOutil.setMaxSpeed(VITESSE_MAX);
        moteurRotationOutil.setMaxSpeed(VITESSE_MAX);
    } 
    else if ((int)g == 1) {  // G01 : coupe
        Serial.println("G01 → vitesse de coupe");
        moteurDeplacementX.setMaxSpeed(VITESSE_COUPE);
        moteurDeplacementY1.setMaxSpeed(VITESSE_COUPE);
        moteurDeplacementY2.setMaxSpeed(VITESSE_COUPE);
        moteurHauteurOutil.setMaxSpeed(VITESSE_COUPE);
        moteurRotationOutil.setMaxSpeed(VITESSE_COUPE);
    }

    // Exécuter le mouvement en fonction du G-code
    moveXYZ(x, y, z, zRot);
    Serial.println("Fin du moveXYZ");
    delay(1000);
}

// ======================= Boutons Pi 5 ===================================
void processSerialCommand(String input) {
    input.trim();
    if (input.length() == 0) return;

    String cmd;
    float stepValue = 1.0;  // Valeur par défaut

    int spaceIndex = input.indexOf(' ');
    if (spaceIndex != -1) {
        cmd = input.substring(0, spaceIndex);
        String stepStr = input.substring(spaceIndex + 1);
        stepValue = stepStr.toFloat();
        if (stepValue == 0.0) stepValue = 1.0;  // fallback
    } else {
        cmd = input;
    }

    // === Commandes système ===
    if (cmd == "RESET") {
        emergencyStop = false;
        Serial.println("Reset d’urgence effectué (via série).");
        return;
    }

    if (cmd == "PAUSE") {
        isPaused = !isPaused;
        Serial.println(isPaused ? "Pause activée (via série)" : "Reprise (via série)");
        return;
    }

    if (cmd == "START") {
        isStarted = true;
        Serial.println("Démarrage demandé (via série)");
        return;
    }

    if (cmd == "STOP") {
        isStarted = false;
        Serial.println("Arrêt manuel de l'exécution du G-code.");
        return;
    }    

    if (cmd == "HOME") {
        Serial.println("Homing demandé (via série)");
        homeAxes();
        return;
    }    

    if (cmd == "UPLOAD") {
        gcode_command.clear();
        gcodeIndex = 0;
        Serial.println("Prêt à recevoir le G-code. Envoyez 'END' pour terminer.");
        while (true) {
            while (!Serial.available()) delay(10);
            String line = Serial.readStringUntil('\n');
            line.trim();
            if (line == "END") break;
            if (line.length() > 0) {
                gcode_command.push_back(line);
            }
        }
        Serial.println("G-code reçu !");
        return;
    }

    if (cmd == "RUN_GCODE") {
        gcodeIndex = 0;
        gcodeFinished = false;
        isStarted = true;
    
        Serial.println("=== [RUN_GCODE] ===");
        Serial.println("Homing automatique avant l'exécution du G-code...");
        homeAxes();
        
        Serial.println("Exécution du G-code redémarrée (via série).");
        Serial.println("Nombre de commandes : " + String(gcode_command.size()));
        Serial.println("Index remis à zéro.");
        Serial.println("====================");
        return;
    }    

    if (cmd == "STATUS") {
        Serial.println("--- État du système ---");
        Serial.println("emergencyStop: " + String(emergencyStop));
        Serial.println("isPaused: " + String(isPaused));
        Serial.println("isStarted: " + String(isStarted));
        Serial.println("gcodeIndex: " + String(gcodeIndex));
        Serial.println("gcodeSize: " + String(gcode_command.size()));
        Serial.println("gcodeFinished: " + String(gcodeFinished));
        
        if (gcode_command.size() > 0) {
            float percent = ((float)gcodeIndex / gcode_command.size()) * 100.0;
            Serial.println("Progression G-code: " + String(percent, 1) + "%");
        }
    
        Serial.println("Pos X: " + String(moteurDeplacementX.currentPosition()));
        Serial.println("Pos Y1: " + String(moteurDeplacementY1.currentPosition()));
        Serial.println("Pos Z: " + String(moteurHauteurOutil.currentPosition()));
        Serial.println("ZRot: " + String(moteurRotationOutil.currentPosition()));
        return;
    }    

    // === Commandes de mouvement ===
    bool moved = false;

    if (cmd == "X_MOINS") {
        moteurDeplacementX.move(-PAS_PAR_MM_X * stepValue);
        moved = true;
    } else if (cmd == "X_PLUS") {
        moteurDeplacementX.move(PAS_PAR_MM_X * stepValue);
        moved = true;
    } else if (cmd == "Y_PLUS") {
        moteurDeplacementY1.move(-PAS_PAR_MM_Y * stepValue);
        moteurDeplacementY2.move(-PAS_PAR_MM_Y * stepValue);
        moved = true;
    } else if (cmd == "Y_MOINS") {
        moteurDeplacementY1.move(PAS_PAR_MM_Y * stepValue);
        moteurDeplacementY2.move(PAS_PAR_MM_Y * stepValue);
        moved = true;
    } else if (cmd == "Z_UP") {
        moteurHauteurOutil.move(PAS_PAR_MM_Z * stepValue);
        moved = true;
    } else if (cmd == "Z_DOWN") {
        moteurHauteurOutil.move(-PAS_PAR_MM_Z * stepValue);
        moved = true;
    } else if (cmd == "ZROT_CLOCK") {
        moteurRotationOutil.move(-PAS_PAR_DEGREE * stepValue);
        moved = true;
    } else if (cmd == "ZROT_C_CLOCK") {
        moteurRotationOutil.move(PAS_PAR_DEGREE * stepValue);
        moved = true;
    } else if (cmd == "Z_ZERO") {
        moteurHauteurOutil.setCurrentPosition(0);
        Serial.println("Position Z enregistrée comme zéro.");
        return;
    } else {
        Serial.println("Commande inconnue : " + cmd);
        return;
    }

    if (moved) {
        while (
            moteurDeplacementX.distanceToGo() ||
            moteurDeplacementY1.distanceToGo() ||
            moteurDeplacementY2.distanceToGo() ||
            moteurHauteurOutil.distanceToGo() ||
            moteurRotationOutil.distanceToGo()
        ) {
            moteurDeplacementX.run();
            moteurDeplacementY1.run();
            moteurDeplacementY2.run();
            moteurHauteurOutil.run();
            moteurRotationOutil.run();
            delay(1);
        }
        Serial.println("Déplacement effectué : " + cmd + " de " + String(stepValue));
    }
}

// ======================= printLimitSwitchStates ====================
void printLimitSwitchStates() {
    if (digitalRead(LIMIT_X_MIN) == HIGH) Serial.println("X Min Switch Pressed!");
    if (digitalRead(LIMIT_X_MAX) == HIGH) Serial.println("X Max Switch Pressed!");
    if (digitalRead(LIMIT_Y_MIN_L) == HIGH) Serial.println("Y Min L Switch Pressed!");
    if (digitalRead(LIMIT_Y_MIN_R) == HIGH) Serial.println("Y Min R Switch Pressed!");
    if (digitalRead(LIMIT_Y_MAX_L) == HIGH) Serial.println("Y Max L Switch Pressed!");
    if (digitalRead(LIMIT_Y_MAX_R) == HIGH) Serial.println("Y Max R Switch Pressed!");
    if (digitalRead(LIMIT_Z_MAX) == HIGH) Serial.println("Z Max Switch Pressed!");
    if (digitalRead(LIMIT_ZRot) == HIGH) Serial.println("ZRot Switch Pressed!");
}

void checkLimitSwitches() {
    // Arrêts d'urgence
    printLimitSwitchStates();

    if (digitalRead(LIMIT_X_MIN) == HIGH) {
        emergencyStop = true;
        Serial.println("Limiteur X_MIN activé");
    }
    else if (digitalRead(LIMIT_X_MAX) == HIGH) {
        emergencyStop = true;
        Serial.println("Limiteur X_MAX activé");
    }
    else if (digitalRead(LIMIT_Y_MIN_L) == HIGH) {
        emergencyStop = true;
        Serial.println("Limiteur Y_MIN_L activé");
    }
    else if (digitalRead(LIMIT_Y_MIN_R) == HIGH) {
        emergencyStop = true;
        Serial.println("Limiteur Y_MIN_R activé");
    }
    else if (digitalRead(LIMIT_Y_MAX_L) == HIGH) {
        emergencyStop = true;
        Serial.println("Limiteur Y_MAX_L activé");
    }
    else if (digitalRead(LIMIT_Y_MAX_R) == HIGH) {
        emergencyStop = true;
        Serial.println("Limiteur Y_MAX_R activé");
    }
    else if (digitalRead(LIMIT_Z_MAX) == HIGH) {
        emergencyStop = true;
        Serial.println("Limiteur Z_MAX activé");
    }
}

// ======================= Setup =======================
void setup() {
    Serial.begin(115200);
    Serial.flush();
    delay(1000);  // Laisse le temps au port série de s'initialiser
    Serial.println("Début du setup");

    // Set limit switches as inputs with pull-up resistors
    Serial.println("Pins des limiteurs configurés");
    pinMode(LIMIT_X_MIN, INPUT_PULLUP);
    pinMode(LIMIT_X_MAX, INPUT_PULLUP);
    pinMode(LIMIT_Y_MIN_L, INPUT_PULLUP);
    pinMode(LIMIT_Y_MIN_R, INPUT_PULLUP);
    pinMode(LIMIT_Y_MAX_L, INPUT_PULLUP);
    pinMode(LIMIT_Y_MAX_R, INPUT_PULLUP);
    pinMode(LIMIT_Z_MAX, INPUT_PULLUP);
    pinMode(LIMIT_ZRot, INPUT_PULLUP);

    moteurDeplacementY2.setPinsInverted(true, false, false);
    Serial.println("Vitesse des moteurs réglée");
    moteurDeplacementX.setMaxSpeed(VITESSE_MAX);
    moteurDeplacementX.setAcceleration(ACCELERATION);
    moteurDeplacementY1.setMaxSpeed(VITESSE_MAX);
    moteurDeplacementY1.setAcceleration(ACCELERATION);
    moteurDeplacementY2.setMaxSpeed(VITESSE_MAX);
    moteurDeplacementY2.setAcceleration(ACCELERATION);
    moteurRotationOutil.setMaxSpeed(VITESSE_MAX);
    moteurRotationOutil.setAcceleration(ACCELERATION);
    moteurHauteurOutil.setMaxSpeed(VITESSE_MAX);
    moteurHauteurOutil.setAcceleration(ACCELERATION);

    Serial.println("MultiStepper initialisé");
    multiStepper.addStepper(moteurDeplacementX);
    multiStepper.addStepper(moteurDeplacementY1);
    multiStepper.addStepper(moteurDeplacementY2);
    multiStepper.addStepper(moteurHauteurOutil);
    multiStepper.addStepper(moteurRotationOutil);

    homeAxes();

    Serial.println("Fin du setup");
}

// ======================= Loop =======================
void loop() {

    if (Serial.available()) {
        String incoming = Serial.readStringUntil('\n');
        processSerialCommand(incoming);
    }    

    if (!isStarted) {
        delay(100);
        return;
    }

    // Vérifier si le tableau de commandes G-code contient des éléments
    if (!emergencyStop && gcodeIndex < gcode_command.size()) {
        Serial.println("Loop en cours, gcodeIndex = " + String(gcodeIndex));
        String command = gcode_command[gcodeIndex++];
        Serial.println("Exécution : " + command);
        executeGCodeCommand(command);
    } 
    
    else if (emergencyStop) {
        Serial.println("Système bloqué. Appuyez sur le bouton RESET.");
        while (emergencyStop) {
            delay(100);
        }
    }
    
    else if (!gcodeFinished) {
        Serial.println("Toutes les commandes G-code ont été exécutées.");
        gcodeFinished = true;
        // gcodeIndex = 0; ← tu peux le commenter si tu veux garder l'état pour relancer manuellement
    }

    checkLimitSwitches();

    float poids = readLoadCell();
    Serial.print("Poids détecté : ");
    Serial.print(poids, 3);  // Affiche avec 3 décimales
    Serial.println(" kg");
    delay(500);  // Lis toutes les 500 ms

    yield();
}