#include <Arduino.h>
#include <AccelStepper.h>
#include <vector>
#include "circle_gcode.hpp" // Inclure le fichier de génération du G-code

// Déclaration du buffer G-code
std::vector<String> gcodeBuffer;
int gcodeIndex = 0;

// ======================= CONFIGURATION DES MOTEURS =======================
#define STEP_X  2
#define DIR_X   5
#define STEP_Y1 26
#define DIR_Y1  27
#define STEP_Y2 21
#define DIR_Y2  22
#define STEP_ROTATION_OUTIL  18
#define DIR_ROTATION_OUTIL   19
#define STEP_Z  32
#define DIR_Z   33

AccelStepper moteurDeplacementX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper moteurDeplacementY1(AccelStepper::DRIVER, STEP_Y1, DIR_Y1);
AccelStepper moteurDeplacementY2(AccelStepper::DRIVER, STEP_Y2, DIR_Y2);
AccelStepper moteurRotationOutil(AccelStepper::DRIVER, STEP_ROTATION_OUTIL, DIR_ROTATION_OUTIL);
AccelStepper moteurHauteurOutil(AccelStepper::DRIVER, STEP_Z, DIR_Z);

#define PAS_PAR_MM_X   80
#define PAS_PAR_MM_Y   80
#define PAS_PAR_MM_Z   400
#define VITESSE_MAX    3000
#define ACCELERATION   3000
#define PAS_PAR_DEGREE 10
#define ERREUR_MAX_Y   10

// ======================= STOCKAGE DU G-CODE =======================
void storeGCode(const char* gcode) {
    gcodeBuffer.push_back(String(gcode));
    Serial.println(gcode);
}

// ======================= MOUVEMENT DES MOTEURS =======================
void moveXYZ(float x, float y, float z, float zRot) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    long stepsX = x * PAS_PAR_MM_X;
    long stepsY = y * PAS_PAR_MM_Y;
    long stepsZ = z * PAS_PAR_MM_Z;
    long stepsRotationOutil = zRot * PAS_PAR_DEGREE;

    // Définir les nouvelles positions cibles
    moteurHauteurOutil.moveTo(stepsZ);
    moteurRotationOutil.moveTo(stepsRotationOutil);
    moteurDeplacementX.moveTo(stepsX);
    moteurDeplacementY1.moveTo(stepsY);
    moteurDeplacementY2.moveTo(stepsY);

    // Boucle de mise à jour en temps réel
    while (moteurHauteurOutil.distanceToGo() ||
           moteurRotationOutil.distanceToGo() ||
           moteurDeplacementX.distanceToGo() ||
           moteurDeplacementY1.distanceToGo() ||
           moteurDeplacementY2.distanceToGo()) {

        // Mettre à jour tous les moteurs simultanément
        moteurHauteurOutil.run();
        moteurRotationOutil.run();
        moteurDeplacementX.run();
        moteurDeplacementY1.run();
        moteurDeplacementY2.run();

        // Vérifier l'alignement des moteurs Y1 et Y2 en temps réel
        long erreur = moteurDeplacementY1.currentPosition() - moteurDeplacementY2.currentPosition();
        if (abs(erreur) > 2 && abs(erreur) < ERREUR_MAX_Y) {
            moteurDeplacementY2.move(erreur > 0 ? 1 : -1);
            moteurDeplacementY2.run();
        } else if (abs(erreur) >= ERREUR_MAX_Y) {
            Serial.println("Alerte: Désalignement excessif de Y2 !");
        }
    }
}

// ======================= Setup =======================
void setup() {
    Serial.begin(115200);

    moteurDeplacementY2.setPinsInverted(true, false, false);

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
    CircleGCode circle(0, 0, 100, 1000);
    circle.generateGCode(storeGCode);
    Serial.println("Setup terminé, démarrage du loop...");
}

// ======================= Loop =======================
void loop() {
    if (gcodeIndex < gcodeBuffer.size()) {
        String command = gcodeBuffer[gcodeIndex++];
        Serial.println("Exécution du G-code : " + command);

        float g = 0, x = 0, y = 0, z = 0, zRot = 0;
        if (sscanf(command.c_str(), "G%f X%f Y%f Z%f ZRot%f", &g, &x, &y, &z, &zRot) >= 2) {
            moveXYZ(x, y, z, zRot);
        } else {
            Serial.println("Commande G-code invalide !");
        }
    } else {
        yield();
    }
}



/*
#include <Arduino.h>
#include <AccelStepper.h>
#include <vector>
#include "circle_gcode.hpp" // Inclure le fichier de génération du G-code

// Déclaration du buffer G-code
std::vector<String> gcodeBuffer;
int gcodeIndex = 0;

// ======================= CONFIGURATION DES MOTEURS =======================
#define STEP_X  2
#define DIR_X   5
#define STEP_Y1 18
#define DIR_Y1  19
#define STEP_Y2 21
#define DIR_Y2  22
#define STEP_ROTATION_OUTIL  26
#define DIR_ROTATION_OUTIL   27
#define STEP_Z  32
#define DIR_Z   33

AccelStepper moteurDeplacementX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper moteurDeplacementY1(AccelStepper::DRIVER, STEP_Y1, DIR_Y1);
AccelStepper moteurDeplacementY2(AccelStepper::DRIVER, STEP_Y2, DIR_Y2);
AccelStepper moteurRotationOutil(AccelStepper::DRIVER, STEP_ROTATION_OUTIL, DIR_ROTATION_OUTIL);
AccelStepper moteurHauteurOutil(AccelStepper::DRIVER, STEP_Z, DIR_Z);

#define PAS_PAR_MM_X   80
#define PAS_PAR_MM_Y   80
#define PAS_PAR_MM_Z   400
#define VITESSE_MAX    5000
#define ACCELERATION   3000
#define PAS_PAR_DEGREE 10 // Conversion des degrés en pas pour l'axe de rotation

// ======================= STOCKAGE DU G-CODE =======================
void storeGCode(const char* gcode) {
    gcodeBuffer.push_back(String(gcode));
    Serial.println(gcode);  // Afficher le G-code généré dans le terminal
}

// ======================= MOUVEMENT DES MOTEURS =======================
void moveXYZ(float x, float y, float z) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    long stepsX = x * PAS_PAR_MM_X;
    long stepsY = y * PAS_PAR_MM_Y;
    long stepsZ = z * PAS_PAR_MM_Z;
    long stepsRotationOutil = atan2(y, x) * (180.0 / M_PI) * PAS_PAR_DEGREE;

    moteurDeplacementX.moveTo(stepsX);
    moteurDeplacementY1.moveTo(stepsY);
    moteurDeplacementY2.moveTo(stepsY);
    moteurRotationOutil.moveTo(stepsRotationOutil);
    moteurHauteurOutil.moveTo(stepsZ);

    while (moteurDeplacementX.distanceToGo() || moteurDeplacementY1.distanceToGo() || moteurDeplacementY2.distanceToGo() || moteurRotationOutil.distanceToGo() || moteurHauteurOutil.distanceToGo()) {
        moteurDeplacementX.run();
        moteurDeplacementY1.run();
        moteurDeplacementY2.run();
        moteurRotationOutil.run();
        moteurHauteurOutil.run();
    }
}

// ======================= SetUp =======================
void setup() {
    Serial.begin(115200);

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
    CircleGCode circle(0, 0, 100, 1000); // position x, position y, rayon, segments
    circle.generateGCode(storeGCode);
    Serial.println("Setup terminé, démarrage du loop...");
}

// ======================= Loop =======================
void loop() {
    if (gcodeIndex < gcodeBuffer.size()) {
        String command = gcodeBuffer[gcodeIndex++];
        Serial.println("Exécution du G-code : " + command);

        float g = 0, x = 0, y = 0, z = 0;
        if (sscanf(command.c_str(), "G%f X%f Y%f Z%f", &g, &x, &y, &z) >= 2) {
            moveXYZ(x, y, z);
        }
    } else {
        yield();
    }
}
*/