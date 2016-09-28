/* Secret Knock Trinket
   Code for running a secret knock lock on the Adafruit Trinket.
   
   Version 13.10.31  Built with Arduino IDE 1.0.5
   
   By Steve Hoefer http://grathio.com
   Licensed under Creative Commons Attribution-Noncommercial-Share Alike 3.0
   http://creativecommons.org/licenses/by-nc-sa/3.0/us/
   (In short: Do what you want, as long as you credit me, don't relicense it, and don't sell it or use it in anything you sell without contacting me.)
   
   ------Wiring------
   Pin 0: Record A New Knock button.
   Pin 1: (uses the built in LED)
   Pin 2 (Analog 1): A piezo element for beeping and sensing knocks.
   Pin 3: Connects to a transistor that opens a solenoid lock when HIGH.
*/

#include <EEPROM.h>
const byte eepromValid = 123;    // If the first byte in eeprom is this then the data is valid.

/*Pin definitions*/
const int programButton = 0;   // Record A New Knock button.
const int ledPin = 1;          // The built in LED
const int knockSensor = 1;     // (Analog 1) for using the piezo as an input device. (aka knock sensor)
const int audioOut = 2;        // (Digial 2) for using the peizo as an output device. (Thing that goes beep.)
const int lockPin = 3;         // The pin that activates the solenoid lock.

/*Tuning constants. Changing the values below changes the behavior of the device.*/
int threshold = 3;                 // Minimum signal from the piezo to register as a knock. Higher = less sensitive. Typical values 1 - 10
const int rejectValue = 25;        // If an individual knock is off by this percentage of a knock we don't unlock. Typical values 10-30
const int averageRejectValue = 15; // If the average timing of all the knocks is off by this percent we don't unlock. Typical values 5-20
const int knockFadeTime = 150;     // Milliseconds we allow a knock to fade before we listen for another one. (Debounce timer.)
const int lockOperateTime = 2500;  // Milliseconds that we operate the lock solenoid latch before releasing it.
const int maximumKnocks = 20;      // Maximum number of knocks to listen for.
const int knockComplete = 1200;    // Longest time to wait for a knock before we assume that it's finished. (milliseconds)

byte secretCode[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial setup: "Shave and a Hair Cut, two bits."
int knockReadings[maximumKnocks];    // When someone knocks this array fills with the delays between knocks.
int knockSensorValue = 0;            // Last reading of the knock sensor.
boolean programModeActive = false;   // True if we're trying to program a new knock.

void setup() {
  pinMode(ledPin, OUTPUT); 
  pinMode(lockPin, OUTPUT);
  readSecretKnock();   // Load the secret knock (if any) from EEPROM.
  doorUnlock(500);     // Unlock the door for a bit when we power up. For system check and to allow a way in if the key is forgotten.
  delay(500);          // This delay is here because the solenoid lock returning to place can otherwise trigger and inadvertent knock.
}

void loop() {
  // Listen for any knock at all.
  knockSensorValue = analogRead(knockSensor);
  
  if (digitalRead(programButton) == HIGH){  // is the program button pressed?
    delay(100);   // Cheap debounce.
    if (digitalRead(programButton) == HIGH){ 
      if (programModeActive == false){     // If we're not in programming mode, turn it on.
        programModeActive = true;          // Remember we're in programming mode.
        digitalWrite(ledPin, HIGH);        // Turn on the red light too so the user knows we're programming.
        chirp(500, 1500);                  // And play a tone in case the user can't see the LED.
        chirp(500, 1000);
      } else {                             // If we are in programing mode, turn it off.
        programModeActive = false;
        digitalWrite(ledPin, LOW);
        chirp(500, 1000);                  // Turn off the programming LED and play a sad note.
        chirp(500, 1500);
        delay(500);
      }
      while (digitalRead(programButton) == HIGH){
        delay(10);                         // Hang around until the button is released.
      } 
    }
    delay(250);   // Another cheap debounce. Longer because releasing the button can sometimes be sensed as a knock.
  }
  
  
  if (knockSensorValue >= threshold){
     if (programModeActive == true){  // Blink the LED when we sense a knock.
       digitalWrite(ledPin, LOW);
     } else {
       digitalWrite(ledPin, HIGH);
     }
     knockDelay();
     if (programModeActive == true){  // Un-blink the LED.
       digitalWrite(ledPin, HIGH);
     } else {
       digitalWrite(ledPin, LOW);
     }
     listenToSecretKnock();           // We have our first knock. Go and see what other knocks are in store...
  }
 
} 

// Records the timing of knocks.
void listenToSecretKnock(){
  int i = 0;
  // First reset the listening array.
  for (i=0; i < maximumKnocks; i++){
    knockReadings[i] = 0;
  }
  
  int currentKnockNumber = 0;               // Position counter for the array.
  int startTime = millis();                 // Reference for when this knock started.
  int now = millis();   

  do {                                      // Listen for the next knock or wait for it to timeout. 
    knockSensorValue = analogRead(knockSensor);
    if (knockSensorValue >= threshold){                   // Here's another knock. Save the time between knocks.
      now=millis();
      knockReadings[currentKnockNumber] = now - startTime;
      currentKnockNumber ++;                             
      startTime = now;          

       if (programModeActive==true){     // Blink the LED when we sense a knock.
         digitalWrite(ledPin, LOW);
       } else {
         digitalWrite(ledPin, HIGH);
       } 
       knockDelay();
       if (programModeActive == true){  // Un-blink the LED.
         digitalWrite(ledPin, HIGH);
       } else {
         digitalWrite(ledPin, LOW);
       }
    }

    now = millis();
  
    // Stop listening if there are too many knocks or there is too much time between knocks.
  } while ((now-startTime < knockComplete) && (currentKnockNumber < maximumKnocks));
  
  //we've got our knock recorded, lets see if it's valid
  if (programModeActive == false){           // Only do this if we're not recording a new knock.
    if (validateKnock() == true){
      doorUnlock(lockOperateTime); 
    } else {
      // knock is invalid. Blink the LED as a warning to others.
      for (i=0; i < 4; i++){          
        digitalWrite(ledPin, HIGH);
        delay(50);
        digitalWrite(ledPin, LOW);
        delay(50);
      }
    }
  } else { // If we're in programming mode we still validate the lock because it makes some numbers we need, we just don't do anything with the return.
    validateKnock();
  }
}


// Unlocks the door.
void doorUnlock(int delayTime){
  digitalWrite(ledPin, HIGH);
  digitalWrite(lockPin, HIGH);
  delay(delayTime);
  digitalWrite(lockPin, LOW);
  digitalWrite(ledPin, LOW);  
  delay(500);   // This delay is here because releasing the latch can cause a vibration that will be sensed as a knock.
}

// Checks to see if our knock matches the secret.
// Returns true if it's a good knock, false if it's not.
boolean validateKnock(){
  int i = 0;
 
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;               // We use this later to normalize the times.
  
  for (i=0;i<maximumKnocks;i++){
    if (knockReadings[i] > 0){
      currentKnockCount++;
    }
    if (secretCode[i] > 0){         
      secretKnockCount++;
    }
    
    if (knockReadings[i] > maxKnockInterval){   // Collect normalization data while we're looping.
      maxKnockInterval = knockReadings[i];
    }
  }
  
  // If we're recording a new knock, save the info and get out of here.
  if (programModeActive == true){
      for (i=0; i < maximumKnocks; i++){ // Normalize the time between knocks. (the longest time = 100)
        secretCode[i] = map(knockReadings[i], 0, maxKnockInterval, 0, 100); 
      }
      saveSecretKnock();                // save the result to EEPROM
      programModeActive = false;
      playbackKnock(maxKnockInterval);
      return false;
  }
  
  if (currentKnockCount != secretKnockCount){  // Easiest check first. If the number of knocks is wrong, don't unlock.
    return false;
  }
  
  /*  Now we compare the relative intervals of our knocks, not the absolute time between them.
      (ie: if you do the same pattern slow or fast it should still open the door.)
      This makes it less picky, which while making it less secure can also make it
      less of a pain to use if you're tempo is a little slow or fast. 
  */
  int totaltimeDifferences = 0;
  int timeDiff = 0;
  for (i=0; i < maximumKnocks; i++){    // Normalize the times
    knockReadings[i]= map(knockReadings[i], 0, maxKnockInterval, 0, 100);      
    timeDiff = abs(knockReadings[i] - secretCode[i]);
    if (timeDiff > rejectValue){        // Individual value too far out of whack. No access for this knock!
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences / secretKnockCount > averageRejectValue){
    return false; 
  }
  
  return true;
}


// reads the secret knock from EEPROM. (if any.)
void readSecretKnock(){
  byte reading;
  int i;
  reading = EEPROM.read(0);
  if (reading == eepromValid){    // only read EEPROM if the signature byte is correct.
    for (int i=0; i < maximumKnocks ;i++){
      secretCode[i] =  EEPROM.read(i+1);
    }
  }
}


//saves a new pattern too eeprom
void saveSecretKnock(){
  EEPROM.write(0, 0);  // clear out the signature. That way we know if we didn't finish the write successfully.
  for (int i=0; i < maximumKnocks; i++){
    EEPROM.write(i+1, secretCode[i]);
  }
  EEPROM.write(0, eepromValid);  // all good. Write the signature so we'll know it's all good.
}

// Plays back the pattern of the knock in blinks and beeps
void playbackKnock(int maxKnockInterval){
      digitalWrite(ledPin, LOW);
      delay(1000);
      digitalWrite(ledPin, HIGH);
      chirp(200, 1800);
      for (int i = 0; i < maximumKnocks ; i++){
        digitalWrite(ledPin, LOW);
        // only turn it on if there's a delay
        if (secretCode[i] > 0){                                   
          delay(map(secretCode[i], 0, 100, 0, maxKnockInterval)); // Expand the time back out to what it was. Roughly. 
          digitalWrite(ledPin, HIGH);
          chirp(200, 1800);
        }
      }
      digitalWrite(ledPin, LOW);
}

// Deals with the knock delay thingy.
void knockDelay(){
  int itterations = (knockFadeTime / 20);      // Wait for the peak to dissipate before listening to next one.
  for (int i=0; i < itterations; i++){
    delay(10);
    analogRead(knockSensor);                  // This is done in an attempt to defuse the analog sensor's capacitor that will give false readings on high impedance sensors.
    delay(10);
  } 
}

// Plays a non-musical tone on the piezo.
// playTime = milliseconds to play the tone
// delayTime = time in microseconds between ticks. (smaller=higher pitch tone.)
void chirp(int playTime, int delayTime){
  long loopTime = (playTime * 1000L) / delayTime;
  pinMode(audioOut, OUTPUT);
  for(int i=0; i < loopTime; i++){
    digitalWrite(audioOut, HIGH);
    delayMicroseconds(delayTime);
    digitalWrite(audioOut, LOW);
  }
  pinMode(audioOut, INPUT);
}