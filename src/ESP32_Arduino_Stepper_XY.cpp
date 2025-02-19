/*
#include <Arduino.h>
#include <AccelStepper.h>

//Tests
#include "circle_gcode.hpp"

// ======================= CONFIGURATION DES MOTEURS =======================
#define STEP_X  21
#define DIR_X   22
#define STEP_Y1 26
#define DIR_Y1  27
#define STEP_Y2 2
#define DIR_Y2  5
#define STEP_ROTATION_OUTIL  18
#define DIR_ROTATION_OUTIL   19
#define STEP_Z  32
#define DIR_Z   33

#define X_MIN_PIN 34
#define X_MAX_PIN 35
#define Y_MIN_PIN 36
#define Y_MAX_PIN 39
#define Z_MIN_PIN 13
#define Z_MAX_PIN 14
#define PRESSURE_SENSOR_PIN 15

#define X_MAX_LIMIT  450  // Limite maximale de déplacement en mm sur X
#define Y_MAX_LIMIT  450  // Limite maximale de déplacement en mm sur Y
#define Z_MAX_LIMIT  20   // Course maximale de l'axe Z en mm
#define PRESSURE_MIN 100 // Seuil minimal de pression
#define PRESSURE_MAX 500
#define PRESSURE_HARD_LIMIT 600 // Pression à ne jamais dépasser

AccelStepper moteurDeplacementX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper moteurDeplacementY1(AccelStepper::DRIVER, STEP_Y1, DIR_Y1);
AccelStepper moteurDeplacementY2(AccelStepper::DRIVER, STEP_Y2, DIR_Y2);
AccelStepper moteurRotationOutil(AccelStepper::DRIVER, STEP_ROTATION_OUTIL, DIR_ROTATION_OUTIL);
AccelStepper moteurHauteurOutil(AccelStepper::DRIVER, STEP_Z, DIR_Z);

#define PAS_PAR_MM_X   80
#define PAS_PAR_MM_Y   80
#define PAS_PAR_MM_Z   400
#define VITESSE_MAX    3000
#define ACCELERATION   1500
#define PAS_PAR_DEGREE 10 // Conversion des degrés en pas pour l'axe de rotation


// =======================       Tests        ======================
void sendToSerial(const char* gcode) {
    Serial.print(gcode);  // Envoie le G-code via le port série
}


// ======================= Hauteur | Pression ======================
// Fonction pour ajuster la hauteur de l'outil en fonction de la pression
void ajusterHauteurOutil() {
    int pression = analogRead(PRESSURE_SENSOR_PIN);
    if (pression < PRESSURE_MIN) {
        moteurHauteurOutil.move(-PAS_PAR_MM_Z); // Descend l'outil
    } else if (pression > PRESSURE_HARD_LIMIT) {
        Serial.println("Alerte : Pression excessive, arrêt immédiat !");
        moteurHauteurOutil.stop();
        return;
    } else if (pression > PRESSURE_MAX) {
        moteurHauteurOutil.move(PAS_PAR_MM_Z); // Monte l'outil
    }
    moteurHauteurOutil.run();
}


// ======================= HOMING (Calibrage) =======================
void homing() {
    Serial.println("Début du homing...");

    // Déplacement vers les capteurs de fin de course
    moteurDeplacementX.setSpeed(-VITESSE_MAX / 2);
    moteurDeplacementY1.setSpeed(-VITESSE_MAX / 2);
    moteurDeplacementY2.setSpeed(-VITESSE_MAX / 2);

    while (digitalRead(X_MIN_PIN) == HIGH || digitalRead(Y_MIN_PIN) == HIGH) {
        if (digitalRead(X_MIN_PIN) == HIGH) moteurDeplacementX.runSpeed();
        if (digitalRead(Y_MIN_PIN) == HIGH) {
            moteurDeplacementY1.runSpeed();
            moteurDeplacementY2.runSpeed();
        }
    }

    // Attendre un moment puis reculer légèrement
    delay(100);

    moteurDeplacementX.setSpeed(VITESSE_MAX / 4);
    moteurDeplacementY1.setSpeed(VITESSE_MAX / 4);
    moteurDeplacementY2.setSpeed(VITESSE_MAX / 4);
    
    for (int i = 0; i < 10; i++) {
        moteurDeplacementX.runSpeed();
        moteurDeplacementY1.runSpeed();
        moteurDeplacementY2.runSpeed();
    }

    // Définir la position actuelle comme origine
    moteurDeplacementX.setCurrentPosition(0);
    moteurDeplacementY1.setCurrentPosition(0);
    moteurDeplacementY2.setCurrentPosition(0);

    Serial.println("Homing terminé !");
}


// ======================= MOUVEMENT DES MOTEURS =======================
void moveXYZ(float x, float y, float z) {
    // Vérifier les limites de la machine
    if (x < 0) x = 0;
    if (x > X_MAX_LIMIT) x = X_MAX_LIMIT;
    if (y < 0) y = 0;
    if (y > Y_MAX_LIMIT) y = Y_MAX_LIMIT;

    long stepsX = x * PAS_PAR_MM_X;
    long stepsY = y * PAS_PAR_MM_Y;
    float angle = atan2(y, x) * (180.0 / M_PI); // Calcul de l'angle en degrés
    long stepsRotationOutil = angle * PAS_PAR_DEGREE;
    long stepsZ = z * PAS_PAR_MM_Z; // Rotation en degrés et non translation // Conversion en pas moteur

    moteurDeplacementX.moveTo(stepsX);
    moteurDeplacementY1.moveTo(stepsY);
    moteurDeplacementY2.moveTo(stepsY);
    moteurRotationOutil.moveTo(stepsRotationOutil);
    moteurHauteurOutil.moveTo(stepsZ);

    while (moteurDeplacementX.distanceToGo() != 0 || moteurDeplacementY1.distanceToGo() != 0 || moteurDeplacementY2.distanceToGo() != 0 || moteurHauteurOutil.distanceToGo() != 0) {
        if (digitalRead(X_MIN_PIN) == LOW || digitalRead(X_MAX_PIN) == LOW || digitalRead(Y_MIN_PIN) == LOW || digitalRead(Y_MAX_PIN) == LOW) {
            Serial.println("Avertissement : Limite atteinte, arrêt des moteurs !");
            break;
        }
        moteurDeplacementX.run();
        moteurDeplacementY1.run();
        moteurDeplacementY2.run();
        moteurRotationOutil.run();
        moteurHauteurOutil.run();
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(X_MIN_PIN, INPUT_PULLUP);
    pinMode(X_MAX_PIN, INPUT_PULLUP);
    pinMode(Y_MIN_PIN, INPUT_PULLUP);
    pinMode(Y_MAX_PIN, INPUT_PULLUP);
    pinMode(Z_MIN_PIN, INPUT_PULLUP);
    pinMode(Z_MAX_PIN, INPUT_PULLUP);
    pinMode(PRESSURE_SENSOR_PIN, INPUT);

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

    Serial.println("ESP32 prêt à contrôler les moteurs !");

    //homing();

    //Tests
    CircleGCode circle(50, 50, 20, 36);
    circle.generateGCode(sendToSerial);  // Génère et envoie le G-code

    Serial.println("Setup terminé, démarrage du loop...");
}


// ======================= RÉCEPTION DU G-CODE AMÉLIORÉ =======================
void loop() {
    Serial.println("Attente d'une commande...");

    static bool firstCommand = true;
    if (Serial1.available()) {
        if (firstCommand) {
            Serial.println("Exécution du homing avant de commencer le G-code...");
            //homing();
            firstCommand = false;
        }

        String command = Serial1.readStringUntil('\n');
        command.trim();

        // Vérifier si c'est la fin du G-code (ex: M30 ou commande vide)
        if (command == "M30") {
            Serial.println("Fin du G-code, attente de nouvelles commandes...");
            firstCommand = true; 
            return;
        }
        if (command.length() == 0) {  
            return; // Ignore les lignes vides pour éviter un homing inutile
        }


        float x = 0, y = 0;
        if (sscanf(command.c_str(), "G%d X%f Y%f", &x, &y) >= 2) {
            Serial.print("Commande reçue : X=");
            Serial.print(x);
            Serial.print(" Y=");
            Serial.println(y);

            moveXYZ(x, y, 0);
        }
    }
}
*/