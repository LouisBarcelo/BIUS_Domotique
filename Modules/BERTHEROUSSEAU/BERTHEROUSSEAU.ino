// Brancher les DHTs output sur D6, D7, et D8
// Brancher le capteur photo sur A0


/************************************** CAYENNE *****************************************/
#define CAYENNE_PRINT Serial               
#include <CayenneMQTTESP8266.h>

// If too many errors when connecting, restart
int failedConnections = 0;

// WiFi network info.
char ssid[] = "Vamoslilo";
char wifiPassword[] = "jemesure5pieds0";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "75b6d920-cb5b-11e8-8a08-61f9e5c4f140";
char password[] = "13a3664856850223f1994846cd5aa6d668226da9";
char clientID[] = "13c365e0-3a66-11eb-8779-7d56e82df461";

// Channels cayenne
#define VIRTUAL_CHANNEL 2                                                       // Luminosite

#define VIRTUAL_CHANNEL 3                                                       // Temperature - 1
#define VIRTUAL_CHANNEL 4                                                       // Humidite - 1

#define VIRTUAL_CHANNEL 5                                                       // Temperature - 2
#define VIRTUAL_CHANNEL 6                                                       // Humidite - 2

#define VIRTUAL_CHANNEL 7                                                       // Temperature - 3
#define VIRTUAL_CHANNEL 8                                                       // Humidite - 3


/*********************************Donnees********************************************/
// Temps entre les prises de donnees (en secondes)
#define DELAIS_PRISE_DONNEES 10 // Secondes
unsigned long lastMillis = 0;   // Dernière prise de mesure

// photo
int photoPin = A0;

// DHT22 AM2302 - adafruit dht library + adafruit unified sensors library
#include "DHT.h"
#define DHTTYPE DHT22
int dhtPin1 = D6;
int dhtPin2 = D7;
int dhtPin3 = D8;

DHT dht1(dhtPin1, DHTTYPE);
DHT dht2(dhtPin2, DHTTYPE);
DHT dht3(dhtPin3, DHTTYPE);

int lastDHTRead = 0;

void setup() {
  Serial.begin(9600);
  delay(10);   
  
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("We connected boys");
    Cayenne.loop();

    // S'il y a eu assez de temps, republier les données
    if (millis() - lastMillis > DELAIS_PRISE_DONNEES*1000) {
      lastMillis = millis();

      // Envoyer valeur analog photosensor
      Cayenne.virtualWrite(2, analogRead(photoPin));

      // Lire dht en alternance sinon ils fuckent leurs valeurs
      if (lastDHTRead == 0) {
        readDht1();
        lastDHTRead++;
      } else if (lastDHTRead == 1) {
        readDht2();
        lastDHTRead++;
      } else if (lastDHTRead == 2) {
        readDht3();
        lastDHTRead = 0;
      }

      
    }
  } else {
    failedConnections++;
    // Si pas connecté plus de 10 fois, restart
    if (failedConnections > 10) {
      Serial.println("No wifi connection, restarting.");
      ESP.restart();
    } else {
      // Sinon, delay 1000
      delay(1000);
    }
  }
  delay(5000);
}

void readDht1() {
  // DHT readings
  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht1.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht1.readTemperature();

  // Check if any reads failed, if so, do not publish, else, publish to cayenne
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor 1!");
  } else {
    Cayenne.virtualWrite(3, t);
    Cayenne.virtualWrite(4, h);
  }
}

void readDht2() {
  // DHT readings
  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht2.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht2.readTemperature();

  // Check if any reads failed, if so, do not publish, else, publish to cayenne
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor 2!");
  } else {
    Cayenne.virtualWrite(5, t);
    Cayenne.virtualWrite(6, h);
  }
}

void readDht3() {
  // DHT readings
  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht3.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht3.readTemperature();

  // Check if any reads failed, if so, do not publish, else, publish to cayenne
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor 3!");
  } else {
    Cayenne.virtualWrite(7, t);
    Cayenne.virtualWrite(8, h);
  }
}
