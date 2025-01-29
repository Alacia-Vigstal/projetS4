#include <Arduino.h>
#include <AccelStepper.h>
#include <TMCStepper.h>
#include <math.h>

// ======================= CONFIGURATION DES MOTEURS =======================
#define STEP_X  2
#define DIR_X   5
#define STEP_Y1 3
#define DIR_Y1  6
#define STEP_Y2 4
#define DIR_Y2  7
#define STEP_ROTATION_OUTIL  9
#define DIR_ROTATION_OUTIL   10
#define STEP_Z 13
#define DIR_Z 14

#define SERIAL_PORT Serial2  // UART2 sur Mega2560
#define DRIVER_ADDRESS 0x00  // Adresse par défaut des TMC2209

#define X_MIN_PIN 8
#define X_MAX_PIN 11
#define Y_MIN_PIN 9
#define Y_MAX_PIN 12
#define Z_MIN_PIN 15
#define Z_MAX_PIN 16
#define PRESSURE_SENSOR_PIN A0

#define X_MAX_LIMIT 500  // Limite maximale de déplacement en mm sur X
#define Y_MAX_LIMIT 500  // Limite maximale de déplacement en mm sur Y
#define Z_MAX_LIMIT 20   // Course maximale de l'axe Z en mm
#define PRESSURE_MIN 100 // Seuil minimal de pression
#define PRESSURE_MAX 500
#define PRESSURE_HARD_LIMIT 600 // Pression à ne jamais dépasser // Seuil maximal de pression

AccelStepper moteurDeplacementX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper moteurDeplacementY1(AccelStepper::DRIVER, STEP_Y1, DIR_Y1);
AccelStepper moteurDeplacementY2(AccelStepper::DRIVER, STEP_Y2, DIR_Y2);
AccelStepper moteurRotationOutil(AccelStepper::DRIVER, STEP_ROTATION_OUTIL, DIR_ROTATION_OUTIL);
AccelStepper moteurHauteurOutil(AccelStepper::DRIVER, STEP_Z, DIR_Z);

TMC2209Stepper driverX(&SERIAL_PORT, DRIVER_ADDRESS);
TMC2209Stepper driverY1(&SERIAL_PORT, DRIVER_ADDRESS);
TMC2209Stepper driverY2(&SERIAL_PORT, DRIVER_ADDRESS);
TMC2209Stepper driverRotationOutil(&SERIAL_PORT, DRIVER_ADDRESS);
TMC2209Stepper driverHauteurOutil(&SERIAL_PORT, DRIVER_ADDRESS);

#define PAS_PAR_MM_X   80
#define PAS_PAR_MM_Y   80
#define PAS_PAR_MM_Z   400  // Calcul basé sur vis T8 (8mm par tour) avec 200 pas par tour
#define VITESSE_MAX    3000
#define ACCELERATION   1500
#define PAS_PAR_DEGREE 10 // Conversion des degrés en pas pour l'axe de rotation

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    SERIAL_PORT.begin(115200);

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

    driverX.begin();
    driverX.toff(4);
    driverX.rms_current(800);
    driverX.microsteps(16);
    driverX.pwm_autoscale(true);
    driverX.en_pwm_mode(true);

    driverY1.begin();
    driverY1.toff(4);
    driverY1.rms_current(800);
    driverY1.microsteps(16);
    driverY1.pwm_autoscale(true);
    driverY1.en_pwm_mode(true);

    driverY2.begin();
    driverY2.toff(4);
    driverY2.rms_current(800);
    driverY2.microsteps(16);
    driverY2.pwm_autoscale(true);
    driverY2.en_pwm_mode(true);
    
    driverRotationOutil.begin();
    driverRotationOutil.toff(4);
    driverRotationOutil.rms_current(800);
    driverRotationOutil.microsteps(16);
    driverRotationOutil.pwm_autoscale(true);
    driverRotationOutil.en_pwm_mode(true);

    driverHauteurOutil.begin();
    driverHauteurOutil.toff(4);
    driverHauteurOutil.rms_current(800);
    driverHauteurOutil.microsteps(16);
    driverHauteurOutil.pwm_autoscale(true);
    driverHauteurOutil.en_pwm_mode(true);

    Serial.println("TMC2209 configurés en UART !");
    Serial.println("Arduino Mega 2560 prêt à recevoir du G-code !");
    
    homing();
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
    moteurX.setSpeed(-VITESSE_MAX / 2);
    moteurY1.setSpeed(-VITESSE_MAX / 2);
    moteurY2.setSpeed(-VITESSE_MAX / 2);

    while (digitalRead(X_MIN_PIN) == HIGH || digitalRead(Y_MIN_PIN) == HIGH) {
        if (digitalRead(X_MIN_PIN) == HIGH) moteurX.runSpeed();
        if (digitalRead(Y_MIN_PIN) == HIGH) {
            moteurY1.runSpeed();
            moteurY2.runSpeed();
        }
    }

    // Attendre un moment puis reculer légèrement
    delay(100);

    moteurX.setSpeed(VITESSE_MAX / 4);
    moteurY1.setSpeed(VITESSE_MAX / 4);
    moteurY2.setSpeed(VITESSE_MAX / 4);
    
    for (int i = 0; i < 10; i++) {
        moteurX.runSpeed();
        moteurY1.runSpeed();
        moteurY2.runSpeed();
    }

    // Définir la position actuelle comme origine
    moteurX.setCurrentPosition(0);
    moteurY1.setCurrentPosition(0);
    moteurY2.setCurrentPosition(0);

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

    moteurX.moveTo(stepsX);
    moteurY1.moveTo(stepsY);
    moteurY2.moveTo(stepsY);
    moteurRotationOutil.moveTo(stepsRotationOutil);
    moteurHauteurOutil.moveTo(stepsZ);

    while (moteurDeplacementX.distanceToGo() != 0 || moteurDeplacementY1.distanceToGo() != 0 || moteurDeplacementY2.distanceToGo() != 0 || moteurHauteurOutil.distanceToGo() != 0) {
        if (digitalRead(X_MIN_PIN) == LOW || digitalRead(X_MAX_PIN) == LOW || digitalRead(Y_MIN_PIN) == LOW || digitalRead(Y_MAX_PIN) == LOW) {
            Serial.println("Avertissement : Limite atteinte, arrêt des moteurs !");
            break;
        }
        moteurX.run();
        moteurY1.run();
        moteurY2.run();
        moteurRotationOutil.run();
        moteurHauteurOutil.run();
    }
}

// ======================= RÉCEPTION DU G-CODE AMÉLIORÉ =======================
void loop() {
    static bool firstCommand = true;
    if (Serial1.available()) {
        if (firstCommand) {
            Serial.println("Exécution du homing avant de commencer le G-code...");
            homing();
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

            moveXYZ(x, y);
        }
    }
}

/*
Le JK42HS40-1004A-02F est un moteur pas à pas, probablement de type NEMA 17, 
utilisé dans diverses applications de contrôle de mouvement, 
y compris les imprimantes 3D, les CNC et l'automatisation.

Voici ses caractéristiques principales :

Type : Moteur pas à pas bipolaire
Taille : NEMA 17
Angle de pas : 1,8° par pas (200 pas par tour)
Courant nominal : 1 A par phase
Résistance de phase : Environ 3,6 Ω
Inductance de phase : Environ 4,8 mH
Couple de maintien : Approximativement 40 N·cm (0,4 Nm)
Nombre de fils : 4 fils (configuration bipolaire)
Longueur du moteur : 40 mm
*/