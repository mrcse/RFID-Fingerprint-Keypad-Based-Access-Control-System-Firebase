#include <EEPROM.h>     // We are going to read and write Tag's UIDs from/to EEPROM
#include <MFRC522.h>    //RFID Library
#include <FPM.h>        //Finger Print Library
#include <LiquidCrystal_I2C.h> //16x2 LCD Display
#include <SoftwareSerial.h>
#include <Keypad.h>
#include <SPI.h>       //Serial Peripheral interface    To communicate with SPI devices
//#include <Wire.h>      // Create instances

MFRC522 mfrc522(53, 5); // MFRC522 mfrc522(SS_PIN, RST_PIN)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16 chars and 2 line display
SoftwareSerial fserial(10, 11); // Finger Print Sensor FM2662(greenWire, whiteWire)/(RXD, TXD)

// Set Pins for led's, servo, buzzer and wipe button
constexpr uint8_t greenLed = 7;
constexpr uint8_t blueLed = 8;
constexpr uint8_t redLed = 6;
constexpr uint8_t BuzzerPin = 4;
constexpr uint8_t wipeB = 23;     // Button pin for WipeMode

boolean match = false;          // initialize card match to false
boolean programMode = false;  // initialize programming mode to false
boolean replaceMaster = false;

uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader
int fid=0;
String ID="";
char id[8];
char jdart;             //finger print match test
byte storedCard[4];   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM
char storedPass[4]; // Variable to get password from EEPROM
char password[4];   // Variable to store users password
char masterPass[4]; // Variable to store master password
boolean RFIDMode = true; // boolean to change modes
boolean NormalMode = true; // boolean to change modes
char key_pressed = 0; // Variable to store incoming keys
uint8_t i = 0;  // Variable used for counter
// defining how many rows and columns our keypad have
const byte rows = 4;
const byte columns = 4;
// Keypad pin map
char hexaKeys[rows][columns] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
// Initializing pins for keypad
byte row_pins[rows] = {28, 30, 32, 34};
byte column_pins[columns] = {29, 31, 33, 35};
// Create instance for keypad
Keypad newKey = Keypad( makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);

FPM finger(&fserial);    //Finger Print Object
FPM_System_Params params; //To make easy

///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  
  //Arduino Pin Configuration
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(" Control System");
  lcd.setCursor(0,0);
  lcd.print("RFID & FingrPrnt");
  delay(5000);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(wipeB, INPUT);   // Wipe data

  // Make sure leds are off
  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, LOW);
  digitalWrite(blueLed, LOW);
  
  //Protocol Configuration
  
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware
  Serial.begin(115200);
          //FINGER PRINT INITIALIZER
  //########################################################################################################3
  fserial.begin(57600);
  
    if (finger.begin()) {
        finger.readParams(&params);
        ////////Serial.println("Found fingerprint sensor!");
        ////////Serial.print("Capacity: "); ////////Serial.println(params.capacity);
        ////////Serial.print("Packet length: "); ////////Serial.println(FPM::packet_lengths[params.packet_len]);
    } else {
        ////////Serial.println("Did not find fingerprint sensor :(");
        while (1) yield();
    }

    /* just to find out if your sensor supports the handshake command */
    if (finger.handshake()) {
        ////////Serial.println("Handshake command is supported.");
    }
    else {
        ////////Serial.println("Handshake command not supported.");
    }


  
  //If you set Antenna Gain to Max it will increase reading distance
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details
  //Wipe Code - If the Button (wipeB) Pressed while setup run (powered on) it wipes EEPROM
  //Below code for Wipe buton
  
  //########################################################################################################################################################################
 
  if (digitalRead(wipeB) == LOW) {  // when button pressed pin should get low, button connected to ground
    digitalWrite(redLed, HIGH); // Red Led stays on to inform user we are going to wipe
    lcd.setCursor(0, 0);
    lcd.print("Button Pressed");
    digitalWrite(BuzzerPin, HIGH);
    delay(1000);
    digitalWrite(BuzzerPin, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("This will remove");
    lcd.setCursor(0, 1);
    lcd.print("all records");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You have 10 ");
    lcd.setCursor(0, 1);
    lcd.print("secs to Cancel");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unpres to cancel");
    lcd.setCursor(0, 1);
    lcd.print("Counting: ");
    bool buttonState = monitorWipeButton(10000); // Give user enough time to cancel operation
    if (buttonState == true && digitalRead(wipeB) == LOW) {    // If button still be pressed, wipe EEPROM
      lcd.clear();
      lcd.print("Wiping EEPROM...");
      for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) {    //Loop end of EEPROM address
        if (EEPROM.read(x) == 0) {              //If EEPROM address 0
          // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
        }
        else {
          EEPROM.write(x, 0);       // if not write 0 to clear, it takes 3.3mS
        }
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wiping Done");
      empty_database();                  //wipe Finger print database
      // visualize a successful wipe
      cycleLeds();
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wiping Cancelled"); // Show some feedback that the wipe button did not pressed for 10 seconds
      digitalWrite(redLed, LOW);
    }
  }

  //##############################################################################################################################################################################
  
  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine the Master Card
  // You can keep other EEPROM records just write other than 143 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '143'
  if (EEPROM.read(1) != 143) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Master Card ");
    lcd.setCursor(0, 1);
    lcd.print("Defined");
    delay(2000);
    lcd.setCursor(0, 0);
    lcd.print("Scan A Tag to ");
    lcd.setCursor(0, 1);
    lcd.print("Define as Master");
    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
      // Visualize Master Card need to be defined
      digitalWrite(blueLed, HIGH);
      digitalWrite(BuzzerPin, HIGH);
      delay(100);
      digitalWrite(BuzzerPin, LOW);
      digitalWrite(blueLed, LOW);
      delay(100);
    }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( uint8_t j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned Tag's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("   Master Tag");
    lcd.setCursor(0, 1);
    lcd.print("    Defined!");
    delay(1000);
    storePassword(6); // Store password for master tag. 6 is the position in the EEPROM
  }
  for ( uint8_t i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
    masterPass[i] = EEPROM.read(6 + i);
  }
  ShowOnLCD();    // Print on the LCD
  cycleLeds();    // Everything ready lets give user some feedback by cycling leds
  
}

//////////////////////////////////////////////////////////////////////////////// Main Loop ///////////////////////////////////////////////////////////////////////////////////////////

void loop () {
  Repeat:
  jdart=0;   //finger print match test
 //###############################################################################################################################################################################
  
  // System will first check the mode
  if (NormalMode == false) {  // If the RFID Mode is false then it will only look for master tag and messages
    // Function to get tag's UID
    getID();
    if ( isMaster(readCard) ) { // If Scanned tag is master tag
      NormalMode = true; // Go back to RFID mode
      RFIDMode == true;
      cycleLeds();
    }
  }
  else if (NormalMode == true && RFIDMode == true) { // If RFID Mode is true then it will look for all tags and messages
    do {
      if (NormalMode == false) {
        break;
      }
      successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
      if (programMode) {
        cycleLeds();              // Program Mode cycles through Red Green Blue waiting to read a new card
      }
      else {
        normalModeOn();     // Normal mode, blue Power LED is on, all others are off
      }
    }
    while (!successRead );   //the program will not go further while you are not getting a successful read
    if (programMode && RFIDMode == true) {
      if ( isMaster(readCard) ) { //When in program mode check First If master card scanned again to exit program mode
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Exiting Program Mode");
        digitalWrite(BuzzerPin, HIGH);
        delay(1000);
        digitalWrite(BuzzerPin, LOW);
        ShowOnLCD();
        programMode = false;
        return;
      }
      else {
        if ( findID(readCard) ) { // If scanned card is known delete it
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Already there");
          deleteID(readCard);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Tag to ADD/REM");
          lcd.setCursor(0, 1);
          lcd.print("Master to Exit");
        }
        else {                    // If scanned card is not known add it
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("New Tag....");
          delay(400);
          lcd.setCursor(0, 1);
          lcd.print("Adding");
          delay(200);
          writeID(readCard);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Scan to ADD/REM");
          lcd.setCursor(0, 1);
          lcd.print("Master to Exit");
        }
      }
    }
    else {
      if ( isMaster(readCard)) {    // If scanned card's ID matches Master Card's ID - enter program mode
        programMode = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   Scan Your");
        lcd.setCursor(0, 1);
        lcd.print("     Finger");
        search_database();
        if(jdart=='n'){
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Finger Print is");
          lcd.setCursor(0,1);
          lcd.print("  Compulsory");
          delay(300);
          programMode = false;
          goto Repeat;
          }
          else{
            matchpass();
        if (programMode == true) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Program Mode");
          uint8_t count = EEPROM.read(0);   // Read the first Byte of EEPROM that stores the number of ID's in EEPROM
          lcd.setCursor(0, 1);
          lcd.print("I have ");
          lcd.print(count);
          lcd.print(" records");
          digitalWrite(BuzzerPin, HIGH);
          delay(2500);
          digitalWrite(BuzzerPin, LOW);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Scan a Tag to ");
          lcd.setCursor(0, 1);
          lcd.print("ADD/REMOVE");
        }
      }
      }
      else {
        if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
          granted();
          RFIDMode = false;  // Make RFID mode false
          ShowOnLCD();
        }
        else {      // If not, show that the Access is denied
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Access Denied");
          denied();
          ShowOnLCD();
        }
      }
    }
  }
  // If RFID mode is false, get keys
  else if (NormalMode == true && RFIDMode == false) {
    int x=1;
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("   Scan Your");
    lcd.setCursor(0, 1);
    lcd.print("     Finger");
    search_database();
    if(jdart=='n'){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Finger Print is");
      lcd.setCursor(0,1);
      lcd.print("  Compulsory");
      delay(300);
      denied();
    
    }
    else{
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Enter Your ID:");
      lcd.setCursor(0,1);
    while(x){
    
    key_pressed = newKey.getKey();  //Store new key
    if (key_pressed)
    {
      password[i++] = key_pressed;  // Storing in password variable
      lcd.print("*");
    }
    if (i == 4) // If 4 keys are completed
    {
      delay(200);
      if (!(strncmp(password, storedPass, 4)))  // If password is matched
      {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("ID Accepted");
        lcd.setCursor(0, 1);
        lcd.print("Access Granted");
        granted();
        x=0;
        RFIDMode = true;   // Make RFID mode true
        ShowOnLCD();
        i = 0;
      }
      else   // If password is not matched
      {
        lcd.clear();
        lcd.print("Wrong ID");
        lcd.setCursor(0, 1);
        lcd.print("Access Denied");
        denied();
        x=0;
        RFIDMode = true;   // Make RFID mode true
        ShowOnLCD();
        i = 0;
      }
      
    }
  }
  }
 }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
String printHex(byte *buffer, byte bufferSize) {
  String string="";
  for (byte i = 0; i < bufferSize; i++) {
    string +=(buffer[i] < 0x10 ? "0" : "");
    string +=String(buffer[i],HEX);
  }
  //////////Serial.println(string);
  return string;
}

/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted () {
  digitalWrite(blueLed, LOW);   // Turn off blue LED
  digitalWrite(redLed, LOW);  // Turn off red LED
  digitalWrite(greenLed, HIGH);   // Turn on green LED
  if (RFIDMode == false) {
    for(int i=0;i<8;i++){
      id[i]=ID[i];
    }
    Serial.write(id,8);
    //////////Serial.print(id);
    delay(1000);
  }
  delay(1000);
  digitalWrite(blueLed, HIGH);
  digitalWrite(greenLed, LOW);
}
///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  digitalWrite(redLed, HIGH);   // Turn on red LED
  digitalWrite(BuzzerPin, HIGH);
  delay(1000);
  digitalWrite(BuzzerPin, LOW);
  digitalWrite(blueLed, HIGH);
  digitalWrite(redLed, LOW);
}
///////////////////////////////////////// Get Tag's UID ///////////////////////////////////
uint8_t getID() {
  // Getting ready for Reading Tags
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new Tag placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a Tag placed get Serial and continue
    return 0;
  }
  // There are Mifare Tags which have 4 byte or 7 byte UID care if you use 7 byte Tag
  // I think we should assume every Tag as they have 4 byte UID
 
  // Until we support 7 byte Tags
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
  }
  ID=printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
  mfrc522.PICC_HaltA(); // Stop reading
  
  return 1;
}
/////////////////////// Check if RFID Reader is correctly initialized or not /////////////////////
void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    lcd.setCursor(0, 0);
    lcd.print("Communication Failure");
    lcd.setCursor(0, 1);
    lcd.print("Check Connections");
    digitalWrite(BuzzerPin, HIGH);
    delay(2000);
    // Visualize system is halted
    digitalWrite(greenLed, LOW);  // Make sure green LED is off
    digitalWrite(blueLed, LOW);   // Make sure blue LED is off
    digitalWrite(redLed, HIGH);   // Turn on red LED
    digitalWrite(BuzzerPin, LOW);
    while (true); // do not go further
  }
}
///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////
void cycleLeds() {
  digitalWrite(redLed, LOW);  // Make sure red LED is off
  digitalWrite(greenLed, HIGH);   // Make sure green LED is on
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  delay(200);
  digitalWrite(redLed, LOW);  // Make sure red LED is off
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, HIGH);  // Make sure blue LED is on
  delay(200);
  digitalWrite(redLed, HIGH);   // Make sure red LED is on
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  delay(200);
  digitalWrite(redLed, LOW);   // Make sure red LED is on
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, HIGH);   // Make sure blue LED is off
}
//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {
  digitalWrite(blueLed, HIGH);  // Blue LED ON and ready to read card
  digitalWrite(redLed, LOW);  // Make sure Red LED is off
  digitalWrite(greenLed, LOW);  // Make sure Green LED is off
}
//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( uint8_t number ) {
  uint8_t start = (number * 8) + 2; // Figure out starting position
  for ( uint8_t i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
    storedPass[i] = EEPROM.read(start + i + 4);
  }
}
///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    uint8_t num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t start = ( num * 8 ) + 10; // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    storePassword(start + 4);
    BlinkLEDS(greenLed);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("New Tag....");
    delay(100);
    lcd.setCursor(0, 1);
    lcd.print("Added");
    delay(1000);
  }
  else {
    BlinkLEDS(redLed);
    lcd.setCursor(0, 0);
    lcd.print("Failed!");
    lcd.setCursor(0, 1);
    lcd.print("wrong ID or bad EEPROM");
    delay(2000);
  }
}
///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  search_database();
    int temp=jdart;
    if(jdart=='n'){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Finger Print is");
      lcd.setCursor(0,1);
      lcd.print("  Compulsory");
      delay(500);
    
    }
    else
    {
      deleteFingerprint(temp);
      delay(200);
      BlinkLEDS(blueLed);
      lcd.setCursor(0, 1);
      lcd.print(" Removed");
      delay(1000);
      if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    BlinkLEDS(redLed);      // If not
    lcd.setCursor(0, 0);
    lcd.print("Failed!");
    lcd.setCursor(0, 1);
    lcd.print("wrong ID or bad EEPROM");
    delay(2000);
  }
  else {
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 8) + 10;
    looping = ((num - slot)) * 4;
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    
  }
    }
  
}
///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != 0 )      // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}
///////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
      break;          // Stop looking we found it
    }
  }
}
///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
      break;  // Stop looking we found it
    }
    else {    // If not, return false
    }
  }
  return false;
}
///////////////////////////////////////// Blink LED's For Indication   ///////////////////////////////////
void BlinkLEDS(int led) {
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  digitalWrite(redLed, LOW);  // Make sure red LED is off
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(led, HIGH);  // Make sure blue LED is on
  digitalWrite(BuzzerPin, LOW);
  delay(200);
  digitalWrite(led, LOW);   // Make sure blue LED is off
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(led, HIGH);  // Make sure blue LED is on
  digitalWrite(BuzzerPin, LOW);
  delay(200);
  digitalWrite(led, LOW);   // Make sure blue LED is off
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(led, HIGH);  // Make sure blue LED is on
  digitalWrite(BuzzerPin, LOW);
  delay(200);
}
////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) ) {
    return true;
  }
  else
    return false;
}
/////////////////// Counter to check in reset/wipe button is pressed or not   /////////////////////
bool monitorWipeButton(uint32_t interval) {
  unsigned long currentMillis = millis(); // grab current time
  while (millis() - currentMillis < interval)  {
    int timeSpent = (millis() - currentMillis) / 1000;
    ////////Serial.println(timeSpent);
    lcd.setCursor(10, 1);
    lcd.print(timeSpent);
    // check on every half a second
    if (((uint32_t)millis() % 10) == 0) {
      if (digitalRead(wipeB) != LOW) {
        return false;
      }
    }
  }
  return true;
}
////////////////////// Print Info on LCD   ///////////////////////////////////
void ShowOnLCD() {
  if (RFIDMode == false) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Enter Your ID");
    lcd.setCursor(0, 1);
  }
  else if (RFIDMode == true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Access Control");
    lcd.setCursor(0, 1);
    lcd.print("   Scan a Tag");
  }
}
////////////////////// Store Passwords in EEPROOM   ///////////////////////////////////
void storePassword(int j) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("   Scan Your");
    lcd.setCursor(0, 1);
    lcd.print("     Finger");
    
    if (get_free_id(&fid))               ///searching free slot
        enroll_finger(fid);             ///scanning finger
    else
        {
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print(" Sorry There is");
         lcd.setCursor(0, 1);
         lcd.print(" No Free Slot!!");  
        }
  int k = j + 4;
  BlinkLEDS(blueLed);
  lcd.clear();
  lcd.print("Enter New ID :");
  lcd.setCursor(0, 1);
  while (j < k)
  {
    char key = newKey.getKey();
    if (key)
    {
      lcd.print("*");
      EEPROM.write(j, key);
      j++;
    }
  }
}
void matchpass() {
  RFIDMode = false;
  ShowOnLCD();
  int n = 0;
  while (n < 1) {
    key_pressed = newKey.getKey();  //Store new key
    if (key_pressed)
    {
      password[i++] = key_pressed;  // Storing in password variable
      lcd.print("*");
    }
    if (i == 4) // If 4 keys are completed
    {
      delay(200);
      if (!(strncmp(password, masterPass, 4)))  // If password is matched
      {
        RFIDMode = true;
        programMode = true;
        i = 0;
      }
      else   // If password is not matched
      {
        lcd.clear();
        lcd.print("    Wrong ID");
        digitalWrite(BuzzerPin, HIGH);
        delay(1000);
        digitalWrite(BuzzerPin, LOW);
        programMode = false;
        RFIDMode = true;
        ShowOnLCD();
        i = 0;
      }
      n = 4;
    
  }
 }
}
bool get_free_id(int16_t * fid) {
    int16_t p = -1;
    for (int page = 0; page < (params.capacity / FPM_TEMPLATES_PER_PAGE) + 1; page++) {
        p = finger.getFreeIndex(page, fid);
        switch (p) {
            case FPM_OK:
                if (*fid != FPM_NOFREEINDEX) {
                    ////////Serial.print("Free slot at ID ");
                    ////////Serial.println(*fid);
                    return true;
                }
                break;
            case FPM_PACKETRECIEVEERR:
                ////////Serial.println("Communication error!");
                return false;
            case FPM_TIMEOUT:
                ////////Serial.println("Timeout!");
                return false;
            case FPM_READ_ERROR:
                ////////Serial.println("Got wrong PID or length!");
                return false;
            default:
                ////////Serial.println("Unknown error!");
                return false;
        }
        yield();
    }
    
    //////////Serial.println("No free slots!");
    return false;
}

int16_t enroll_finger(int16_t fid) {
    int16_t p = -1;
    ////////Serial.println("Waiting for valid finger to enroll");
    while (p != FPM_OK) {
        p = finger.getImage();
        switch (p) {
            case FPM_OK:
                ////////Serial.println("Image taken");
                break;
            case FPM_NOFINGER:
                ////////Serial.println(".");
                break;
            case FPM_PACKETRECIEVEERR:
                ////////Serial.println("Communication error");
                break;
            case FPM_IMAGEFAIL:
                ////////Serial.println("Imaging error");
                break;
            case FPM_TIMEOUT:
                ////////Serial.println("Timeout!");
                break;
            case FPM_READ_ERROR:
                ////////Serial.println("Got wrong PID or length!");
                break;
            default:
                ////////Serial.println("Unknown error");
                break;
        }
        yield();
    }
    // OK success!

    p = finger.image2Tz(1);
    switch (p) {
        case FPM_OK:
            ////////Serial.println("Image converted");
            break;
        case FPM_IMAGEMESS:
            ////////Serial.println("Image too messy");
            return p;
        case FPM_PACKETRECIEVEERR:
            ////////Serial.println("Communication error");
            return p;
        case FPM_FEATUREFAIL:
            ////////Serial.println("Could not find fingerprint features");
            return p;
        case FPM_INVALIDIMAGE:
            ////////Serial.println("Could not find fingerprint features");
            return p;
        case FPM_TIMEOUT:
            ////////Serial.println("Timeout!");
            return p;
        case FPM_READ_ERROR:
            ////////Serial.println("Got wrong PID or length!");
            return p;
        default:
            ////////Serial.println("Unknown error");
            return p;
    }
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(" Remove  Finger");
    ////////Serial.println("Remove finger");
    delay(500);
    p = 0;
    while (p != FPM_NOFINGER) {
        p = finger.getImage();
        yield();
    }

    p = -1;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("  Place Same ");
    lcd.setCursor(0,1);
    lcd.print("  Finger again ");
    
    ////////Serial.println("Place same finger again");
    while (p != FPM_OK) {
        p = finger.getImage();
        switch (p) {
            case FPM_OK:
                ////////Serial.println("Image taken");
                break;
            case FPM_NOFINGER:
                ////////Serial.print(".");
                break;
            case FPM_PACKETRECIEVEERR:
                ////////Serial.println("Communication error");
                break;
            case FPM_IMAGEFAIL:
                ////////Serial.println("Imaging error");
                break;
            case FPM_TIMEOUT:
                ////////Serial.println("Timeout!");
                break;
            case FPM_READ_ERROR:
                ////////Serial.println("Got wrong PID or length!");
                break;
            default:
                ////////Serial.println("Unknown error");
                break;
        }
        yield();
    }

    // OK success!

    p = finger.image2Tz(2);
    switch (p) {
        case FPM_OK:
            ////////Serial.println("Image converted");
            break;
        case FPM_IMAGEMESS:
            ////////Serial.println("Image too messy");
            return p;
        case FPM_PACKETRECIEVEERR:
            ////////Serial.println("Communication error");
            return p;
        case FPM_FEATUREFAIL:
            ////////Serial.println("Could not find fingerprint features");
            return p;
        case FPM_INVALIDIMAGE:
            ////////Serial.println("Could not find fingerprint features");
            return p;
        case FPM_TIMEOUT:
            ////////Serial.println("Timeout!");
            return false;
        case FPM_READ_ERROR:
            ////////Serial.println("Got wrong PID or length!");
            return false;
        default:
            ////////Serial.println("Unknown error");
            return p;
    }


    // OK converted!
    p = finger.createModel();
    if (p == FPM_OK) {
        ////////Serial.println("Prints matched!");
    } else if (p == FPM_PACKETRECIEVEERR) {
        ////////Serial.println("Communication error");
        return p;
    } else if (p == FPM_ENROLLMISMATCH) {
        ////////Serial.println("Fingerprints did not match");
        return p;
    } else if (p == FPM_TIMEOUT) {
        ////////Serial.println("Timeout!");
        return p;
    } else if (p == FPM_READ_ERROR) {
        ////////Serial.println("Got wrong PID or length!");
        return p;
    } else {
        ////////Serial.println("Unknown error");
        return p;
    }

    ////////Serial.print("ID "); ////////Serial.println(fid);
    p = finger.storeModel(fid);
    if (p == FPM_OK) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("    Done....");
        ////////Serial.println("Stored!");
        return 0;
    } else if (p == FPM_PACKETRECIEVEERR) {
        ////////Serial.println("Communication error");
        return p;
    } else if (p == FPM_BADLOCATION) {
        ////////Serial.println("Could not store in that location");
        return p;
    } else if (p == FPM_FLASHERR) {
        ////////Serial.println("Error writing to flash");
        return p;
    } else if (p == FPM_TIMEOUT) {
        ////////Serial.println("Timeout!");
        return p;
    } else if (p == FPM_READ_ERROR) {
        ////////Serial.println("Got wrong PID or length!");
        return p;
    } else {
        ////////Serial.println("Unknown error");
        return p;
    }
}
void empty_database(void) {
    int16_t p = finger.emptyDatabase();
    if (p == FPM_OK) {
        ////////Serial.println("FingerPrint Database empty!");
    }
    else if (p == FPM_PACKETRECIEVEERR) {
        ////////Serial.print("Communication error!");
    }
    else if (p == FPM_DBCLEARFAIL) {
        ////////Serial.println("Could not clear database!");
    } 
    else if (p == FPM_TIMEOUT) {
        ////////Serial.println("Timeout!");
    } 
    else if (p == FPM_READ_ERROR) {
        ////////Serial.println("Got wrong PID or length!");
    } 
    else {
        ////////Serial.println("Unknown error");
    }
}
int search_database(void) {
    int16_t p = -1;

    /* first get the finger image */
    ////////Serial.println("Waiting for valid finger");
    while (p != FPM_OK) {
        p = finger.getImage();
        switch (p) {
            case FPM_OK:
                ////////Serial.println("Image taken");
                break;
            case FPM_NOFINGER:
                ////////Serial.println(".");
                break;
            case FPM_PACKETRECIEVEERR:
                ////////Serial.println("Communication error");
                break;
            case FPM_IMAGEFAIL:
                ////////Serial.println("Imaging error");
                break;
            case FPM_TIMEOUT:
                ////////Serial.println("Timeout!");
                break;
            case FPM_READ_ERROR:
                ////////Serial.println("Got wrong PID or length!");
                break;
            default:
                ////////Serial.println("Unknown error");
                break;
        }
        yield();
    }

    /* convert it */
    p = finger.image2Tz();
    switch (p) {
        case FPM_OK:
            ////////Serial.println("Image converted");
            break;
        case FPM_IMAGEMESS:
            ////////Serial.println("Image too messy");
            return p;
        case FPM_PACKETRECIEVEERR:
            ////////Serial.println("Communication error");
            return p;
        case FPM_FEATUREFAIL:
            ////////Serial.println("Could not find fingerprint features");
            return p;
        case FPM_INVALIDIMAGE:
            ////////Serial.println("Could not find fingerprint features");
            return p;
        case FPM_TIMEOUT:
            ////////Serial.println("Timeout!");
            return p;
        case FPM_READ_ERROR:
            ////////Serial.println("Got wrong PID or length!");
            return p;
        default:
            ////////Serial.println("Unknown error");
            return p;
    }

    ////////Serial.println("Remove finger");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(" Remove  Finger");
    ////////Serial.println("Remove finger");
    delay(300);
    p = 0;
    while (p != FPM_NOFINGER) {
        p = finger.getImage();
        yield();
    }
    ////////Serial.println();

    /* search the database for the converted print */
    uint16_t fid, score;
    p = finger.searchDatabase(&fid, &score);
    if (p == FPM_OK) {
        ////////Serial.println("Found a print match!");
    } else if (p == FPM_PACKETRECIEVEERR) {
        ////////Serial.println("Communication error");
        return p;
    } else if (p == FPM_NOTFOUND) {
        ////////Serial.println("Did not find a match");
        jdart='n';
        return p;
    } else if (p == FPM_TIMEOUT) {
        ////////Serial.println("Timeout!");
        return p;
    } else if (p == FPM_READ_ERROR) {
        ////////Serial.println("Got wrong PID or length!");
        return p;
    } else {
        ////////Serial.println("Unknown error");
        return p;
    }

    // found a match!
    return fid;
}
int deleteFingerprint(int fid) {
    int p = -1;

    p = finger.deleteModel(fid);

    if (p == FPM_OK) {
        ////////Serial.println("Deleted!");
    } else if (p == FPM_PACKETRECIEVEERR) {
        ////////Serial.println("Communication error");
        return p;
    } else if (p == FPM_BADLOCATION) {
        ////////Serial.println("Could not delete in that location");
        return p;
    } else if (p == FPM_FLASHERR) {
        ////////Serial.println("Error writing to flash");
        return p;
    } else if (p == FPM_TIMEOUT) {
        //////Serial.println("Timeout!");
        return p;
    } else if (p == FPM_READ_ERROR) {
        //////Serial.println("Got wrong PID or length!");
        return p;
    } else {
        //////Serial.print("Unknown error: 0x"); //////Serial.println(p, HEX);
        return p;
    }
}