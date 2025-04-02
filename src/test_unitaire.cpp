#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <vector>

// ======================= Booléens de contrôle global ====================
volatile bool emergencyStop = false;
volatile bool isPaused = false;
volatile bool isStarted = true;

// ======================= INTERRUPT HANDLER ==============================
void handleEmergencyStop() {
    emergencyStop = true;
}

// ======================= Déclaration du buffer G-code ===================
std::vector<String> gcodeBuffer;
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
#define Pression       12

// ======================= Boutons ========================================
#define PIN_RESET_URGENCE  36  // Choisis une pin libre (ex. GPIO15)
#define PIN_PAUSE          35  // GPIO libre pour Pause
#define PIN_START          34  // GPIO libre pour Start

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
#define VITESSE_MAX    3000
#define VITESSE_COUPE  1500
#define VITESSE_HOME   1000
#define ACCELERATION   3000
#define PAS_PAR_DEGREE 10
#define ERREUR_MAX_Y   10

// ======================= STOCKAGE DU G-CODE ==========================
std::vector<String> gcode_command = {
        "G1 X36.5658 Y253.5164 Z0 Zrot-90.0000",
        "G1 X60.0370 Y253.5164 Z0 Zrot0.0000",
        "G1 X59.7900 Y229.0568 Z0 Zrot90.5787",
        "G1 X36.5658 Y228.5627 Z0 Zrot178.7811"
    };

// ======================= STOCKAGE DU G-CODE ==========================
void storeGCode(const char* gcode) {
    gcodeBuffer.push_back(String(gcode));
    Serial.println(gcode);
}

// ======================= MOUVEMENT DES MOTEURS =======================
void moveXYZ(float x, float y, float z, float zRot) {
    long stepsX = x * PAS_PAR_MM_X;
    long stepsY = y * PAS_PAR_MM_Y;
    long stepsZ = z * PAS_PAR_MM_Z;
    long stepsRotationOutil = zRot * PAS_PAR_DEGREE;

    // Tableau contenant les positions cibles pour les moteurs gérés par MultiStepper.
    // L'ordre des valeurs doit correspondre à l'ordre des moteurs ajoutés à multiStepper.
    long positions[4] = { stepsX, stepsY, stepsZ, stepsRotationOutil };


    Serial.println("Commande reçue : X=" + String(x) + " Y=" + String(y) + " Z=" + String(z) + " ZRot=" + String(zRot));
    Serial.println("Steps X: " + String(stepsX) + " Y: " + String(stepsY));
    multiStepper.moveTo(positions);

    while (
        moteurDeplacementX.distanceToGo() ||
        moteurDeplacementY1.distanceToGo() ||
        moteurDeplacementY2.distanceToGo() ||
        moteurRotationOutil.distanceToGo() ||
        moteurHauteurOutil.distanceToGo()
    ) {
        if (emergencyStop) {
            Serial.println("Arrêt d'urgence déclenché !");
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

    // Move Y towards MIN limit switch
    moteurDeplacementY1.setSpeed(VITESSE_HOME);  // Move in negative direction
    moteurDeplacementY2.setSpeed(VITESSE_HOME);
    while (digitalRead(LIMIT_Y_MIN_R) != HIGH) {
        if (digitalRead(LIMIT_Y_MIN_R) == HIGH) Serial.println("Y Min Switch Pressed!");
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
    moteurHauteurOutil.setCurrentPosition(40*PAS_PAR_MM_Z);
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

    // Position centrale après homing
    moveXYZ(175, -175, 10, 0);
    moteurDeplacementX.setCurrentPosition(0);
    moteurDeplacementY1.setCurrentPosition(0);
    moteurDeplacementY2.setCurrentPosition(0);
    moteurHauteurOutil.setCurrentPosition(0);
    moteurRotationOutil.setCurrentPosition(0);
    Serial.println("Positionné au centre après homing.");
}

// ======================= EXÉCUTION DU G-CODE =======================
void executeGCodeCommand(const String& command) {
    // Variables pour extraire les valeurs du G-code
    float g = 0, x = 0, y = 0, z = 0, zRot = 0;
    char gcode[20];
    int readCount = sscanf(command.c_str(), "G%f X%f Y%f Z%f ZRot%f", &g, &x, &y, &z, &zRot);

    // Ignore les lignes qui ne commencent pas par G00 ou G01
    //if (!(command.startsWith("G00") || command.startsWith("G01"))) {
    //    Serial.println("Commande ignorée : " + command);
    //    return;
    //}

    //Serial.println("Commande acceptée : " + command);

    // === Appliquer les vitesses selon G00 ou G01 ===
    //if ((int)g == 0) {  // G00 : déplacement rapide
    //    Serial.println("Commande G00 (rapide)");
    //    moteurDeplacementX.setMaxSpeed(VITESSE_MAX);
    //    moteurDeplacementY1.setMaxSpeed(VITESSE_MAX);
    //    moteurDeplacementY2.setMaxSpeed(VITESSE_MAX);
    //    moteurRotationOutil.setMaxSpeed(VITESSE_MAX);
    //    moteurHauteurOutil.setMaxSpeed(VITESSE_MAX);
   // } else if ((int)g == 1) {  // G01 : déplacement contrôlé
     //   Serial.println("Commande G01 (contrôlée)");
      //  moteurDeplacementX.setMaxSpeed(VITESSE_COUPE);
     //   moteurDeplacementY1.setMaxSpeed(VITESSE_COUPE);
    //    moteurDeplacementY2.setMaxSpeed(VITESSE_COUPE);
     //   moteurRotationOutil.setMaxSpeed(VITESSE_COUPE);
     //   moteurHauteurOutil.setMaxSpeed(VITESSE_COUPE);
    //} else {
      //  Serial.println("Commande ignorée (pas G00 ni G01) : " + command);
    //}
    // Exécuter le mouvement en fonction du G-code
    moveXYZ(x, y, z, zRot);
}

// ======================= Boutons ===================================
void checkEmergencyReset() {
    if (digitalRead(PIN_RESET_URGENCE) == LOW) {
        emergencyStop = false;
        Serial.println("Reset d’urgence effectué.");
    }
}

void handlePauseButton() {
    isPaused = !isPaused;  // Toggle pause
    Serial.println(isPaused ? "Pause activée" : "Reprise");
}

void handleStartButton() {
    isStarted = true;
    Serial.println("Démarrage demandé");
}

// ======================= printLimitSwitchStates ====================
void printLimitSwitchStates() {
    if (digitalRead(LIMIT_X_MIN) == LOW) Serial.println("X Min Switch Pressed!");
    if (digitalRead(LIMIT_X_MAX) == LOW) Serial.println("X Max Switch Pressed!");
    if (digitalRead(LIMIT_Y_MIN_L) == LOW) Serial.println("Y Min L Switch Pressed!");
    if (digitalRead(LIMIT_Y_MIN_R) == LOW) Serial.println("Y Min R Switch Pressed!");
    if (digitalRead(LIMIT_Y_MAX_L) == LOW) Serial.println("Y Max L Switch Pressed!");
    if (digitalRead(LIMIT_Y_MAX_R) == LOW) Serial.println("Y Max R Switch Pressed!");
    if (digitalRead(LIMIT_Z_MAX) == LOW) Serial.println("Z Max Switch Pressed!");
    if (digitalRead(LIMIT_ZRot) == LOW) Serial.println("ZRot Switch Pressed!");
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

    // Boutons
    Serial.println("Boutons configurés");
    pinMode(PIN_RESET_URGENCE, INPUT_PULLUP);
    pinMode(PIN_PAUSE, INPUT_PULLUP);
    pinMode(PIN_START, INPUT_PULLUP);

    /*
    // Attacher les interruptions
    Serial.println("Interruptions attachées");
    attachInterrupt(digitalPinToInterrupt(LIMIT_X_MIN), handleEmergencyStop, FALLING);
    attachInterrupt(digitalPinToInterrupt(LIMIT_X_MAX), handleEmergencyStop, FALLING);
    attachInterrupt(digitalPinToInterrupt(LIMIT_Y_MIN_L), handleEmergencyStop, FALLING);
    attachInterrupt(digitalPinToInterrupt(LIMIT_Y_MIN_R), handleEmergencyStop, FALLING);
    attachInterrupt(digitalPinToInterrupt(LIMIT_Y_MAX_L), handleEmergencyStop, FALLING);
    attachInterrupt(digitalPinToInterrupt(LIMIT_Y_MAX_R), handleEmergencyStop, FALLING);
    attachInterrupt(digitalPinToInterrupt(LIMIT_Z_MAX), handleEmergencyStop, FALLING);
    attachInterrupt(digitalPinToInterrupt(LIMIT_ZRot), handleEmergencyStop, FALLING);
    Serial.println("Interruptions 2 attachées");
    attachInterrupt(digitalPinToInterrupt(PIN_PAUSE), handlePauseButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_START), handleStartButton, FALLING);
    */

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

    homeAxes(); // Tu peux l’activer si tu veux faire un homing au départ

    Serial.println("Fin du setup");
}

// ======================= Loop =======================
void loop() {

    //printLimitSwitchStates();

    if (!isStarted) {
        Serial.println("En attente du démarrage...");
        while (!isStarted) {
            delay(100);
        }
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
            checkEmergencyReset();  // Autorise un déblocage manuel
            delay(100);
        }
    }

    else {
        Serial.println("Toutes les commandes G-code ont été exécutées.");
        gcodeIndex = 0;
    }

    yield();
}