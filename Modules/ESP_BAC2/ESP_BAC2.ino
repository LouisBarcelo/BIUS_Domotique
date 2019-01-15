/************************************** OTA *****************************************/
const int FW_VERSION = 1;                                                        // Version number, don't forget to update this on changes
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// Note the raw.githubuserconent, this allows us to access the contents at the url, not the webpage itself
const char* fwURLBase = "https://raw.githubusercontent.com/BIUS-USherbrooke/BIUS_Domotique/ESP_Bac2/Modules/ESP_BAC2/ESP_BAC2"; // IP adress to the subfolder containing the binary and version number for this specific device

/************************************** CAYENNE *****************************************/
#define CAYENNE_PRINT Serial               
#include <CayenneMQTTESP8266.h>

// If too many errors when connecting, deep sleep (which will restart the esp)
// PIN D0 OF THE NODEMCU MUST BE CONNECTed TO THE RST PIN
int failedConnections = 0;
#define SLEEP_TIME 20e6

// WiFi network info.
char ssid[] = "SmartRGE1BB";
char wifiPassword[] = "s9003502";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "75b6d920-cb5b-11e8-8a08-61f9e5c4f140";
char password[] = "13a3664856850223f1994846cd5aa6d668226da9";
char clientID[] = "f5511690-18ff-11e9-8cb9-732fc93af22b";

// Channels cayenne
#define VIRTUAL_CHANNEL 98                                                       // Version number
#define VIRTUAL_CHANNEL 0                                                       // Channel to force an OTA update

#define VIRTUAL_CHANNEL 3     // Humidité du sol
#define VIRTUAL_CHANNEL 5     // Humidité ambiente
#define VIRTUAL_CHANNEL 6     // Température ambiente
#define VIRTUAL_CHANNEL 7     // Litres received
#define VIRTUAL_CHANNEL 8     // Temperature du sol

/*********************************Donnees********************************************/
// Temps entre les prises de donnees (en secondes)
#define DELAIS_PRISE_DONNEES 10 // Secondes
unsigned long lastMillis = 0;   // Dernière prise de mesure

// Humidite du sol
int pinHumiditeSol = A0;      // Senseur d'humidite analog
int pinVccSondeHum = D8;      // D2 pour alimenter la sonde lors de la prise de mesures 
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

// Sonde temperature sol
#include <OneWire.h>
#include <DallasTemperature.h>
int tempSolPin = D4;
OneWire oneWire(tempSolPin); //Setup the onewire instance to communicate with all OneWire devices
DallasTemperature sondesTempSol(&oneWire); // Pass the OneWire reference to DallasTemperature

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

  sondesTempSol.begin();
  delay(100);
     
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

      Cayenne.virtualWrite(98,FW_VERSION);                                         // Send the version number to Cayenne
   
      // Si le nombre de révolutions n'a pas changé depuis la dernière mesure, envoyer à Cayenne et reset tout
      if (lastRevFlowSensor == revFlowSensor && revFlowSensor != 0) {
        float litresFloat = (float)litres;
        float temp = (float)revFlowSensor;
        litresFloat += temp / REV_TO_LITRES;
        Serial.println(litres);
        Serial.println(revFlowSensor);
        Serial.println(litresFloat);
        Cayenne.virtualWrite(7, litresFloat);
        litres = 0;
        revFlowSensor = 0;
        lastRevFlowSensor = 0;
      } else
        lastRevFlowSensor = revFlowSensor;

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

      // Temperature du sol
      readAndSendTempSol();

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

/***************************************** Cayenne Ins **********************************************/
// Check for updates on web server
CAYENNE_IN(0)
{
  checkForUpdates();
}

/***************************************** Flow sensor ****************************************************/

//Interrupt function, so that the counting of pulse “rise ups” dont interfere with the rest of the code  (attachInterrupt)
void pin_ISR() {   
    revFlowSensor++;
}

/***************************************** Temp Sol ****************************************************/
void readAndSendTempSol() {
  // Lire temperature du sol
  sondesTempSol.requestTemperatures();
  // This command writes the temperature in Celsius to the Virtual Pin.
  float filtre = sondesTempSol.getTempCByIndex(0);
  if (filtre >= 0)
     Cayenne.celsiusWrite(8, filtre);
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
