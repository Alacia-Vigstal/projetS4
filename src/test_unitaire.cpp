#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <AccelStepper.h>
#include <vector>
#include "circle_gcode.hpp" // Inclure le fichier de génération du G-code

// Déclaration du buffer G-code
std::vector<String> gcodeBuffer;
int gcodeIndex = 0;

// ======================= CONFIGURATION DES LIMIT SWITCH =================
#define LIMIT_X_MIN    4
#define LIMIT_X_MAX    34
#define LIMIT_Y_MIN_L  35
#define LIMIT_Y_MIN_R  23
#define LIMIT_Y_MAX_L  25
#define LIMIT_Y_MAX_R  14
#define LIMIT_Z_MAX    13
#define LIMIT_ZRot     12

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

// ======================= CONFIGURATION DES PARAMETRES =======================
#define PAS_PAR_MM_X   80
#define PAS_PAR_MM_Y   80
#define PAS_PAR_MM_Z   400
#define VITESSE_MAX    3000
#define ACCELERATION   3000
#define PAS_PAR_DEGREE 10
#define ERREUR_MAX_Y   10

// ======================= HOMING FUNCTION =======================
void homeAxes() {
    Serial.println("Homing X and Y...");

    // Move X towards MIN limit switch
    moteurDeplacementX.setSpeed(-1000);  // Move in negative direction
    while (digitalRead(LIMIT_X_MIN) == HIGH) {
        moteurDeplacementX.runSpeed();
    }
    moteurDeplacementX.stop();  
    moteurDeplacementX.setCurrentPosition(0);  // Set home position

    // Move Y towards MIN limit switch (both motors)
    moteurDeplacementY1.setSpeed(-1000);
    moteurDeplacementY2.setSpeed(-1000);
    while (digitalRead(LIMIT_Y_MIN_L) == HIGH && digitalRead(LIMIT_Y_MIN_R) == HIGH) {
        moteurDeplacementY1.runSpeed();
        moteurDeplacementY2.runSpeed();
    }
    moteurDeplacementY1.stop();
    moteurDeplacementY2.stop();
    moteurDeplacementY1.setCurrentPosition(0);
    moteurDeplacementY2.setCurrentPosition(0);

    Serial.println("Homing completed!");
}

// ======================= STOCKAGE DU G-CODE =======================
std::vector<String> gcode_command = {
        "G0 X0 Y0 Z0 ZRot0",
        "G0 X100 Y0 Z10 ZRot90",
        "G0 X100 Y100 Z0 ZRot180",
        "G0 X0 Y100 Z10 ZRot90",
        "G0 X0 Y0 Z0 ZRot0",
    };


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

    moteurHauteurOutil.moveTo(stepsZ);
    moteurRotationOutil.moveTo(stepsRotationOutil);
    moteurDeplacementX.moveTo(stepsX);
    moteurDeplacementY1.moveTo(stepsY);
    moteurDeplacementY2.moveTo(stepsY);

    // Boucle de mise à jour optimisée
    while (moteurDeplacementX.distanceToGo() || moteurDeplacementY1.distanceToGo() || moteurDeplacementY2.distanceToGo() || moteurRotationOutil.distanceToGo() || moteurHauteurOutil.distanceToGo()) {
        // Mettez à jour tous les moteurs simultanément
        moteurDeplacementX.run();
        moteurDeplacementY1.run();
        moteurDeplacementY2.run();
        moteurRotationOutil.run();
        moteurHauteurOutil.run();

        // Gestion dynamique de la vitesse en fonction de la distance restante
        if (moteurDeplacementX.distanceToGo() < 100) {
            moteurDeplacementX.setMaxSpeed(1500);
        } else {
            moteurDeplacementX.setMaxSpeed(VITESSE_MAX);
        }
    }
}

// ======================= EXÉCUTION DU G-CODE =======================
void executeGCodeCommand(const String& command) {
    // Variables pour extraire les valeurs du G-code
    float g = 0, x = 0, y = 0, z = 0, zRot = 0;
    char gcode[20];
    int readCount = sscanf(command.c_str(), "G%f X%f Y%f Z%f ZRot%f", &g, &x, &y, &z, &zRot);

    // Exécuter le mouvement en fonction du G-code
    moveXYZ(x, y, z, zRot);
}

// ======================= Setup =======================
void setup() {
    Serial.begin(115200);

    // Set limit switches as inputs with pull-up resistors
    pinMode(LIMIT_X_MIN, INPUT_PULLUP);
    pinMode(LIMIT_X_MAX, INPUT_PULLUP);
    pinMode(LIMIT_Y_MIN_L, INPUT_PULLUP);
    pinMode(LIMIT_Y_MIN_R, INPUT_PULLUP);
    pinMode(LIMIT_Y_MAX_L, INPUT_PULLUP);
    pinMode(LIMIT_Y_MAX_R, INPUT_PULLUP);
    pinMode(LIMIT_Z_MAX, INPUT_PULLUP);
    pinMode(LIMIT_ZRot, INPUT_PULLUP);

    // Vérification de l'état initial
    Serial.println("Vérification des limit switches au démarrage:");
    //Serial.print("X Min: "); Serial.println(digitalRead(LIMIT_X_MIN));
    //Serial.print("X Max: "); Serial.println(digitalRead(LIMIT_X_MAX));
    Serial.print("Y Min L: "); Serial.println(digitalRead(LIMIT_Y_MIN_L));
    Serial.print("Y Min R: "); Serial.println(digitalRead(LIMIT_Y_MIN_R));
    Serial.print("Y Max L: "); Serial.println(digitalRead(LIMIT_Y_MAX_L));
    Serial.print("Y Max R: "); Serial.println(digitalRead(LIMIT_Y_MAX_R));
    //Serial.print("Z Max: "); Serial.println(digitalRead(LIMIT_Z_MAX));
    //Serial.print("Z Rot: "); Serial.println(digitalRead(LIMIT_ZRot));
    
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
}

// ======================= Loop =======================
void loop() {
    // Vérifier si le tableau de commandes G-code contient des éléments
    if (gcodeIndex < gcode_command.size()) {
        // Récupérer la commande à exécuter à l'index actuel
        String command = gcode_command[gcodeIndex++];
        
        // Afficher la commande en cours d'exécution
        Serial.println("Exécution du G-code : " + command);
        
        // Exécuter la commande en appelant la fonction correspondante
        executeGCodeCommand(command);
    } 
    else {
        // Si toutes les commandes ont été exécutées, relancer l'exécution
        gcodeIndex = 0; // Réinitialiser l'index pour recommencer le cycle
        Serial.println("Toutes les commandes G-code ont été exécutées, recommencez.");
    }

    /*
    if (digitalRead(LIMIT_X_MIN) == LOW) {
        Serial.println("X Min Switch Pressed!");
    }
    if (digitalRead(LIMIT_X_MAX) == LOW) {
        Serial.println("X Max Switch Pressed!");
    }
    */
    /*if (digitalRead(LIMIT_Y_MIN_L) == LOW) {
        Serial.println("Y Min L Switch Pressed!");
    }*/
    if (digitalRead(LIMIT_Y_MIN_R) == LOW) {
        Serial.println("Y Min R(L temp) Switch Pressed!");
    }
    if (digitalRead(LIMIT_Y_MAX_L) == LOW) {
        Serial.println("Y Max L Switch Pressed!");
    }
    if (digitalRead(LIMIT_Y_MAX_R) == LOW) {
        Serial.println("Y Max R Switch Pressed!");
    }
    /*
    if (digitalRead(LIMIT_Z_MAX) == LOW) {
        Serial.println("Z Min Switch Pressed!");
    }
    if (digitalRead(LIMIT_ZRot) == LOW) {
        Serial.println("ZRot Switch Pressed!");
    }
    */

    yield();  // Small delay to reduce serial spam
}

//Pour le code en temps réel
/* 
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void TaskBlink(void *pvParameters);
void TaskSerial(void *pvParameters);

void setup() {
    Serial.begin(115200);
    
    xTaskCreate(TaskBlink, "LED Task", 1000, NULL, 1, NULL);
    xTaskCreate(TaskSerial, "Serial Task", 1000, NULL, 1, NULL);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000)); // Laisser FreeRTOS gérer
}

// Tâche pour clignoter une LED
void TaskBlink(void *pvParameters) {
    pinMode(LED_BUILTIN, OUTPUT);
    while (1) {
        digitalWrite(LED_BUILTIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(500)); 
        digitalWrite(LED_BUILTIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

// Tâche pour envoyer un message série
void TaskSerial(void *pvParameters) {
    while (1) {
        Serial.println("Hello from FreeRTOS on ESP32!");
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}
*/