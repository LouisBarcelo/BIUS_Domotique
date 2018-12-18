// Faire couler 1 L dedans pi voir la valeur finale de revFlowSensor

#include <Arduino.h>

// Variable init
const int  flowSensorPin = D5; // variable for D2 pin
int revFlowSensor = 0;   // variable to store the “rise ups” from the flowmeter pulses
int litres = 0;


void setup() {
   // Serial Comunication init
   Serial.begin(9600);
   delay(10);

   // Initialization of the variable “flowSensorPin” as INPUT (D2 pin)
   pinMode(flowSensorPin, INPUT);
    
  // Attach an interrupt to the ISR vector
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pin_ISR, RISING);
}


void loop() {
  // revFlowSensor is the number of revolutions
  Serial.println(revFlowSensor);
  // Currently found 535 to be 1 L (very crude test, to be redone)
  if(revFlowSensor > 535 )
  {
      // If enough revolutions have passed, increase the litre count and reset the revolutions
      litres++;
      Serial.println();
      Serial.print("litres: ");
      Serial.print(litres);

      revFlowSensor = 0;

  }//stop counting

  delay(500);
}
