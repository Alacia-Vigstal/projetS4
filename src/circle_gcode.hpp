#ifndef CIRCLE_GCODE_HPP
#define CIRCLE_GCODE_HPP

#include <Arduino.h>
#include <math.h>

class CircleGCode {
public:
    CircleGCode(float centerX, float centerY, float radius, int segments)
        : centerX(centerX), centerY(centerY), radius(radius), segments(segments) {}

    void generateGCode(void (*outputFunc)(const char*)) {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "G0 X%.2f Y%.2f\n", centerX + radius, centerY);
        outputFunc(buffer);
        
        for (int i = 1; i <= segments; i++) {
            float angle = (i / (float)segments) * 2 * M_PI;
            float x = centerX + radius * cos(angle);
            float y = centerY + radius * sin(angle);
            snprintf(buffer, sizeof(buffer), "G1 X%.2f Y%.2f\n", x, y);
            outputFunc(buffer);
        }
    }

private:
    float centerX;
    float centerY;
    float radius;
    int segments;
};

#endif // CIRCLE_GCODE_HPP