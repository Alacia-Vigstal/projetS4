#ifndef DATA_H
#define DATA_H

#include <vector>
#include <Arduino.h>

// ======================= STOCKAGE DU G-CODE ==========================

// G-code à exécuter
std::vector<String> gcode_command = {
    "G0 X36.5658 Y228.5627 Z0 Zrot-90.0000",
    "G1 X36.5658 Y228.5627 Z1 Zrot-90.0000",
    "G1 X36.5658 Y253.5164 Z0 Zrot-90.0000",
    "G1 X36.5658 Y253.5164 Z0 Zrot-90.0000",
    "G0 X36.5658 Y253.5164 Z0 Zrot0.0000",
    "G1 X36.5658 Y253.5164 Z1 Zrot0.0000",
    "G1 X60.0370 Y253.5164 Z0 Zrot0.0000",
    "G1 X60.0370 Y253.5164 Z0 Zrot0.0000",
    "G0 X60.0370 Y253.5164 Z0 Zrot90.5787",
    "G1 X60.0370 Y253.5164 Z1 Zrot90.5787",
    "G1 X59.7900 Y229.0568 Z0 Zrot90.5787",
    "G1 X59.7900 Y229.0568 Z0 Zrot90.5787",
    "G0 X59.7900 Y229.0568 Z0 Zrot178.7811",
    "G1 X59.7900 Y229.0568 Z1 Zrot178.7811",
    "G1 X36.5658 Y228.5627 Z0 Zrot178.7811"
};

#endif
// 