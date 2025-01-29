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
#define STEP_Z  9
#define DIR_Z   10

#define SERIAL_PORT Serial2  // UART2 sur Mega2560
#define DRIVER_ADDRESS 0x00  // Adresse par défaut des TMC2209

#define X_MIN_PIN 8
#define X_MAX_PIN 11
#define Y_MIN_PIN 9
#define Y_MAX_PIN 12

#define X_MAX_LIMIT 500  // Limite maximale de déplacement en mm sur X
#define Y_MAX_LIMIT 500  // Limite maximale de déplacement en mm sur Y

AccelStepper moteurX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper moteurY1(AccelStepper::DRIVER, STEP_Y1, DIR_Y1);
AccelStepper moteurY2(AccelStepper::DRIVER, STEP_Y2, DIR_Y2);
AccelStepper moteurZ(AccelStepper::DRIVER, STEP_Z, DIR_Z);

TMC2209Stepper driverX(&SERIAL_PORT, DRIVER_ADDRESS);
TMC2209Stepper driverY1(&SERIAL_PORT, DRIVER_ADDRESS);
TMC2209Stepper driverY2(&SERIAL_PORT, DRIVER_ADDRESS);
TMC2209Stepper driverZ(&SERIAL_PORT, DRIVER_ADDRESS);

#define PAS_PAR_MM_X   80
#define PAS_PAR_MM_Y   100
#define VITESSE_MAX    3000
#define ACCELERATION   1500
#define PAS_PAR_DEGREE 10 // Conversion des degrés en pas pour l'axe Z

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    SERIAL_PORT.begin(115200);

    pinMode(X_MIN_PIN, INPUT_PULLUP);
    pinMode(X_MAX_PIN, INPUT_PULLUP);
    pinMode(Y_MIN_PIN, INPUT_PULLUP);
    pinMode(Y_MAX_PIN, INPUT_PULLUP);

    moteurX.setMaxSpeed(VITESSE_MAX);
    moteurX.setAcceleration(ACCELERATION);
    moteurY1.setMaxSpeed(VITESSE_MAX);
    moteurY1.setAcceleration(ACCELERATION);
    moteurY2.setMaxSpeed(VITESSE_MAX);
    moteurY2.setAcceleration(ACCELERATION);
    moteurZ.setMaxSpeed(VITESSE_MAX);
    moteurZ.setAcceleration(ACCELERATION);

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
    
    driverZ.begin();
    driverZ.toff(4);
    driverZ.rms_current(800);
    driverZ.microsteps(16);
    driverZ.pwm_autoscale(true);
    driverZ.en_pwm_mode(true);

    Serial.println("TMC2209 configurés en UART !");
    Serial.println("Arduino Mega 2560 prêt à recevoir du G-code !");
    
    homing();
}

// ======================= MOUVEMENT DES MOTEURS =======================
void moveXYZ(float x, float y) {
    // Vérifier les limites de la machine
    if (x < 0) x = 0;
    if (x > X_MAX_LIMIT) x = X_MAX_LIMIT;
    if (y < 0) y = 0;
    if (y > Y_MAX_LIMIT) y = Y_MAX_LIMIT;

    long stepsX = x * PAS_PAR_MM_X;
    long stepsY = y * PAS_PAR_MM_Y;
    float angle = atan2(y, x) * (180.0 / M_PI); // Calcul de l'angle en degrés
    long stepsZ = angle * PAS_PAR_DEGREE; // Conversion en pas moteur

    moteurX.moveTo(stepsX);
    moteurY1.moveTo(stepsY);
    moteurY2.moveTo(stepsY);
    moteurZ.moveTo(stepsZ);

    while (moteurX.distanceToGo() != 0 || moteurY1.distanceToGo() != 0 || moteurY2.distanceToGo() != 0 || moteurZ.distanceToGo() != 0) {
        if (digitalRead(X_MIN_PIN) == LOW || digitalRead(X_MAX_PIN) == LOW || digitalRead(Y_MIN_PIN) == LOW || digitalRead(Y_MAX_PIN) == LOW) {
            Serial.println("Avertissement : Limite atteinte, arrêt des moteurs !");
            break;
        }
        moteurX.run();
        moteurY1.run();
        moteurY2.run();
        moteurZ.run();
    }
}

// ======================= RÉCEPTION DU G-CODE AMÉLIORÉ =======================
void loop() {
    if (Serial1.available()) {
        String command = Serial1.readStringUntil('\n');
        command.trim();

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