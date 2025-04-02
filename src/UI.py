import PySimpleGUI as sg
import os
import math
import serial
import threading  

# Définir les variables
x: int = 0
y: int = 0
z: int = 0

run_state: bool = False

# Définir la mise en page de la première page
layout_setup = [
    [sg.Button('Setup'), sg.Button('Monitoring'), sg.Push(), sg.Button('X', button_color=('white', 'red'), key='Quit', size=(2, 1))],
    [sg.Text('Path'), sg.InputText(key='-Path-'), sg.FileBrowse(button_text='Browse', file_types=(("SVG Files", "*.svg"),))],
    [sg.Text('', key='-Path_Validation-', size=(40, 1))],
    [sg.Text('Temps estimé:'), sg.Text('', key='-Estimated_Time-')],
    [sg.Button('Start'), sg.Button('Pause'), sg.Button('Resume')],
]

# Définir la mise en page de la deuxième page
layout_monitoring = [
    [sg.Button('Setup'), sg.Button('Monitoring'), sg.Push(), sg.Button('X', button_color=('white', 'red'), key='Quit', size=(2, 1))],
    [sg.Text('Axe X'), sg.ProgressBar(250, orientation='h', size=(20, 20), key='-X-'), sg.Text('', key='-X-')],
    [sg.Text('Axe Y'), sg.ProgressBar(150, orientation='h', size=(20, 20), key='-Y-'), sg.Text('', key='-Y-')],
    [sg.Text('Axe Z'), sg.ProgressBar(50 , orientation='h', size=(20, 20), key='-Z-'), sg.Text('', key='-Z-')],
    [sg.Text('Angle'), sg.Text('', key='-Theta-'), sg.Text('°')],
    [sg.Text('Pression'), sg.Text('', key='-Force-'), sg.Text('N')],
    [sg.Text('Limit Switches: X1: '), sg.Text('', key='-LS_X1-'), sg.Text(' X2: '), sg.Text('', key='-LS_X2-'), sg.Text(' Y1: '), sg.Text('', key='-LS_Y1-'), sg.Text(' Y2: '), sg.Text('', key='-LS_Y2-')],
    [sg.Text('Progression:'), sg.Text('Ligne Gcode en cours'), sg.Text('', key='-Current_Line-')],
    [sg.ProgressBar(100, orientation='h', size=(20, 20), key='-Progress-'), sg.Text('Lignes restantes:'), sg.Text('', key='-Left_Line-')],
    [sg.Button('Start'), sg.Button('Pause'), sg.Button('Resume')]
]

# Créer les fenêtres
window1 = sg.Window('Robot 3 Axes - Page 1', layout_setup, size=(800, 480), finalize=True)
window2 = sg.Window('Robot 3 Axes - Page 2', layout_monitoring, size=(800, 480), finalize=True)
window2.hide()

# Boucle d'événements
window = window1
while True:
    event, values = window.read(timeout=200)  # 5 times per second
    if event == sg.WINDOW_CLOSED or event == 'Quit':
        break

    if event == 'Start':
        run_state = True

    if event == 'Pause':
        run_state = False

    if run_state == True:
        if '-X-' in values:
            x = values['-X-']
            y = values['-Y-']
            z = values['-Z-']
        # Ici, vous pouvez envoyer les commandes au robot 3 axes
        # print(f"Commandes envoyées : X={x}, Y={y}, Z={z}")
    if event == 'Monitoring' and window == window1:
        window.hide()
        window = window2
        window.un_hide()
    if event == 'Setup' and window == window2:
        window.hide()
        window = window1
        window.un_hide()
    
    # Valider le chemin
    if window == window1 and '-Path-' in values:
        path = values['-Path-']
        if path:
            if os.path.exists(path):
                window1['-Path_Validation-'].update('Path is valid', text_color='green')
            else:
                window1['-Path_Validation-'].update('Path is invalid', text_color='red')
        else:
            window1['-Path_Validation-'].update('Path is empty', text_color='red')
                
    # Simuler la mise à jour des valeurs depuis un autre programme
    window1['-Estimated_Time-'].update('10 minutes')
    window2['-Theta-'].update(math.atan2(y, x) * 180 / math.pi)
    window2['-Force-'].update(100)

    # Simuler la mise à jour des états des limit switches
    window2['-LS_X1-'].update('ON')
    window2['-LS_X2-'].update('OFF')
    window2['-LS_Y1-'].update('ON')
    window2['-LS_Y2-'].update('OFF')

# Fermer la fenêtre
window.close()

'''
# ConnexionESP32 Future
 Connectez le TX de l'ESP32 au RX du Raspberry Pi.
Connectez le RX de l'ESP32 au TX du Raspberry Pi.
Connectez les GND des deux appareils ensemble.
Configuration du port série sur le Raspberry Pi
Assurez-vous que le port série est activé sur le Raspberry Pi et que vous utilisez le bon port série (/dev/ttyUSB0 ou autre).
Avec cette configuration, l'ESP32 envoie des données au Raspberry Pi via le port série. Le Raspberry Pi lit ces données et met à jour les valeurs dans l'interface PySimpleGUI en conséquence.

# Code pour le Raspberry Pi

# Configurer la connexion série
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)  # Remplacez '/dev/ttyUSB0' par le port série correct

# Fonction pour lire les données série
def read_serial():
    global x_value, y_value, z_value
    while True:
        if ser.in_waiting > 0:
            data = ser.readline().decode('utf-8').strip()
            # Suppose the data is in the format "X:1000,Y:2000,Z:50"
            values = data.split(',')
            for value in values:
                key, val = value.split(':')
                if key == 'X':
                    x_value = int(val)
                elif key == 'Y':
                    y_value = int(val)
                elif key == 'Z':
                    z_value = int(val)

# Démarrer le thread de lecture série
serial_thread = threading.Thread(target=read_serial)
serial_thread.daemon = True
serial_thread.start()

    # Update the sliders on page 2 with the stored values
    window2['-X-'].update(x_value)
    window2['-Y-'].update(y_value)
    window2['-Z-'].update(z_value)

    # Simuler la mise à jour des valeurs depuis un autre programme
    window1['-Estimated_Time-'].update('10 minutes')
    window2['-Theta-'].update(math.atan2(y, x) * 180 / math.pi)
    window2['-Force-'].update(100)

    # Simuler la mise à jour des états des limit switches
    window2['-LS_X1-'].update('ON')
    window2['-LS_X2-'].update('OFF')
    window2['-LS_Y1-'].update('ON')
    window2['-LS_Y2-'].update('OFF')

# Code pour l'ESP32
#include <HardwareSerial.h>

void setup() {
  Serial.begin(115200);  // Initialiser la communication série
}

void loop() {
  // Envoyer les données au format "X:1000,Y:2000,Z:50"
  String data = "X:1000,Y:2000,Z:50";
  Serial.println(data);

  delay(5000);  // Envoyer les données toutes les 5 secondes
}
'''
