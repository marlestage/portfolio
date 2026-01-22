# Importer la bibliothèque nécessaire depuis le terminal pour la communication série : pip install pyserial
import serial
import time
import json

# Chargement des paramètres à partir du fichier JSON (port, baudrate, dictionnaire d'erreurs, etc.)
def read_parameter(file="Parameter.json"):
    with open(file, "r") as f:
        return json.load(f)

# Envoie une commande série à l'Arduino et affiche la réponse ou une éventuelle erreur
def write_read(serialArduino, command, erreurs):
    serialArduino.write((command + '\n').encode())  # Envoi de la commande
    #print("> Commande envoyée :", command)
    time.sleep(0.5)  # Délai nécessaire pour laisser le temps à l'Arduino de répondre

    if serialArduino.in_waiting:  # Vérifie si des données sont disponibles en lecture
        data = serialArduino.read(serialArduino.in_waiting).decode().strip()  # Lecture et décodage de la réponse
        if data in erreurs:  # Si la réponse correspond à une erreur connue
            print("Attention, erreur :", erreurs[data])
        else:
            print("> Réponse de l'Arduino :", data)

# Envoie automatiquement une série de valeurs d'atténuation avec délai entre chaque
def mesureSerie(serialArduino, erreurs):
    start = float(input("Atténuation de départ (dB) : "))
    step = float(input("Pas entre chaque valeur (multiple de 0.5) : "))
    delayseries = float(input("Délai entre chaque mesure (secondes) : "))

    max_att = 31.5
    count = int((max_att - start) // step) + 1  # Nombre total de pas
    last_att = start + step * (count - 1)  # Calcul de la dernière valeur d'atténuation

    if last_att > max_att:
        last_att -= step  # Ajustement si dépassement de la valeur max autorisée par l'ADRF5720

    print(f"\n Série prévue : de {start:.2f} dB à {last_att:.2f} dB par pas de {step} dB.")
    print("Délai entre mesures :", delayseries, "secondes")

    attenuation = start
    while attenuation <= max_att:
        command = "att " + str(attenuation)  # Construction de la commande à envoyer
        write_read(serialArduino, command, erreurs)
        time.sleep(delayseries)
        attenuation += step

    print("\n Fin de la série de mesures")

# Fonction principale du programme
def start_program():
    # Chargement des paramètres json
    parameter = read_parameter()
    erreurs = parameter["errors"]
    port = parameter["port"]
    baudrate = parameter["baudrate"]

    # Initialisation de la communication série avec l'Arduino
    serialArduino = serial.Serial(port, baudrate, timeout=1)
    print(f"\n Connexion réussie sur le port {port} à {baudrate} bauds.")
    time.sleep(1)  # Délai d'initialisation de l'Arduino après ouverture du port

    serialArduino.reset_input_buffer()  # Nettoyage du buffer d'entrée pour éviter les messages parasites

    # Boucle de commande utilisateur
    while True:
        command = input("\n Entrer une commande (ex : 'att 10', 'serie', 'power on', 'exit') : ").strip().lower()
        
        if command == "exit":
            print("Arrêt du programme.")
            break  # Sortie de la boucle principale, renvoie à la boucle de redémarrage
        elif command == "serie":
            mesureSerie(serialArduino, erreurs)
        else:
            write_read(serialArduino, command, erreurs)
    serialArduino.close()  # Fermeture propre du port série

# Première exécution du programme
start_program()

# Boucle permettant de redémarrer le programme sans relancer le script
while True:
    restart = input("\n Souhaitez-vous fermer le programme ? (oui/non) : ").strip().lower()
    if restart == "non":
        start_program()
    else:
        print("Programme terminé.")
        break
