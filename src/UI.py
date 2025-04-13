"""
Interface utilisateur pour le Raspberry Pi.
Responsable code UI : Alexis Vegiard
Description: Code de l'inerface graphique permettant de contrôler le robot 3 axes.
"""

import PySimpleGUI as sg
import os
import math
import serial
import threading
import time
import subprocess

# Définir les variables
x: int = 0
y: int = 0
z: int = 0

run_state: bool = False

# Variable globale pour stocker la réponse de l'ESP32
response = ""

# Définir la mise en page de la première page
layout_setup = [
    [sg.Button('Setup'), sg.Button('Monitoring'), sg.Push(), sg.Button('X', button_color=('white', 'red'), key='Quit', size=(2, 1))],
    [sg.Text('Path'), sg.InputText(key='-Path-'), sg.FileBrowse(button_text='Browse', file_types=(("SVG Files", "*.svg"),)), sg.Button('Generate Gcode'), sg.Button('Upload')],
    [sg.Text('', key='-Path_Validation-', size=(40, 1))],
    [sg.Text('Temps estimé:'), sg.Text('', key='-Estimated_Time-')],
    [sg.Button('Home'), sg.Button('Run Gcode'), sg.Button('Start'), sg.Button('Stop'), sg.Button('Pause'), sg.Button('Reset')],
]

# Définir la mise en page de la deuxième page
layout_monitoring = [
    [sg.Button('Setup'), sg.Button('Monitoring'), sg.Push(), sg.Button('X', button_color=('white', 'red'), key='Quit', size=(2, 1))],
    [sg.Text('Axe X:'), sg.Text('0', key='-X-', size=(10, 1))],
    [sg.Text('Axe Y:'), sg.Text('0', key='-Y-', size=(10, 1))],
    [sg.Text('Axe Z:'), sg.Text('0', key='-Z-', size=(10, 1))],
    [sg.Text('Angle (ZRot):'), sg.Text('0', key='-Theta-', size=(10, 1)), sg.Text('°')],
    [sg.Text('Pression:'), sg.Text('0', key='-Force-', size=(10, 1)), sg.Text('N')],
    [sg.Text('Déplacement manuel:'), sg.Button('X+'), sg.Button('X-'), sg.Button('Y+'), sg.Button('Y-'), sg.Button('Z+'), sg.Button('Z-'), sg.Button('Zset')],
    [sg.Text('Progression G-code:'), sg.Text('0%', key='-Progress-', size=(10, 1))],
    [sg.Text('Ligne G-code en cours:'), sg.Text('0', key='-Current_Line-', size=(10, 1))],
    [sg.Text('Lignes restantes:'), sg.Text('0', key='-Left_Line-', size=(10, 1))],
    [sg.Button('Home'), sg.Button('Run Gcode'), sg.Button('Start'), sg.Button('Stop'), sg.Button('Pause'), sg.Button('Reset')]
]

# Créer les fenêtres
window1 = sg.Window('Robot 3 Axes - Page 1', layout_setup, size=(800, 480), finalize=True)
window2 = sg.Window('Robot 3 Axes - Page 2', layout_monitoring, size=(800, 480), finalize=True)
window2.hide()

# Fonction pour mettre à jour la réponse
def update_response():
    global response
    while True:
        if ser is not None and ser.in_waiting:
            try:
                # Lire les données disponibles sur le port série
                raw_response = ser.readline()
                try:
                    # Décoder les données en texte
                    response = raw_response.decode(errors='ignore').strip()
                    print(f"Message de l'ESP32 : {response}")
                except UnicodeDecodeError as e:
                    print(f"Erreur de décodage : {e}")
            except Exception as e:
                print(f"Erreur lors de la lecture des données série : {e}")
        time.sleep(0.1)  # Petit délai pour éviter de surcharger le CPU

# Configurer la connexion série
try:
    ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)  # Remplacez '/dev/ttyUSB0' par le port série correct
    print("Connexion série établie avec succès.")
    # Lancer le thread pour mettre à jour la réponse
    threading.Thread(target=update_response, daemon=True).start()
except serial.SerialException as e:
    print(f"Erreur lors de la connexion série : {e}")
    ser = None  # Désactiver la connexion série si une erreur survient

def send_list():
    if ser is None:
        print("Connexion série non disponible. Impossible d'envoyer les données.")
        return

    # Vérifier si le chemin du fichier est valide
    nom_fichier = "/media/lazyfox/ESD-USB/outputfile.txt"
    if not os.path.exists(nom_fichier):
        print("Chemin du fichier invalide.")
        return

    # Lecture du fichier et stockage des lignes qui commencent par 'G' dans une liste
    try:
        with open(nom_fichier, "r", encoding="utf-8") as fichier:
            lignes = [ligne.strip() for ligne in fichier.readlines() if ligne.strip().startswith('G')]
            print(f"Lignes filtrées : {lignes}")  # Afficher les lignes filtrées pour le débogage
    except Exception as e:
        print(f"Erreur lors de la lecture du fichier : {e}")
        return

    # Envoyer la commande initiale "UPLOAD"
    ser.write("UPLOAD\n".encode('utf-8'))
    print("Commande UPLOAD envoyée")

    # Attendre la réponse "READY_FOR_UPLOAD"
    response = read_from_esp32()
    while True:
        if ser.in_waiting:
            response = read_from_esp32()
            print(f"Réponse reçue : {response}")
        if response == "READY_FOR_UPLOAD" or "OK":
            print("ESP32 prêt pour l'upload")
            break
        time.sleep(0.1)

    # Envoyer les lignes du fichier G-code
    for ligne in lignes:
        ser.write((ligne + '\n').encode('utf-8'))
        print(f"Envoyé : {ligne}")
        response = "Waiting"  # Réinitialiser la réponse pour chaque ligne

        # Attendre l'accusé de réception "OK"
        while True:
            if ser.in_waiting:
                response = read_from_esp32()
                print(f"Réponse reçue : {response}")
            if response =="OK":
                print("ESP32 prêt pour l'upload")
                break
        time.sleep(0.1)

    # Envoyer la commande "END" pour signaler la fin de l'upload
    ser.write("END\n".encode('utf-8'))
    print("Commande END envoyée")
    time.sleep(0.1)

    print("Upload terminé avec succès.")
    

def send_command(cmd):
    if ser is None or not ser.is_open:
        print("Erreur : La connexion série n'est pas disponible.")
        return

    try:
        ser.write((cmd + '\n').encode('utf-8'))
        time.sleep(0.1)  # Petit délai pour éviter de surcharger le tampon série

        # Lire les réponses
        status_data = {}
        while ser.in_waiting:
            response = ser.readline().decode(errors='ignore').strip()

            # Analyser les données de la commande STATUS
            if cmd == "STATUS":
                if "emergencyStop:" in response:
                    status_data["emergencyStop"] = response.split(":")[1].strip()
                elif "isPaused:" in response:
                    status_data["isPaused"] = response.split(":")[1].strip()
                elif "isStarted:" in response:
                    status_data["isStarted"] = response.split(":")[1].strip()
                elif "gcodeIndex:" in response:
                    status_data["gcodeIndex"] = response.split(":")[1].strip()
                elif "gcodeSize:" in response:
                    status_data["gcodeSize"] = response.split(":")[1].strip()
                elif "gcodeFinished:" in response:
                    status_data["gcodeFinished"] = response.split(":")[1].strip()
                elif "Progression G-code:" in response:
                    status_data["progression"] = response.split(":")[1].strip()
                elif "Pos X:" in response:
                    status_data["posX"] = response.split(":")[1].strip()
                elif "Pos Y1:" in response:
                    status_data["posY1"] = response.split(":")[1].strip()
                elif "Pos Z:" in response:
                    status_data["posZ"] = response.split(":")[1].strip()
                elif "ZRot:" in response:
                    status_data["zRot"] = response.split(":")[1].strip()

        return status_data
    except serial.SerialException as e:
        print(f"Erreur lors de l'envoi des données série : {e}")
    except Exception as e:
        print(f"Erreur inattendue : {e}")

def read_from_esp32():
    if ser is not None and ser.in_waiting:
        try:
            # Lire les données disponibles sur le port série
            raw_response = ser.readline()
            try:
                # Décoder les données en texte
                response = raw_response.decode(errors='ignore').strip()
                print(f"Message de l'ESP32 : {response}")
                return response
            except UnicodeDecodeError as e:
                print(f"Erreur de décodage : {e}")
        except Exception as e:
            print(f"Erreur lors de la lecture des données série : {e}")

def generate_gcode():
    # Chemin vers le script Python externe
    script_path = "/media/lazyfox/ESD-USB/SVG2Gcode.py"
    inputfile = values['-Path-'] 
    outputfile = "/media/lazyfox/ESD-USB/outputfile.txt" # Remplacez par le chemin réel du script

    try:
        # Exécuter le script Python externe
        result = subprocess.run(['python', script_path, inputfile, outputfile], capture_output=True, text=True)

        # Afficher la sortie du script dans la console
        print("Gcode généré avec succès.")
        print("Sortie du script :")
        print(result.stdout)

        # Vérifier les erreurs
        if result.stderr:
            print("Erreurs du script :")
            print(result.stderr)
    except Exception as e:
        print(f"Erreur lors de l'exécution du script : {e}")

# Boucle d'événements
window = window1
while True:
    event, values = window.read(timeout=200)  # 5 fois par seconde

    # Envoyer la commande STATUS automatiquement à chaque itération
    status_data = send_command("STATUS")
    if status_data:
        # Mettre à jour les informations dans l'interface utilisateur
        window2['-X-'].update(status_data.get("posX", "0"))
        window2['-Y-'].update(status_data.get("posY1", "0"))
        window2['-Z-'].update(status_data.get("posZ", "0"))
        window2['-Theta-'].update(status_data.get("zRot", "0"))
        window2['-Progress-'].update(f"{status_data.get('progression', '0')}%")
        window2['-Current_Line-'].update(status_data.get("gcodeIndex", "0"))
        window2['-Left_Line-'].update(status_data.get("gcodeSize", "0"))
        
    if event == sg.WINDOW_CLOSED or event == 'Quit':
        break

    if event == 'Generate Gcode':
        # Générer le fichier G-code
        generate_gcode()

    if event == 'Upload':
        # Envoyer les commandes G-code à l'ESP32
        send_list()

    if event == 'Run Gcode':
        # Démarrer l'exécution du G-code
        send_command("RUN_GCODE")

    if event == 'Home':
        # Effectuer le homing
        send_command("HOME")

    if event == 'Start':
        run_state = True
        # Envoyer la commande Start au robot 3 axes
        send_command("START")
        print("Commande Start envoyée")
        
    if event == 'Stop':
        run_state = False
        # Envoyer la commande Stop au robot 3 axes
        send_command("STOP")
        print("Commande Stop envoyée")
        
    if event == 'Pause':
        run_state = False
        
    if event == 'Reset':
        # Envoyer la commande Reset au robot 3 axes
        send_command("RESET")
        print("Commande Reset envoyée")
        
    if event == 'X+':
        send_command("X_PLUS 1")
        print("Commande X+ envoyée")
        
    if event == 'X-':
        # Envoyer la commande X- au robot 3 axes
        send_command("X_MOINS 1")
        print("Commande X- envoyée")
        
    if event == 'Y+':
        # Envoyer la commande Y+ au robot 3 axes
        send_command("Y_PLUS 1")
        print("Commande Y+ envoyée")
        
    if event == 'Y-':
        # Envoyer la commande Y- au robot 3 axes
        send_command("Y_MOINS 1")
        print("Commande Y- envoyée")
        
    if event == 'Z+':
        # Envoyer la commande Z+ au robot 3 axes
        send_command("Z_UP 1")
        print("Commande Z+ envoyée")
        
    if event == 'Z-':
        # Envoyer la commande Z- au robot 3 axes
        send_command("Z_DOWN 1")
        print("Commande Z- envoyée")
        
    if event == 'Zset':
        # Envoyer la commande Zset au robot 3 axes
        print("Commande Zset envoyée")

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
    window2['-Force-'].update(100)

# Fermer la fenêtre
window.close()