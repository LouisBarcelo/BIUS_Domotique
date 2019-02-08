/************************************** OTA *****************************************/
const int FW_VERSION = 3;                                                        // Version number, don't forget to update this on changes
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// Note the raw.githubuserconent, this allows us to access the contents at the url, not the webpage itself
const char* fwURLBase = "https://raw.githubusercontent.com/BIUS-USherbrooke/BIUS_Domotique/ESP_SERRE/Modules/ESP_SERRE/ESP_SERRE"; // IP adress to the subfolder containing the binary and version number for this specific device

/************************************** CAYENNE *****************************************/
#define CAYENNE_PRINT Serial               
#include <CayenneMQTTESP8266.h>

// If too many errors when connecting, deep sleep (which will restart the esp)
// PIN D0 OF THE NODEMCU MUST BE CONNECTed TO THE RST PIN
int failedConnections = 0;
#define SLEEP_TIME 20e6

// WiFi network info.
char ssid[] = "MotoCoco";
char wifiPassword[] = "VG360esp";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "75b6d920-cb5b-11e8-8a08-61f9e5c4f140";
char password[] = "13a3664856850223f1994846cd5aa6d668226da9";
char clientID[] = "588c5190-cb60-11e8-a770-b31a480d1229";

// Channels cayenne
#define VIRTUAL_CHANNEL 98                                                       // Version number
#define VIRTUAL_CHANNEL 0                                                       // Channel to force an OTA update

#define VIRTUAL_CHANNEL 1                                                       //  CO2
#define VIRTUAL_CHANNEL 2                                                       // Temperature
#define VIRTUAL_CHANNEL 3                                                       // Humidite

#define VIRTUAL_CHANNEL 10                                                       // Monter rideau
#define VIRTUAL_CHANNEL 11                                                       // Temps pour monter rideau


#define VIRTUAL_CHANNEL 12                                                       // Descendre rideau

int co2Pin = A0;


/*********************************Donnees********************************************/
// Temps entre les prises de donnees (en secondes)
#define DELAIS_PRISE_DONNEES 10 // Secondes
unsigned long lastMillis = 0;   // Dernière prise de mesure

// DHT22 AM2302 - adafruit dht library + adafruit unified sensors library
#include "DHT.h"
#define DHTTYPE DHT22         // Sorte de DHT
int dhtPin = D6;              // La pin de réception du DHT22
DHT dht(dhtPin, DHTTYPE);     // Le dht wrapper

int pinActuateurRideauBas = D7;

int pinMoteurBaisser = D3;
int pinMoteurMonter = D4;
unsigned long debutMonterRideau = 0;   // Dernière prise de mesure
unsigned long secondesMonterRideau = 37;  // Temps pour monter rideau
bool monterRideau = false;
bool descendreRideau = false;

bool rideauPositionHaut = false;  // False pour bas, true pour haut
int readingActuateurBas = 0;

void setup() {
  Serial.begin(9600);
  delay(10);

  pinMode(pinActuateurRideauBas, INPUT);

  pinMode(pinMoteurBaisser, OUTPUT);
  digitalWrite(pinMoteurBaisser, HIGH);

  pinMode(pinMoteurMonter, OUTPUT);
  digitalWrite(pinMoteurMonter, HIGH);

  digitalWrite(pinMoteurBaisser, LOW);

  readingActuateurBas = digitalRead(pinActuateurRideauBas);

  while (readingActuateurBas != HIGH) {
    delay(50);
    readingActuateurBas = digitalRead(pinActuateurRideauBas);
  }
  rideauPositionHaut = false;
  digitalWrite(pinMoteurBaisser, HIGH);
  delay(100);
  digitalWrite(pinMoteurMonter, LOW);

  while (readingActuateurBas != LOW) {
    delay(50);
    readingActuateurBas = digitalRead(pinActuateurRideauBas);
  }
  digitalWrite(pinMoteurMonter, HIGH);    
  
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Cayenne.loop();

    // S'il y a eu assez de temps, republier les données
    if (millis() - lastMillis > DELAIS_PRISE_DONNEES*1000) {
      lastMillis = millis();

      Cayenne.virtualWrite(98,FW_VERSION);                                         // Send the version number to Cayenne

      // Envoyer valeur analog CO2
      Cayenne.virtualWrite(1, analogRead(co2Pin));

      // DHT readings
      // Reading temperature or humidity takes about 250 milliseconds!
      float h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();

      // Check if any reads failed, if so, do not publish, else, publish to cayenne
      if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
      } else {
        Cayenne.virtualWrite(3, h);
        Cayenne.virtualWrite(2,t);
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
  checkMonterRideau();
  checkDescendreRideau();
}

/***************************************** Cayenne Ins **********************************************/
// Check for updates on web server
CAYENNE_IN(0)
{
  checkForUpdates();
}

// Monter le rideau
CAYENNE_IN(10)
{
  if (monterRideau == false && descendreRideau == false && rideauPositionHaut == false) {
    digitalWrite(pinMoteurMonter, LOW);
    debutMonterRideau = millis();
    monterRideau = true;
  }
}

// Temps pour monter le rideau
CAYENNE_IN(11)
{
  secondesMonterRideau = getValue.asInt();
}

// Descendre le rideau
CAYENNE_IN(12)
{
  if (monterRideau == false && descendreRideau == false && rideauPositionHaut == true) {
    digitalWrite(pinMoteurBaisser, LOW);
    descendreRideau = true;
  }
}

void checkMonterRideau() {
  // Si rideau est actionné depuis assez longtemps
  if (millis() - debutMonterRideau >= secondesMonterRideau * 1000 && monterRideau == true) {
    digitalWrite(pinMoteurMonter, HIGH);
    monterRideau = false;
    rideauPositionHaut = true;
  }
}

void checkDescendreRideau() {
  // Si actuateur est ouvert, remonter un peu pi arreter
  if (digitalRead(pinActuateurRideauBas) == HIGH) {
    digitalWrite(pinMoteurBaisser, HIGH);   // arreter de descendre
    digitalWrite(pinMoteurMonter, LOW);
    readingActuateurBas = digitalRead(pinActuateurRideauBas);

  while (readingActuateurBas != LOW) {
    delay(50);
    readingActuateurBas = digitalRead(pinActuateurRideauBas);
  }
  digitalWrite(pinMoteurMonter, HIGH);    
    descendreRideau = false;
    rideauPositionHaut = false;
  }
}

/***************************************** http update function *************************************/
// This function checks the web server to see if a new version number is available, if so, it updates with the new firmware (binary)
void checkForUpdates() {
  String fwImageURL = String(fwURLBase);
      fwImageURL.concat( ".ino.nodemcu.bin" );                                              // Adds the url for the binary
      Serial.println(fwImageURL);
      t_httpUpdate_return ret = ESPhttpUpdate.update( fwImageURL , "", "CC:AA:48:48:66:46:0E:91:53:2C:9C:7C:23:2A:B1:74:4D:29:9D:33");             // Update the esp with the new binary, third is the certificate of the site
      delay(50);

      switch(ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("no updates");
          break;
  }
}
