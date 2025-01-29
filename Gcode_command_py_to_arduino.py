import serial
import time

# Ouvre la connexion série avec l’Arduino (vérifiez le bon port !)
arduino = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
time.sleep(2)  # Attendre l'initialisation de l’Arduino

# Liste des commandes G-code à envoyer
gcode_commands = [
    "G0 X50 Y30\n",
    "G0 X100 Y60\n",
    "G1 X0 Y0\n"
]

for command in gcode_commands:
    print(f"Envoi : {command.strip()}")
    arduino.write(command.encode())  # Envoi de la commande à l’Arduino
    time.sleep(1)  # Pause pour permettre l'exécution

arduino.close()  # Fermer la connexion
print("Fin de l'envoi du G-code.")
