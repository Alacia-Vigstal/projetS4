#include <Arduino.h>
#include <AccelStepper.h>
#include <vector>
#include "circle_gcode.hpp" // Inclure le fichier de génération du G-code

// Déclaration du buffer G-code
std::vector<String> gcodeBuffer;
int gcodeIndex = 0;

// ======================= CONFIGURATION DES LIMIT SWITCH =================
#define LIMIT_X_MIN  4
#define LIMIT_X_MAX  16
#define LIMIT_Y_MIN_L  17
#define LIMIT_Y_MIN_R  23
#define LIMIT_Y_MAX_L  25
#define LIMIT_Y_MAX_R  14
#define LIMIT_Z_MAX  13

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
    "G0 X50.0 Y0.0 Z0 ZRot0.0",
    "G0 X49.996 Y0.628 Z0 ZRot0.013",
    "G0 X49.984 Y1.257 Z0 ZRot0.025",
    "G0 X49.964 Y1.885 Z0 ZRot0.038",
    "G0 X49.937 Y2.512 Z0 ZRot0.05",
    "G0 X49.901 Y3.14 Z0 ZRot0.063",
    "G0 X49.858 Y3.766 Z0 ZRot0.075",
    "G0 X49.807 Y4.393 Z0 ZRot0.088",
    "G0 X49.748 Y5.018 Z0 ZRot0.101",
    "G0 X49.681 Y5.643 Z0 ZRot0.113",
    "G0 X49.606 Y6.267 Z0 ZRot0.126",
    "G0 X49.523 Y6.89 Z0 ZRot0.138",
    "G0 X49.433 Y7.511 Z0 ZRot0.151",
    "G0 X49.334 Y8.132 Z0 ZRot0.163",
    "G0 X49.228 Y8.751 Z0 ZRot0.176",
    "G0 X49.114 Y9.369 Z0 ZRot0.188",
    "G0 X48.993 Y9.985 Z0 ZRot0.201",
    "G0 X48.863 Y10.6 Z0 ZRot0.214",
    "G0 X48.726 Y11.214 Z0 ZRot0.226",
    "G0 X48.582 Y11.825 Z0 ZRot0.239",
    "G0 X48.429 Y12.434 Z0 ZRot0.251",
    "G0 X48.269 Y13.042 Z0 ZRot0.264",
    "G0 X48.101 Y13.648 Z0 ZRot0.276",
    "G0 X47.926 Y14.251 Z0 ZRot0.289",
    "G0 X47.743 Y14.852 Z0 ZRot0.302",
    "G0 X47.553 Y15.451 Z0 ZRot0.314",
    "G0 X47.355 Y16.047 Z0 ZRot0.327",
    "G0 X47.15 Y16.641 Z0 ZRot0.339",
    "G0 X46.937 Y17.232 Z0 ZRot0.352",
    "G0 X46.716 Y17.821 Z0 ZRot0.364",
    "G0 X46.489 Y18.406 Z0 ZRot0.377",
    "G0 X46.254 Y18.989 Z0 ZRot0.39",
    "G0 X46.012 Y19.569 Z0 ZRot0.402",
    "G0 X45.762 Y20.145 Z0 ZRot0.415",
    "G0 X45.505 Y20.719 Z0 ZRot0.427",
    "G0 X45.241 Y21.289 Z0 ZRot0.44",
    "G0 X44.97 Y21.856 Z0 ZRot0.452",
    "G0 X44.692 Y22.419 Z0 ZRot0.465",
    "G0 X44.407 Y22.979 Z0 ZRot0.478",
    "G0 X44.115 Y23.535 Z0 ZRot0.49",
    "G0 X43.815 Y24.088 Z0 ZRot0.503",
    "G0 X43.509 Y24.636 Z0 ZRot0.515",
    "G0 X43.196 Y25.181 Z0 ZRot0.528",
    "G0 X42.876 Y25.722 Z0 ZRot0.54",
    "G0 X42.55 Y26.259 Z0 ZRot0.553",
    "G0 X42.216 Y26.791 Z0 ZRot0.565",
    "G0 X41.876 Y27.32 Z0 ZRot0.578",
    "G0 X41.53 Y27.844 Z0 ZRot0.591",
    "G0 X41.177 Y28.363 Z0 ZRot0.603",
    "G0 X40.817 Y28.879 Z0 ZRot0.616",
    "G0 X40.451 Y29.389 Z0 ZRot0.628",
    "G0 X40.078 Y29.895 Z0 ZRot0.641",
    "G0 X39.7 Y30.397 Z0 ZRot0.653",
    "G0 X39.314 Y30.893 Z0 ZRot0.666",
    "G0 X38.923 Y31.385 Z0 ZRot0.679",
    "G0 X38.526 Y31.871 Z0 ZRot0.691",
    "G0 X38.122 Y32.353 Z0 ZRot0.704",
    "G0 X37.713 Y32.829 Z0 ZRot0.716",
    "G0 X37.297 Y33.301 Z0 ZRot0.729",
    "G0 X36.876 Y33.767 Z0 ZRot0.741",
    "G0 X36.448 Y34.227 Z0 ZRot0.754",
    "G0 X36.015 Y34.683 Z0 ZRot0.767",
    "G0 X35.577 Y35.132 Z0 ZRot0.779",
    "G0 X35.132 Y35.577 Z0 ZRot0.792",
    "G0 X34.683 Y36.015 Z0 ZRot0.804",
    "G0 X34.227 Y36.448 Z0 ZRot0.817",
    "G0 X33.767 Y36.876 Z0 ZRot0.829",
    "G0 X33.301 Y37.297 Z0 ZRot0.842",
    "G0 X32.829 Y37.713 Z0 ZRot0.855",
    "G0 X32.353 Y38.122 Z0 ZRot0.867",
    "G0 X31.871 Y38.526 Z0 ZRot0.88",
    "G0 X31.385 Y38.923 Z0 ZRot0.892",
    "G0 X30.893 Y39.314 Z0 ZRot0.905",
    "G0 X30.397 Y39.7 Z0 ZRot0.917",
    "G0 X29.895 Y40.078 Z0 ZRot0.93",
    "G0 X29.389 Y40.451 Z0 ZRot0.942",
    "G0 X28.879 Y40.817 Z0 ZRot0.955",
    "G0 X28.363 Y41.177 Z0 ZRot0.968",
    "G0 X27.844 Y41.53 Z0 ZRot0.98",
    "G0 X27.32 Y41.876 Z0 ZRot0.993",
    "G0 X26.791 Y42.216 Z0 ZRot1.005",
    "G0 X26.259 Y42.55 Z0 ZRot1.018",
    "G0 X25.722 Y42.876 Z0 ZRot1.03",
    "G0 X25.181 Y43.196 Z0 ZRot1.043",
    "G0 X24.636 Y43.509 Z0 ZRot1.056",
    "G0 X24.088 Y43.815 Z0 ZRot1.068",
    "G0 X23.535 Y44.115 Z0 ZRot1.081",
    "G0 X22.979 Y44.407 Z0 ZRot1.093",
    "G0 X22.419 Y44.692 Z0 ZRot1.106",
    "G0 X21.856 Y44.97 Z0 ZRot1.118",
    "G0 X21.289 Y45.241 Z0 ZRot1.131",
    "G0 X20.719 Y45.505 Z0 ZRot1.144",
    "G0 X20.145 Y45.762 Z0 ZRot1.156",
    "G0 X19.569 Y46.012 Z0 ZRot1.169",
    "G0 X18.989 Y46.254 Z0 ZRot1.181",
    "G0 X18.406 Y46.489 Z0 ZRot1.194",
    "G0 X17.821 Y46.716 Z0 ZRot1.206",
    "G0 X17.232 Y46.937 Z0 ZRot1.219",
    "G0 X16.641 Y47.15 Z0 ZRot1.232",
    "G0 X16.047 Y47.355 Z0 ZRot1.244",
    "G0 X15.451 Y47.553 Z0 ZRot1.257",
    "G0 X14.852 Y47.743 Z0 ZRot1.269",
    "G0 X14.251 Y47.926 Z0 ZRot1.282",
    "G0 X13.648 Y48.101 Z0 ZRot1.294",
    "G0 X13.042 Y48.269 Z0 ZRot1.307",
    "G0 X12.434 Y48.429 Z0 ZRot1.319",
    "G0 X11.825 Y48.582 Z0 ZRot1.332",
    "G0 X11.214 Y48.726 Z0 ZRot1.345",
    "G0 X10.6 Y48.863 Z0 ZRot1.357",
    "G0 X9.985 Y48.993 Z0 ZRot1.37",
    "G0 X9.369 Y49.114 Z0 ZRot1.382",
    "G0 X8.751 Y49.228 Z0 ZRot1.395",
    "G0 X8.132 Y49.334 Z0 ZRot1.407",
    "G0 X7.511 Y49.433 Z0 ZRot1.42",
    "G0 X6.89 Y49.523 Z0 ZRot1.433",
    "G0 X6.267 Y49.606 Z0 ZRot1.445",
    "G0 X5.643 Y49.681 Z0 ZRot1.458",
    "G0 X5.018 Y49.748 Z0 ZRot1.47",
    "G0 X4.393 Y49.807 Z0 ZRot1.483",
    "G0 X3.766 Y49.858 Z0 ZRot1.495",
    "G0 X3.14 Y49.901 Z0 ZRot1.508",
    "G0 X2.512 Y49.937 Z0 ZRot1.521",
    "G0 X1.885 Y49.964 Z0 ZRot1.533",
    "G0 X1.257 Y49.984 Z0 ZRot1.546",
    "G0 X0.628 Y49.996 Z0 ZRot1.558",
    "G0 X0.0 Y50.0 Z0 ZRot1.571",
    "G0 X-0.628 Y49.996 Z0 ZRot1.583",
    "G0 X-1.257 Y49.984 Z0 ZRot1.596",
    "G0 X-1.885 Y49.964 Z0 ZRot1.608",
    "G0 X-2.512 Y49.937 Z0 ZRot1.621",
    "G0 X-3.14 Y49.901 Z0 ZRot1.634",
    "G0 X-3.766 Y49.858 Z0 ZRot1.646",
    "G0 X-4.393 Y49.807 Z0 ZRot1.659",
    "G0 X-5.018 Y49.748 Z0 ZRot1.671",
    "G0 X-5.643 Y49.681 Z0 ZRot1.684",
    "G0 X-6.267 Y49.606 Z0 ZRot1.696",
    "G0 X-6.89 Y49.523 Z0 ZRot1.709",
    "G0 X-7.511 Y49.433 Z0 ZRot1.722",
    "G0 X-8.132 Y49.334 Z0 ZRot1.734",
    "G0 X-8.751 Y49.228 Z0 ZRot1.747",
    "G0 X-9.369 Y49.114 Z0 ZRot1.759",
    "G0 X-9.985 Y48.993 Z0 ZRot1.772",
    "G0 X-10.6 Y48.863 Z0 ZRot1.784",
    "G0 X-11.214 Y48.726 Z0 ZRot1.797",
    "G0 X-11.825 Y48.582 Z0 ZRot1.81",
    "G0 X-12.434 Y48.429 Z0 ZRot1.822",
    "G0 X-13.042 Y48.269 Z0 ZRot1.835",
    "G0 X-13.648 Y48.101 Z0 ZRot1.847",
    "G0 X-14.251 Y47.926 Z0 ZRot1.86",
    "G0 X-14.852 Y47.743 Z0 ZRot1.872",
    "G0 X-15.451 Y47.553 Z0 ZRot1.885",
    "G0 X-16.047 Y47.355 Z0 ZRot1.898",
    "G0 X-16.641 Y47.15 Z0 ZRot1.91",
    "G0 X-17.232 Y46.937 Z0 ZRot1.923",
    "G0 X-17.821 Y46.716 Z0 ZRot1.935",
    "G0 X-18.406 Y46.489 Z0 ZRot1.948",
    "G0 X-18.989 Y46.254 Z0 ZRot1.96",
    "G0 X-19.569 Y46.012 Z0 ZRot1.973",
    "G0 X-20.145 Y45.762 Z0 ZRot1.985",
    "G0 X-20.719 Y45.505 Z0 ZRot1.998",
    "G0 X-21.289 Y45.241 Z0 ZRot2.011",
    "G0 X-21.856 Y44.97 Z0 ZRot2.023",
    "G0 X-22.419 Y44.692 Z0 ZRot2.036",
    "G0 X-22.979 Y44.407 Z0 ZRot2.048",
    "G0 X-23.535 Y44.115 Z0 ZRot2.061",
    "G0 X-24.088 Y43.815 Z0 ZRot2.073",
    "G0 X-24.636 Y43.509 Z0 ZRot2.086",
    "G0 X-25.181 Y43.196 Z0 ZRot2.099",
    "G0 X-25.722 Y42.876 Z0 ZRot2.111",
    "G0 X-26.259 Y42.55 Z0 ZRot2.124",
    "G0 X-26.791 Y42.216 Z0 ZRot2.136",
    "G0 X-27.32 Y41.876 Z0 ZRot2.149",
    "G0 X-27.844 Y41.53 Z0 ZRot2.161",
    "G0 X-28.363 Y41.177 Z0 ZRot2.174",
    "G0 X-28.879 Y40.817 Z0 ZRot2.187",
    "G0 X-29.389 Y40.451 Z0 ZRot2.199",
    "G0 X-29.895 Y40.078 Z0 ZRot2.212",
    "G0 X-30.397 Y39.7 Z0 ZRot2.224",
    "G0 X-30.893 Y39.314 Z0 ZRot2.237",
    "G0 X-31.385 Y38.923 Z0 ZRot2.249",
    "G0 X-31.871 Y38.526 Z0 ZRot2.262",
    "G0 X-32.353 Y38.122 Z0 ZRot2.275",
    "G0 X-32.829 Y37.713 Z0 ZRot2.287",
    "G0 X-33.301 Y37.297 Z0 ZRot2.3",
    "G0 X-33.767 Y36.876 Z0 ZRot2.312",
    "G0 X-34.227 Y36.448 Z0 ZRot2.325",
    "G0 X-34.683 Y36.015 Z0 ZRot2.337",
    "G0 X-35.132 Y35.577 Z0 ZRot2.35",
    "G0 X-35.577 Y35.132 Z0 ZRot2.362",
    "G0 X-36.015 Y34.683 Z0 ZRot2.375",
    "G0 X-36.448 Y34.227 Z0 ZRot2.388",
    "G0 X-36.876 Y33.767 Z0 ZRot2.4",
    "G0 X-37.297 Y33.301 Z0 ZRot2.413",
    "G0 X-37.713 Y32.829 Z0 ZRot2.425",
    "G0 X-38.122 Y32.353 Z0 ZRot2.438",
    "G0 X-38.526 Y31.871 Z0 ZRot2.45",
    "G0 X-38.923 Y31.385 Z0 ZRot2.463",
    "G0 X-39.314 Y30.893 Z0 ZRot2.476",
    "G0 X-39.7 Y30.397 Z0 ZRot2.488",
    "G0 X-40.078 Y29.895 Z0 ZRot2.501",
    "G0 X-40.451 Y29.389 Z0 ZRot2.513",
    "G0 X-40.817 Y28.879 Z0 ZRot2.526",
    "G0 X-41.177 Y28.363 Z0 ZRot2.538",
    "G0 X-41.53 Y27.844 Z0 ZRot2.551",
    "G0 X-41.876 Y27.32 Z0 ZRot2.564",
    "G0 X-42.216 Y26.791 Z0 ZRot2.576",
    "G0 X-42.55 Y26.259 Z0 ZRot2.589",
    "G0 X-42.876 Y25.722 Z0 ZRot2.601",
    "G0 X-43.196 Y25.181 Z0 ZRot2.614",
    "G0 X-43.509 Y24.636 Z0 ZRot2.626",
    "G0 X-43.815 Y24.088 Z0 ZRot2.639",
    "G0 X-44.115 Y23.535 Z0 ZRot2.652",
    "G0 X-44.407 Y22.979 Z0 ZRot2.664",
    "G0 X-44.692 Y22.419 Z0 ZRot2.677",
    "G0 X-44.97 Y21.856 Z0 ZRot2.689",
    "G0 X-45.241 Y21.289 Z0 ZRot2.702",
    "G0 X-45.505 Y20.719 Z0 ZRot2.714",
    "G0 X-45.762 Y20.145 Z0 ZRot2.727",
    "G0 X-46.012 Y19.569 Z0 ZRot2.739",
    "G0 X-46.254 Y18.989 Z0 ZRot2.752",
    "G0 X-46.489 Y18.406 Z0 ZRot2.765",
    "G0 X-46.716 Y17.821 Z0 ZRot2.777",
    "G0 X-46.937 Y17.232 Z0 ZRot2.79",
    "G0 X-47.15 Y16.641 Z0 ZRot2.802",
    "G0 X-47.355 Y16.047 Z0 ZRot2.815",
    "G0 X-47.553 Y15.451 Z0 ZRot2.827",
    "G0 X-47.743 Y14.852 Z0 ZRot2.84",
    "G0 X-47.926 Y14.251 Z0 ZRot2.853",
    "G0 X-48.101 Y13.648 Z0 ZRot2.865",
    "G0 X-48.269 Y13.042 Z0 ZRot2.878",
    "G0 X-48.429 Y12.434 Z0 ZRot2.89",
    "G0 X-48.582 Y11.825 Z0 ZRot2.903",
    "G0 X-48.726 Y11.214 Z0 ZRot2.915",
    "G0 X-48.863 Y10.6 Z0 ZRot2.928",
    "G0 X-48.993 Y9.985 Z0 ZRot2.941",
    "G0 X-49.114 Y9.369 Z0 ZRot2.953",
    "G0 X-49.228 Y8.751 Z0 ZRot2.966",
    "G0 X-49.334 Y8.132 Z0 ZRot2.978",
    "G0 X-49.433 Y7.511 Z0 ZRot2.991",
    "G0 X-49.523 Y6.89 Z0 ZRot3.003",
    "G0 X-49.606 Y6.267 Z0 ZRot3.016",
    "G0 X-49.681 Y5.643 Z0 ZRot3.028",
    "G0 X-49.748 Y5.018 Z0 ZRot3.041",
    "G0 X-49.807 Y4.393 Z0 ZRot3.054",
    "G0 X-49.858 Y3.766 Z0 ZRot3.066",
    "G0 X-49.901 Y3.14 Z0 ZRot3.079",
    "G0 X-49.937 Y2.512 Z0 ZRot3.091",
    "G0 X-49.964 Y1.885 Z0 ZRot3.104",
    "G0 X-49.984 Y1.257 Z0 ZRot3.116",
    "G0 X-49.996 Y0.628 Z0 ZRot3.129",
    "G0 X-50.0 Y0.0 Z0 ZRot3.142",
    "G0 X-49.996 Y-0.628 Z0 ZRot3.154",
    "G0 X-49.984 Y-1.257 Z0 ZRot3.167",
    "G0 X-49.964 Y-1.885 Z0 ZRot3.179",
    "G0 X-49.937 Y-2.512 Z0 ZRot3.192",
    "G0 X-49.901 Y-3.14 Z0 ZRot3.204",
    "G0 X-49.858 Y-3.766 Z0 ZRot3.217",
    "G0 X-49.807 Y-4.393 Z0 ZRot3.23",
    "G0 X-49.748 Y-5.018 Z0 ZRot3.242",
    "G0 X-49.681 Y-5.643 Z0 ZRot3.255",
    "G0 X-49.606 Y-6.267 Z0 ZRot3.267",
    "G0 X-49.523 Y-6.89 Z0 ZRot3.28",
    "G0 X-49.433 Y-7.511 Z0 ZRot3.292",
    "G0 X-49.334 Y-8.132 Z0 ZRot3.305",
    "G0 X-49.228 Y-8.751 Z0 ZRot3.318",
    "G0 X-49.114 Y-9.369 Z0 ZRot3.33",
    "G0 X-48.993 Y-9.985 Z0 ZRot3.343",
    "G0 X-48.863 Y-10.6 Z0 ZRot3.355",
    "G0 X-48.726 Y-11.214 Z0 ZRot3.368",
    "G0 X-48.582 Y-11.825 Z0 ZRot3.38",
    "G0 X-48.429 Y-12.434 Z0 ZRot3.393",
    "G0 X-48.269 Y-13.042 Z0 ZRot3.405",
    "G0 X-48.101 Y-13.648 Z0 ZRot3.418",
    "G0 X-47.926 Y-14.251 Z0 ZRot3.431",
    "G0 X-47.743 Y-14.852 Z0 ZRot3.443",
    "G0 X-47.553 Y-15.451 Z0 ZRot3.456",
    "G0 X-47.355 Y-16.047 Z0 ZRot3.468",
    "G0 X-47.15 Y-16.641 Z0 ZRot3.481",
    "G0 X-46.937 Y-17.232 Z0 ZRot3.493",
    "G0 X-46.716 Y-17.821 Z0 ZRot3.506",
    "G0 X-46.489 Y-18.406 Z0 ZRot3.519",
    "G0 X-46.254 Y-18.989 Z0 ZRot3.531",
    "G0 X-46.012 Y-19.569 Z0 ZRot3.544",
    "G0 X-45.762 Y-20.145 Z0 ZRot3.556",
    "G0 X-45.505 Y-20.719 Z0 ZRot3.569",
    "G0 X-45.241 Y-21.289 Z0 ZRot3.581",
    "G0 X-44.97 Y-21.856 Z0 ZRot3.594",
    "G0 X-44.692 Y-22.419 Z0 ZRot3.607",
    "G0 X-44.407 Y-22.979 Z0 ZRot3.619",
    "G0 X-44.115 Y-23.535 Z0 ZRot3.632",
    "G0 X-43.815 Y-24.088 Z0 ZRot3.644",
    "G0 X-43.509 Y-24.636 Z0 ZRot3.657",
    "G0 X-43.196 Y-25.181 Z0 ZRot3.669",
    "G0 X-42.876 Y-25.722 Z0 ZRot3.682",
    "G0 X-42.55 Y-26.259 Z0 ZRot3.695",
    "G0 X-42.216 Y-26.791 Z0 ZRot3.707",
    "G0 X-41.876 Y-27.32 Z0 ZRot3.72",
    "G0 X-41.53 Y-27.844 Z0 ZRot3.732",
    "G0 X-41.177 Y-28.363 Z0 ZRot3.745",
    "G0 X-40.817 Y-28.879 Z0 ZRot3.757",
    "G0 X-40.451 Y-29.389 Z0 ZRot3.77",
    "G0 X-40.078 Y-29.895 Z0 ZRot3.782",
    "G0 X-39.7 Y-30.397 Z0 ZRot3.795",
    "G0 X-39.314 Y-30.893 Z0 ZRot3.808",
    "G0 X-38.923 Y-31.385 Z0 ZRot3.82",
    "G0 X-38.526 Y-31.871 Z0 ZRot3.833",
    "G0 X-38.122 Y-32.353 Z0 ZRot3.845",
    "G0 X-37.713 Y-32.829 Z0 ZRot3.858",
    "G0 X-37.297 Y-33.301 Z0 ZRot3.87",
    "G0 X-36.876 Y-33.767 Z0 ZRot3.883",
    "G0 X-36.448 Y-34.227 Z0 ZRot3.896",
    "G0 X-36.015 Y-34.683 Z0 ZRot3.908",
    "G0 X-35.577 Y-35.132 Z0 ZRot3.921",
    "G0 X-35.132 Y-35.577 Z0 ZRot3.933",
    "G0 X-34.683 Y-36.015 Z0 ZRot3.946",
    "G0 X-34.227 Y-36.448 Z0 ZRot3.958",
    "G0 X-33.767 Y-36.876 Z0 ZRot3.971",
    "G0 X-33.301 Y-37.297 Z0 ZRot3.984",
    "G0 X-32.829 Y-37.713 Z0 ZRot3.996",
    "G0 X-32.353 Y-38.122 Z0 ZRot4.009",
    "G0 X-31.871 Y-38.526 Z0 ZRot4.021",
    "G0 X-31.385 Y-38.923 Z0 ZRot4.034",
    "G0 X-30.893 Y-39.314 Z0 ZRot4.046",
    "G0 X-30.397 Y-39.7 Z0 ZRot4.059",
    "G0 X-29.895 Y-40.078 Z0 ZRot4.072",
    "G0 X-29.389 Y-40.451 Z0 ZRot4.084",
    "G0 X-28.879 Y-40.817 Z0 ZRot4.097",
    "G0 X-28.363 Y-41.177 Z0 ZRot4.109",
    "G0 X-27.844 Y-41.53 Z0 ZRot4.122",
    "G0 X-27.32 Y-41.876 Z0 ZRot4.134",
    "G0 X-26.791 Y-42.216 Z0 ZRot4.147",
    "G0 X-26.259 Y-42.55 Z0 ZRot4.159",
    "G0 X-25.722 Y-42.876 Z0 ZRot4.172",
    "G0 X-25.181 Y-43.196 Z0 ZRot4.185",
    "G0 X-24.636 Y-43.509 Z0 ZRot4.197",
    "G0 X-24.088 Y-43.815 Z0 ZRot4.21",
    "G0 X-23.535 Y-44.115 Z0 ZRot4.222",
    "G0 X-22.979 Y-44.407 Z0 ZRot4.235",
    "G0 X-22.419 Y-44.692 Z0 ZRot4.247",
    "G0 X-21.856 Y-44.97 Z0 ZRot4.26",
    "G0 X-21.289 Y-45.241 Z0 ZRot4.273",
    "G0 X-20.719 Y-45.505 Z0 ZRot4.285",
    "G0 X-20.145 Y-45.762 Z0 ZRot4.298",
    "G0 X-19.569 Y-46.012 Z0 ZRot4.31",
    "G0 X-18.989 Y-46.254 Z0 ZRot4.323",
    "G0 X-18.406 Y-46.489 Z0 ZRot4.335",
    "G0 X-17.821 Y-46.716 Z0 ZRot4.348",
    "G0 X-17.232 Y-46.937 Z0 ZRot4.361",
    "G0 X-16.641 Y-47.15 Z0 ZRot4.373",
    "G0 X-16.047 Y-47.355 Z0 ZRot4.386",
    "G0 X-15.451 Y-47.553 Z0 ZRot4.398",
    "G0 X-14.852 Y-47.743 Z0 ZRot4.411",
    "G0 X-14.251 Y-47.926 Z0 ZRot4.423",
    "G0 X-13.648 Y-48.101 Z0 ZRot4.436",
    "G0 X-13.042 Y-48.269 Z0 ZRot4.448",
    "G0 X-12.434 Y-48.429 Z0 ZRot4.461",
    "G0 X-11.825 Y-48.582 Z0 ZRot4.474",
    "G0 X-11.214 Y-48.726 Z0 ZRot4.486",
    "G0 X-10.6 Y-48.863 Z0 ZRot4.499",
    "G0 X-9.985 Y-48.993 Z0 ZRot4.511",
    "G0 X-9.369 Y-49.114 Z0 ZRot4.524",
    "G0 X-8.751 Y-49.228 Z0 ZRot4.536",
    "G0 X-8.132 Y-49.334 Z0 ZRot4.549",
    "G0 X-7.511 Y-49.433 Z0 ZRot4.562",
    "G0 X-6.89 Y-49.523 Z0 ZRot4.574",
    "G0 X-6.267 Y-49.606 Z0 ZRot4.587",
    "G0 X-5.643 Y-49.681 Z0 ZRot4.599",
    "G0 X-5.018 Y-49.748 Z0 ZRot4.612",
    "G0 X-4.393 Y-49.807 Z0 ZRot4.624",
    "G0 X-3.766 Y-49.858 Z0 ZRot4.637",
    "G0 X-3.14 Y-49.901 Z0 ZRot4.65",
    "G0 X-2.512 Y-49.937 Z0 ZRot4.662",
    "G0 X-1.885 Y-49.964 Z0 ZRot4.675",
    "G0 X-1.257 Y-49.984 Z0 ZRot4.687",
    "G0 X-0.628 Y-49.996 Z0 ZRot4.7",
    "G0 X-0.0 Y-50.0 Z0 ZRot4.712",
    "G0 X0.628 Y-49.996 Z0 ZRot4.725",
    "G0 X1.257 Y-49.984 Z0 ZRot4.738",
    "G0 X1.885 Y-49.964 Z0 ZRot4.75",
    "G0 X2.512 Y-49.937 Z0 ZRot4.763",
    "G0 X3.14 Y-49.901 Z0 ZRot4.775",
    "G0 X3.766 Y-49.858 Z0 ZRot4.788",
    "G0 X4.393 Y-49.807 Z0 ZRot4.8",
    "G0 X5.018 Y-49.748 Z0 ZRot4.813",
    "G0 X5.643 Y-49.681 Z0 ZRot4.825",
    "G0 X6.267 Y-49.606 Z0 ZRot4.838",
    "G0 X6.89 Y-49.523 Z0 ZRot4.851",
    "G0 X7.511 Y-49.433 Z0 ZRot4.863",
    "G0 X8.132 Y-49.334 Z0 ZRot4.876",
    "G0 X8.751 Y-49.228 Z0 ZRot4.888",
    "G0 X9.369 Y-49.114 Z0 ZRot4.901",
    "G0 X9.985 Y-48.993 Z0 ZRot4.913",
    "G0 X10.6 Y-48.863 Z0 ZRot4.926",
    "G0 X11.214 Y-48.726 Z0 ZRot4.939",
    "G0 X11.825 Y-48.582 Z0 ZRot4.951",
    "G0 X12.434 Y-48.429 Z0 ZRot4.964",
    "G0 X13.042 Y-48.269 Z0 ZRot4.976",
    "G0 X13.648 Y-48.101 Z0 ZRot4.989",
    "G0 X14.251 Y-47.926 Z0 ZRot5.001",
    "G0 X14.852 Y-47.743 Z0 ZRot5.014",
    "G0 X15.451 Y-47.553 Z0 ZRot5.027",
    "G0 X16.047 Y-47.355 Z0 ZRot5.039",
    "G0 X16.641 Y-47.15 Z0 ZRot5.052",
    "G0 X17.232 Y-46.937 Z0 ZRot5.064",
    "G0 X17.821 Y-46.716 Z0 ZRot5.077",
    "G0 X18.406 Y-46.489 Z0 ZRot5.089",
    "G0 X18.989 Y-46.254 Z0 ZRot5.102",
    "G0 X19.569 Y-46.012 Z0 ZRot5.115",
    "G0 X20.145 Y-45.762 Z0 ZRot5.127",
    "G0 X20.719 Y-45.505 Z0 ZRot5.14",
    "G0 X21.289 Y-45.241 Z0 ZRot5.152",
    "G0 X21.856 Y-44.97 Z0 ZRot5.165",
    "G0 X22.419 Y-44.692 Z0 ZRot5.177",
    "G0 X22.979 Y-44.407 Z0 ZRot5.19",
    "G0 X23.535 Y-44.115 Z0 ZRot5.202",
    "G0 X24.088 Y-43.815 Z0 ZRot5.215",
    "G0 X24.636 Y-43.509 Z0 ZRot5.228",
    "G0 X25.181 Y-43.196 Z0 ZRot5.24",
    "G0 X25.722 Y-42.876 Z0 ZRot5.253",
    "G0 X26.259 Y-42.55 Z0 ZRot5.265",
    "G0 X26.791 Y-42.216 Z0 ZRot5.278",
    "G0 X27.32 Y-41.876 Z0 ZRot5.29",
    "G0 X27.844 Y-41.53 Z0 ZRot5.303",
    "G0 X28.363 Y-41.177 Z0 ZRot5.316",
    "G0 X28.879 Y-40.817 Z0 ZRot5.328",
    "G0 X29.389 Y-40.451 Z0 ZRot5.341",
    "G0 X29.895 Y-40.078 Z0 ZRot5.353",
    "G0 X30.397 Y-39.7 Z0 ZRot5.366",
    "G0 X30.893 Y-39.314 Z0 ZRot5.378",
    "G0 X31.385 Y-38.923 Z0 ZRot5.391",
    "G0 X31.871 Y-38.526 Z0 ZRot5.404",
    "G0 X32.353 Y-38.122 Z0 ZRot5.416",
    "G0 X32.829 Y-37.713 Z0 ZRot5.429",
    "G0 X33.301 Y-37.297 Z0 ZRot5.441",
    "G0 X33.767 Y-36.876 Z0 ZRot5.454",
    "G0 X34.227 Y-36.448 Z0 ZRot5.466",
    "G0 X34.683 Y-36.015 Z0 ZRot5.479",
    "G0 X35.132 Y-35.577 Z0 ZRot5.492",
    "G0 X35.577 Y-35.132 Z0 ZRot5.504",
    "G0 X36.015 Y-34.683 Z0 ZRot5.517",
    "G0 X36.448 Y-34.227 Z0 ZRot5.529",
    "G0 X36.876 Y-33.767 Z0 ZRot5.542",
    "G0 X37.297 Y-33.301 Z0 ZRot5.554",
    "G0 X37.713 Y-32.829 Z0 ZRot5.567",
    "G0 X38.122 Y-32.353 Z0 ZRot5.579",
    "G0 X38.526 Y-31.871 Z0 ZRot5.592",
    "G0 X38.923 Y-31.385 Z0 ZRot5.605",
    "G0 X39.314 Y-30.893 Z0 ZRot5.617",
    "G0 X39.7 Y-30.397 Z0 ZRot5.63",
    "G0 X40.078 Y-29.895 Z0 ZRot5.642",
    "G0 X40.451 Y-29.389 Z0 ZRot5.655",
    "G0 X40.817 Y-28.879 Z0 ZRot5.667",
    "G0 X41.177 Y-28.363 Z0 ZRot5.68",
    "G0 X41.53 Y-27.844 Z0 ZRot5.693",
    "G0 X41.876 Y-27.32 Z0 ZRot5.705",
    "G0 X42.216 Y-26.791 Z0 ZRot5.718",
    "G0 X42.55 Y-26.259 Z0 ZRot5.73",
    "G0 X42.876 Y-25.722 Z0 ZRot5.743",
    "G0 X43.196 Y-25.181 Z0 ZRot5.755",
    "G0 X43.509 Y-24.636 Z0 ZRot5.768",
    "G0 X43.815 Y-24.088 Z0 ZRot5.781",
    "G0 X44.115 Y-23.535 Z0 ZRot5.793",
    "G0 X44.407 Y-22.979 Z0 ZRot5.806",
    "G0 X44.692 Y-22.419 Z0 ZRot5.818",
    "G0 X44.97 Y-21.856 Z0 ZRot5.831",
    "G0 X45.241 Y-21.289 Z0 ZRot5.843",
    "G0 X45.505 Y-20.719 Z0 ZRot5.856",
    "G0 X45.762 Y-20.145 Z0 ZRot5.868",
    "G0 X46.012 Y-19.569 Z0 ZRot5.881",
    "G0 X46.254 Y-18.989 Z0 ZRot5.894",
    "G0 X46.489 Y-18.406 Z0 ZRot5.906",
    "G0 X46.716 Y-17.821 Z0 ZRot5.919",
    "G0 X46.937 Y-17.232 Z0 ZRot5.931",
    "G0 X47.15 Y-16.641 Z0 ZRot5.944",
    "G0 X47.355 Y-16.047 Z0 ZRot5.956",
    "G0 X47.553 Y-15.451 Z0 ZRot5.969",
    "G0 X47.743 Y-14.852 Z0 ZRot5.982",
    "G0 X47.926 Y-14.251 Z0 ZRot5.994",
    "G0 X48.101 Y-13.648 Z0 ZRot6.007",
    "G0 X48.269 Y-13.042 Z0 ZRot6.019",
    "G0 X48.429 Y-12.434 Z0 ZRot6.032",
    "G0 X48.582 Y-11.825 Z0 ZRot6.044",
    "G0 X48.726 Y-11.214 Z0 ZRot6.057",
    "G0 X48.863 Y-10.6 Z0 ZRot6.07",
    "G0 X48.993 Y-9.985 Z0 ZRot6.082",
    "G0 X49.114 Y-9.369 Z0 ZRot6.095",
    "G0 X49.228 Y-8.751 Z0 ZRot6.107",
    "G0 X49.334 Y-8.132 Z0 ZRot6.12",
    "G0 X49.433 Y-7.511 Z0 ZRot6.132",
    "G0 X49.523 Y-6.89 Z0 ZRot6.145",
    "G0 X49.606 Y-6.267 Z0 ZRot6.158",
    "G0 X49.681 Y-5.643 Z0 ZRot6.17",
    "G0 X49.748 Y-5.018 Z0 ZRot6.183",
    "G0 X49.807 Y-4.393 Z0 ZRot6.195",
    "G0 X49.858 Y-3.766 Z0 ZRot6.208",
    "G0 X49.901 Y-3.14 Z0 ZRot6.22",
    "G0 X49.937 Y-2.512 Z0 ZRot6.233",
    "G0 X49.964 Y-1.885 Z0 ZRot6.245",
    "G0 X49.984 Y-1.257 Z0 ZRot6.258",
    "G0 X49.996 Y-0.628 Z0 ZRot6.271",
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
    if (gcodeIndex < gcodeBuffer.size()) {

    /*if (gcodeIndex < gcodeBuffer.size()) {
        String command = gcodeBuffer[gcodeIndex++];
        Serial.println("Exécution du G-code : " + command);
        executeGCodeCommand(command);
    } 
    
    else {
        yield();
    }*/

    if (digitalRead(LIMIT_X_MIN) == LOW) {
        Serial.println("X Min Switch Pressed!");
    }

    if (digitalRead(LIMIT_X_MAX) == LOW) {
        Serial.println("X Max Switch Pressed!");
    }
    if (digitalRead(LIMIT_Y_MIN_L) == LOW || digitalRead(LIMIT_Y_MIN_R) == LOW) {
        Serial.println("Y Min Switch Pressed!");
    }
    if (digitalRead(LIMIT_Y_MAX_L) == LOW || digitalRead(LIMIT_Y_MAX_R) == LOW) {
        Serial.println("Y Max Switch Pressed!");
    }
    if (digitalRead(LIMIT_Z_MAX) == LOW) {
        Serial.println("Z Min Switch Pressed!");
    }

    delay(100);  // Small delay to reduce serial spam
}
