// Include required libraries
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <SPI.h>

// Create instances
SoftwareSerial SIM(15,14); // SoftwareSerial SIM800c(Tx, Rx)
MFRC522 mfrc522(53,16); // MFRC522 mfrc522(SS_PIN, RST_PIN)
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

// Initialize Pins for led's, servo and buzzer

constexpr uint8_t lockPin = 21;
constexpr uint8_t buzzerPin = 20;

char initial_password[4] = {'1', '2', '3', '4'};  // Variable to store initial password
String tagUID = "E2 C5 78 19";  // String to store UID of tag. Change it with your tag's UID
char password[4];   // Variable to store users password
boolean RFIDMode;
boolean PwMode;
boolean NormalMode = false; // boolean to change modes
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
byte row_pins[rows] = {7, 6, 5, 4};
byte column_pins[columns] = {3, 2, 1, 0};

// Create instance for keypad
Keypad keypad_key = Keypad( makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);

void setup() {
  // Arduino Pin configuration
  pinMode(buzzerPin, OUTPUT);
  pinMode(lockPin, OUTPUT);

  lcd.begin(16,2);   // LCD screen
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  SIM.begin(9600);   // Arduino communicates with SIM800C GSM shield at a baud rate of 9600

  // AT command to set SIM to SMS mode
  SIM.print("AT+CMGF=1\r");
  delay(100);
  // Set module to send SMS data to serial out upon receipt
  SIM.print("AT+CNMI=2,2,0,0,0\r");
  delay(100);
  lcd.clear(); // Clear LCD screen
    lcd.setCursor(0, 0);
      lcd.print("* -> RFID");
      lcd.setCursor(0, 1);
      lcd.print("D -> PIN");
}

void loop() {
  
  if (NormalMode == false) {
    receive_message();
    key_pressed = keypad_key.getKey();
    if(key_pressed == '*')
    {RFIDMode = true;
    NormalMode = true;}
    else if(key_pressed == 'D')
    {PwMode = true;
    NormalMode = true;}
  }

  else if (NormalMode == true) {
    // System will first look for mode
    if (RFIDMode == true) {
      // Function to receive message
      receive_message();

      lcd.setCursor(0, 0);
      lcd.print("   Door Lock");
      lcd.setCursor(0, 1);
      lcd.print(" Scan Your Tag ");

      // Look for new cards
      if ( ! mfrc522.PICC_IsNewCardPresent()) {
        return;
      }

      // Select one of the cards
      if ( ! mfrc522.PICC_ReadCardSerial()) {
        return;
      }

      //Reading from the card
      String tag = "";
      for (byte j = 0; j < mfrc522.uid.size; j++)
      {
        tag.concat(String(mfrc522.uid.uidByte[j] < 0x10 ? " 0" : " "));
        tag.concat(String(mfrc522.uid.uidByte[j], HEX));
      }
      tag.toUpperCase();

      //Checking the card
      if (tag.substring(1) == tagUID)
      {
        lcd.clear();
        lcd.print("Tag Matched");
        tone(buzzerPin,200);
        delay(300);
        noTone(buzzerPin);
        delay(100);
        tone(buzzerPin,250);
        delay(300);
        noTone(buzzerPin);
        unlock();
      }

      else
      {
        // If UID of tag is not matched.
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wrong Tag Shown");
        lcd.setCursor(0, 1);
        lcd.print("Access Denied");
        tone(buzzerPin,200);
        send_message("Someone Tried with the wrong tag \nType 'close' to halt the system.");
        delay(500);
        noTone(buzzerPin);
        lock();
      }
    }

    if (PwMode == true) {
      lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter PIN:");
        lcd.setCursor(0, 1);
      key_pressed = keypad_key.getKey(); // Storing keys
      if (key_pressed)
      {
        password[i++] = key_pressed; // Storing in password variable
        lcd.print(key_pressed);
        tone(buzzerPin,200);
        delay(50);
        noTone(buzzerPin);
      }
      if (i == 4) // If 4 keys are completed
      {
        delay(200);
        if (!(strncmp(password, initial_password, 4))) // If password is matched
        {
          i = 0;
          lcd.clear();
          lcd.print("Pass Accepted");
          unlock();
          i=0;
        }
        else    // If password is not matched
        {
          lcd.clear();
          lcd.print("Wrong Password");
          lcd.setCursor(0, 1);
        lcd.print("Access Denied");
          tone(buzzerPin,200);
        send_message("Someone Tried with the wrong PIN \nType 'close' to halt the system.");
        delay(500);
        noTone(buzzerPin);
        lock();
        }
      }
    }
  }
}

// Receiving the message
void receive_message()
{
  char incoming_char = 0; //Variable to save incoming SMS characters
  String incomingData;   // for storing incoming serial data
  
  if (SIM.available() > 0)
  {
    incomingData = SIM.readString(); // Get the incoming data.
    delay(10);
  }

  // if received command is to open the door
  if (incomingData.indexOf("open") >= 0)
  {
    unlock();
    lock();
  }

  // if received command is to halt the system
  if (incomingData.indexOf("close") >= 0)
  {
    digitalWrite(lockPin, LOW);
    lock();
    send_message("Closed");
  }
  incomingData = "";
}

// Function to send the message
void send_message(String message)
{
  SIM.println("AT+CMGF=1");    //Set the GSM Module in Text Mode
  delay(100);
  SIM.println("AT+CMGS=\"+917746052737\""); // Replace it with your mobile number
  delay(100);
  SIM.println(message);   // The SMS text you want to send
  delay(100);
  SIM.println((char)26);  // ASCII code of CTRL+Z
  delay(100);
  SIM.println();
  delay(1000);
}
void lock()
{
  lcd.clear(); // Clear LCD screen
    lcd.setCursor(0, 0);
      lcd.print("LOCKED");
  lcd.clear(); // Clear LCD screen
    lcd.setCursor(0, 0);
      lcd.print("* -> RFID");
      lcd.setCursor(0, 1);
      lcd.print("D -> PIN");
      RFIDMode = false;
          PwMode = false;
          NormalMode = false;
}
void unlock()
{          
          i=0;
          digitalWrite(lockPin, HIGH);
          tone(buzzerPin,200);
        delay(300);
        noTone(buzzerPin);
        delay(100);
        tone(buzzerPin,250);
        delay(300);
        noTone(buzzerPin);
          send_message("Door Opened \nIf it wasn't you, type 'close' to halt the system.");
          delay(2500);
          digitalWrite(lockPin, LOW);
          lcd.clear(); // Clear LCD screen
    lcd.setCursor(0, 0);
      lcd.print("* -> RFID");
      lcd.setCursor(0, 1);
      lcd.print("D -> PIN");
          RFIDMode = false;
          PwMode = false;
          NormalMode = false;
}
