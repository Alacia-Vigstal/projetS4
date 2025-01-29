#include <AccelStepper.h>
#include <TMCStepper.h>

// ======================= CONFIGURATION DES MOTEURS =======================
// Broches moteurs avec TMC2209 (Mode Step/Dir)
#define STEP_X 2
#define DIR_X 5
#define STEP_Y1 3
#define DIR_Y1 6
#define STEP_Y2 4
#define DIR_Y2 7

// Broches UART pour la configuration avancée des TMC2209
#define SERIAL_PORT Serial2  // UART2 sur Mega2560
#define DRIVER_ADDRESS 0x00  // Adresse par défaut des TMC2209

// Capteurs de fin de course (homing)
#define X_MIN_PIN 8
#define Y_MIN_PIN 9

// Définition des moteurs avec AccelStepper
AccelStepper moteurX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper moteurY1(AccelStepper::DRIVER, STEP_Y1, DIR_Y1);
AccelStepper moteurY2(AccelStepper::DRIVER, STEP_Y2, DIR_Y2);

// Drivers TMC2209
TMC2209Stepper driverX(&SERIAL_PORT, DRIVER_ADDRESS);
TMC2209Stepper driverY1(&SERIAL_PORT, DRIVER_ADDRESS);
TMC2209Stepper driverY2(&SERIAL_PORT, DRIVER_ADDRESS);

// ======================= PARAMÈTRES DES MOTEURS =======================
#define PAS_PAR_MM 80  // Ajuster selon la courroie et la mécanique
#define VITESSE_MAX 3000  // Vitesse max en pas/s
#define ACCELERATION 1500  // Accélération des moteurs

void setup() {
    Serial.begin(115200);  // Debug sur PC
    Serial1.begin(115200); // UART avec le Raspberry Pi
    SERIAL_PORT.begin(115200); // UART pour les TMC2209

    // Initialisation des capteurs de fin de course
    pinMode(X_MIN_PIN, INPUT_PULLUP);
    pinMode(Y_MIN_PIN, INPUT_PULLUP);

    // Configuration des moteurs
    moteurX.setMaxSpeed(VITESSE_MAX);
    moteurX.setAcceleration(ACCELERATION);
    moteurY1.setMaxSpeed(VITESSE_MAX);
    moteurY1.setAcceleration(ACCELERATION);
    moteurY2.setMaxSpeed(VITESSE_MAX);
    moteurY2.setAcceleration(ACCELERATION);

    // ======================= CONFIGURATION DES TMC2209 =======================
    driverX.begin();
    driverX.toff(4);
    driverX.rms_current(800);  // 800mA
    driverX.microsteps(16);
    driverX.pwm_autoscale(true);

    driverY1.begin();
    driverY1.toff(4);
    driverY1.rms_current(800);
    driverY1.microsteps(16);
    driverY1.pwm_autoscale(true);

    driverY2.begin();
    driverY2.toff(4);
    driverY2.rms_current(800);
    driverY2.microsteps(16);
    driverY2.pwm_autoscale(true);

    Serial.println("TMC2209 configurés en UART !");
    Serial.println("Arduino Mega 2560 prêt à recevoir du G-code !");
    
    // Faire le homing (calibrage) au démarrage
    homing();
}

// ======================= HOMING (CALIBRAGE) =======================
void homing() {
    Serial.println("Début du homing...");

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

    moteurX.setCurrentPosition(0);
    moteurY1.setCurrentPosition(0);
    moteurY2.setCurrentPosition(0);

    Serial.println("Homing terminé !");
}

// ======================= MOUVEMENT DES MOTEURS =======================
void moveXY(float x, float y) {
    long stepsX = x * PAS_PAR_MM;
    long stepsY = y * PAS_PAR_MM;

    moteurX.moveTo(stepsX);
    moteurY1.moveTo(stepsY);
    moteurY2.moveTo(stepsY);

    while (moteurX.distanceToGo() != 0 || moteurY1.distanceToGo() != 0 || moteurY2.distanceToGo() != 0) {
        moteurX.run();
        moteurY1.run();
        moteurY2.run();
    }
}

// ======================= RÉCEPTION DU G-CODE =======================
void loop() {
    if (Serial1.available()) {  // Vérifie si une commande G-code arrive du Raspberry
        String command = Serial1.readStringUntil('\n');
        command.trim();

        if (command.startsWith("G0") || command.startsWith("G1")) {
            float x = 0, y = 0;
            int xIndex = command.indexOf("X");
            int yIndex = command.indexOf("Y");

            if (xIndex != -1) x = command.substring(xIndex + 1).toFloat();
            if (yIndex != -1) y = command.substring(yIndex + 1).toFloat();

            Serial.print("Commande reçue : X=");
            Serial.print(x);
            Serial.print(" Y=");
            Serial.println(y);

            moveXY(x, y);
        }
    }
}