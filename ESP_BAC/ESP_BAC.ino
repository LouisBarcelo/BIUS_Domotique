//#define CAYENNE_DEBUG
#define CAYENNE_PRINT Serial                 //wifi
#include <CayenneMQTTESP8266.h>

// S'il y a trop d'échecs de connection, deep sleep et reset 
int failedConnections = 0;
#define SLEEP_TIME 20e6

// Temps entre les prises de donnees (en secondes)
#define DELAIS_PRISE_DONNEES 60 // Secondes
unsigned long lastMillis = 0;   // Dernière prise de mesure

// WiFi network info.
char ssid[] = "SmartRGE1BB";
char wifiPassword[] = "s9003502";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "75b6d920-cb5b-11e8-8a08-61f9e5c4f140";
char password[] = "13a3664856850223f1994846cd5aa6d668226da9";
char clientID[] = "a4d64b50-cb5b-11e8-ac9c-271082632120";

// Channels cayenne
#define VIRTUAL_CHANNEL 3     // Humidité du sol
#define VIRTUAL_CHANNEL 5     // Humidité ambiente
#define VIRTUAL_CHANNEL 6     // Température ambiente
#define VIRTUAL_CHANNEL 7     // Litres received

// Humidite du sol
int pinHumiditeSol = A0;      // Senseur d'humidite analog
int pinVccSondeHum = D2;      // D2 pour alimenter la sonde lors de la prise de mesures 
int humiditeSol = 0;          // Valeur de l'humidite du sol

// DHT22 AM2302 - adafruit dht library + adafruit unified sensors library
#include "DHT.h"
#define DHTTYPE DHT22         // Sorte de DHT
int dhtPin = D1;              // La pin de réception du DHT22
DHT dht(dhtPin, DHTTYPE);     // Le dht wrapper

// Flow rate sensor
const int  flowSensorPin = D5;// La pin de réception du flow sensor
int revFlowSensor = 0;        // variable to store the “rise ups” from the flowmeter pulses
int litres = 0;               // Numbre de litres reçus
int lastRevFlowSensor = 0;    // La valeur lors de la dernière lecture (pour voir si il y a eu changement)
#define REV_TO_LITRES 535     // La valeur de caibration du sensor


void setup() {
  Serial.begin(9600);
  delay(10);
  
  pinMode(pinVccSondeHum, OUTPUT);
  digitalWrite(pinVccSondeHum, LOW);
  delay(10); 
  
  // Initialization of the variable “flowSensorPin” as INPUT (D2 pin)
   pinMode(flowSensorPin, INPUT);
    
  // Attach an interrupt to the ISR vector
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pin_ISR, RISING);
   
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Cayenne.loop();

    // Voir s'il y a eu changement du flow (assez pour ajouter un litre de plus)
    if(revFlowSensor > REV_TO_LITRES ) {
      // If enough revolutions have passed, increase the litre count and reset the revolutions
      litres++;
      revFlowSensor = 0;
    }

    // S'il y a eu assez de temps, republier les données
    if (millis() - lastMillis > DELAIS_PRISE_DONNEES*1000) {
      lastMillis = millis();

      // Si le nombre de révolutions n'a pas changé depuis la dernière mesure, envoyer à Cayenne et reset tout
      if (lastRevFlowSensor == revFlowSensor) {
        Cayenne.virtualWrite(7, litres);
        litres = 0;
        revFlowSensor = 0;
        lastRevFlowSensor = 0;
      }

      // Humidite du sol
      digitalWrite(pinVccSondeHum, HIGH);
      delay(100);
      humiditeSol = analogRead(pinHumiditeSol);
      delay(50);
      digitalWrite(pinVccSondeHum, LOW);

      // Check to see if reading failed, if failed, do not publish, else, publish to cayenne
      if (isnan(humiditeSol)) {
        Serial.println("Failed to read from humidity sensor!");
      } else {
        Cayenne.virtualWrite(3, humiditeSol);
      }

      // DHT readings
      // Reading temperature or humidity takes about 250 milliseconds!
      float h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();

      // Check if any reads failed, if so, do not publish, else, publish to cayenne
      if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
      } else {
        Cayenne.virtualWrite(5, h);
        Cayenne.virtualWrite(6,t);
      }

    }
  } else {
    failedConnections++;
    // Si pas connecté plus de 10 fois, sleep pour SLEEP TIME et réveil après
    if (failedConnections > 10) {
      Serial.println("No wifi connection, going to sleep before restarting.");
      ESP.deepSleep(SLEEP_TIME);
    } else {
      // Sinon, delay 1000
      delay(1000);
    }
  }
}

/***************************************** Flow sensor ****************************************************/

//Interrupt function, so that the counting of pulse “rise ups” dont interfere with the rest of the code  (attachInterrupt)
void pin_ISR() {   
    revFlowSensor++;
}
