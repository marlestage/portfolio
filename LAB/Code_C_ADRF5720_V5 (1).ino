const int VDD = 4;  // Pin Vdd (3.3V)
const int VSS = 5;  // Pin Vss (-3.3V)
const int LE = 7;   // Pin LE (Latch Enable)
const int OE = 6;   // Pin LE (Output Enable du translateur de niveau)

// Variables pour stocker l'état
bool powerOn = false;  // État de l'alimentation (Allumé ou Éteint)
float att = 0.0;       // Valeur actuelle de l'atténuation en dB

// Pins de contrôle pour l'ADRF5720 (D0 à D5)
const int pins[] = {13, 12, 11, 10, 9, 8};  // Pins D0 à D5

// Pins des LEDs 
const int Led[] = {A5, A4, A3, A2, A1, A0}; // Pins A0 à A5

void setup() {
  // Initialisation de la communication série
  Serial.begin(9600);

  // Initialisation des lignes de commande de l'alim et du LE
  pinMode(VDD, OUTPUT);
  pinMode(VSS, OUTPUT);
  pinMode(LE, OUTPUT);

  // Éteindre par défaut les lignes d'alimentation et de latch
  digitalWrite(VSS, LOW);
  delay(5);
  digitalWrite(VDD, LOW);
  digitalWrite(LE, LOW);

  // Initialisation des pins de l'ADRF5720 en mode OUTPUT
  for (int i = 0; i < 6; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);  // Initialiser les pins D0 à D5 à LOW
  }

  // Initialisation des LEDs en mode OUTPUT
  for (int i = 0; i < 6; i++) {
    pinMode(Led[i], OUTPUT);
    digitalWrite(Led[i], LOW);  // Initialiser les pins D0 à D5 à LOW
  }
}

void loop() {
  // Vérifier si une commande est disponible via la communication série
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');  // Lire toute la commande jusqu'à un saut de ligne
    command.trim();  // Nettoyer les espaces inutiles au début et à la fin

    processCommand(command);  // Traiter la commande reçue
  }
}

void processCommand(String command) {
  int spaceIdx = command.indexOf(' ');  // Trouver le premier espace
  String commandType = command.substring(0, spaceIdx);  // Action demandée
  String value = command.substring(spaceIdx + 1,'\n');  // Valeur : power ou att

  if (spaceIdx == -1) { // Pas d'espace (fonction de monitoring)
    if(command == "power"){ // Demande de l'état de l'alimentation
      MonPower();
    }
    else {
      if(command == "att"){ // Demande de la valeur de l'atténuation
        MonAtt();
      }
      else{
        Serial.println("ERR0"); 
      }
    } 
  }

  else if(commandType == "power"){ // Commence par "power"
    CmdPower(value);
  }

  else if (commandType == "att"){ // Commence par "att"
    CmdAtt(value);
  } 
  
  else{
    Serial.println("ERR1"); // Possède un espace mais n'est pas repertorier
  }
}

// Afficher le statut de l'alimentation
void MonPower(void){
  String powerState;  // Déclarer une variable pour l'état de l'alimentation sous forme de texte
  // Vérifier si l'alimentation est allumée ou éteinte
  if (powerOn == true) { 
    powerState = "Allumé";  // Mettre "Allumé" dans powerStateText
  } 
  else {
    powerState = "Éteint";  // Mettre "Éteint" dans powerStateText
  }
  Serial.print("État de l'alimentation : ");
  Serial.println(powerState);
}
// Afficher le statut actuel de l'atténuation
void MonAtt(void){
  Serial.print("Atténuation actuelle : ");
  Serial.print(att);
  Serial.println(" dB");
}

// Contrôle de l'alimentation
void CmdPower(String value){
  if (value == "1" || value == "on") {
    digitalWrite(VDD, HIGH);
    delay(10);
    digitalWrite(VSS, HIGH);
    powerOn = true;
    Serial.println("Alimentation allumée");
    digitalWrite(OE,HIGH); // Allume le translateur de niveau
  } 
  else if (value == "0" || value == "off") {
    att = 0.0;    // Réinitialiser l'attenuation 
    sendAtt(0);   // Forcer la valeur 0 physiquement
    delay(5);
    digitalWrite(OE,LOW); // Eteint le translateur de niveau 

    digitalWrite(VSS, LOW);
    delay(10);
    digitalWrite(VDD, LOW);
    powerOn = false;
    Serial.println("Alimentation éteinte");
  } 
  else {
    Serial.println("ERR2");
  }
}

// Contrôle de l'atténuation
void CmdAtt(String value){
  // Vérifie si l'alimentation est allumée
  if (!powerOn) {
    Serial.println("ERR5");
    return;  // Retourne sans faire d'autres actions si l'alimentation n'est pas allumée
  }
  int pointCount = 0;

  // Vérifie si la chaîne contient des caractères invalides (autres que des chiffres et un seul point)
  for (int j = 0; j < value.length(); j++) {
    if (value[j] == '.') {
      pointCount++;  // Incrémente le compteur du point
      if (pointCount > 1) {  // Si plus d'un point
        Serial.println("ERR3");
        return;
      }
    } 
    else if (!isDigit(value[j])) {  // Si ce n'est pas un chiffre
      Serial.println("ERR3");
      return;
    }
  }
  
  // Vérifie si la chaîne ne contient pas de virgule
  if (value.indexOf(',') != -1) { // Si il y a une virgule
    Serial.println("ERR3");       // Utilisez un point "."  pas une virgule ","
    return;
  }

  float attVal = value.toFloat();  // Conversion en float

  // Vérification de la validité de l'atténuation
  if (attVal >= 0.0 && attVal <= 31.5 && fmod(attVal * 2, 1) == 0) {
    att = attVal;            // Mémoriser la valeur
    sendAtt(attVal);         // Envoyer la valeur aux broches
  } 
  else {
    Serial.println("ERR4");
  }
}

void sendAtt(float attValue) {
  // Multiplier par 2 pour les pas de 0.5dB puis convertie de float à integer
  int binValue = int(attValue * 2); 

  // Écrire chaque bit sur les broches D0 à D5
  for (int i = 0; i < 6; i++) {
    int bitValue = (binValue >> i) & 1;
    digitalWrite(pins[i], bitValue);
    digitalWrite(Led[i],bitValue);
  }

  delayMicroseconds(5);
  digitalWrite(LE, HIGH);   // Latch Enable haut
  delayMicroseconds(10);
  digitalWrite(LE, LOW);    // Latch Enable bas
  
  Serial.print("Atténuation en binaire envoyée : ");
  Serial.print(binValue, BIN);
  Serial.print(" ou en dB : ");
  Serial.print(att);
  Serial.println("dB");
}