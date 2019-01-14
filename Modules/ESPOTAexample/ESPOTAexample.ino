/************************************** OTA *****************************************/
const int FW_VERSION = 2;                                                        // Version number, don't forget to update this on changes
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// Note the raw.githubuserconent, this allows us to access the contents at the url, not the webpage itself
const char* fwURLBase = "https://raw.githubusercontent.com/BIUS-USherbrooke/BIUS_Domotique/master/Modules/ESPOTAexample/ESPOTAexample"; // IP adress to the subfolder containing the binary and version number for this specific device

/************************************** CAYENNE *****************************************/
#define CAYENNE_PRINT Serial               
#include <CayenneMQTTESP8266.h>

// If too many errors when connecting, deep sleep (which will restart the esp)
// PIN D0 OF THE NODEMCU MUST BE CONNECTed TO THE RST PIN
int failedConnections = 0;
#define SLEEP_TIME 20e6

// WiFi network info.
char ssid[] = "VIDEOTRON1096";
char wifiPassword[] = "laurianneadesbellesfesses16";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard. Specific to this device
char username[] = "75b6d920-cb5b-11e8-8a08-61f9e5c4f140";
char password[] = "13a3664856850223f1994846cd5aa6d668226da9";
char clientID[] = "67ab96c0-0eec-11e9-a08c-c5a286f8c00d";

// Channels cayenne
//#define VIRTUAL_CHANNEL 1 // EXAMPLE

// Always keep these two channels for cayenne, useful for pushing automatic updates (instead of waiting for timing window)
#define VIRTUAL_CHANNEL 98                                                       // Version number
#define VIRTUAL_CHANNEL 0                                                       // Channel to force an OTA update


void setup() {
  Serial.begin(9600);                                                            // Start Serial for debugging purposes
  delay(10);
  
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);               // Start cayenne
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {                                           // If connected to internet, proceed normally
    Cayenne.virtualWrite(98,FW_VERSION);                                         // Send the version number to Cayenne
    Cayenne.loop();                                                              // Proceed with cayenne.loop()
    //checkForUpdates();
    
    delay(1000);  // For testing this file.
  } else {
    failedConnections++;
    // If we failed the connection more than ten times, deep sleep and retry another time
    if (failedConnections > 10) {
      ESP.deepSleep(SLEEP_TIME);                                                // PIN D0 OF THE NODEMCU MUST BE CONNECT TO THE RST PIN
    } else {
      // Sinon, delay 1000
      delay(1000);
    }
  }

}

// Check for updates on web server
CAYENNE_IN(0)
{
  checkForUpdates();
}

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
