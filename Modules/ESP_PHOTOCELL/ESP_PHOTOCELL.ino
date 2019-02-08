/************************************** OTA *****************************************/
const int FW_VERSION = 1;                                                        // Version number, don't forget to update this on changes
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// Note the raw.githubuserconent, this allows us to access the contents at the url, not the webpage itself
const char* fwURLBase = "https://raw.githubusercontent.com/BIUS-USherbrooke/BIUS_Domotique/ESP_PHOTOCELL/Modules/ESP_PHOTOCELL/ESP_PHOTOCELL"; // IP adress to the subfolder containing the binary and version number for this specific device

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
char clientID[] = "5564eba0-2bd0-11e9-809d-0f8fe4c30267";

// Channels cayenne
#define VIRTUAL_CHANNEL 98                                                       // Version number
#define VIRTUAL_CHANNEL 0                                                       // Channel to force an OTA update

#define VIRTUAL_CHANNEL 1                                                       // Channel to force an OTA update
/*********************************Donnees********************************************/
// Temps entre les prises de donnees (en secondes)
#define DELAIS_PRISE_DONNEES 10 // Secondes
unsigned long lastMillis = 0;   // Dernière prise de mesure

int photoPin = A0;              // pin photocell

void setup() {
  Serial.begin(9600);
  delay(10);
 
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Cayenne.loop();

    // S'il y a eu assez de temps, republier les données
    if (millis() - lastMillis > DELAIS_PRISE_DONNEES*1000) {
      lastMillis = millis();

      Cayenne.virtualWrite(98,FW_VERSION);                                         // Send the version number to Cayenne

      Cayenne.virtualWrite(1,analogRead(photoPin));      
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
