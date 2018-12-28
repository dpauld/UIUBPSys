#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <LiquidCrystal.h>

int i;
//#define LED_G 5 //define green LED pin
//#define LED_R 4 //define red LED
//#define BUZZER 2 //buzzer pin

/*--------RFID----------*/
#define SS_PIN 10
#define RST_PIN 9
String content= "";
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

/*--------Display----------*/
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

/*--------Servo----------*/
Servo servo1; //define servo name
Servo servo2; 
#define servo1Sig_Pin A0 //define servo pin
#define servo2Sig_Pin A1
int initialServoPos = 90;

/*--------IR at Gates----------*/
#define IR1Gate_Pin A2
#define IR2Gate_Pin A3
int IR1_hasObstacle = HIGH;  // HIGH MEANS NO OBSTACLE
int IR2_hasObstacle = HIGH;  // HIGH MEANS NO OBSTACLE

/*--------IR at Slot----------*/
#define IR3Slot_Pin A4
#define IR4Slot_Pin A5
#define IR5Slot_Pin 8
int IR3_hasObstacle = HIGH;
int IR4_hasObstacle = HIGH;
int IR5_hasObstacle = HIGH;

/*--------User Defined Functions----------*/
int getIDSuccessRead();
int moveGate(Servo myServo,int startIndex ,int endIndex ,int amnt);
int updateParkingSlot();
boolean parkingIsNotFull();

int p; //to iterate servo position
int p2;
int arrivalEvent;
int parkingSlot[]={0,0,0};
int numberOfparkingSlot = 3;
int numberOfFreeSlot = 3;

void setup()
{
  Serial.begin(9600);   // Initiate a serial communication
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522

  lcd.begin(16, 2);
  lcd.print("WelcomeToParking!");
  lcd.setCursor(0,1);
  lcd.print("Slot Free: "+String(numberOfFreeSlot));
  servo1.attach(servo1Sig_Pin); //servo pin
  servo2.attach(servo2Sig_Pin);
  
  servo1.write(initialServoPos); //servo start position
  servo2.write(initialServoPos); 

  pinMode(IR1Gate_Pin, INPUT);
  pinMode(IR2Gate_Pin, INPUT);
  pinMode(IR3Slot_Pin, INPUT);
  pinMode(IR4Slot_Pin, INPUT);
  pinMode(IR5Slot_Pin, INPUT);
  
  Serial.println("Put your card to the reader...");
  Serial.println();
  arrivalEvent = 0;
}

void loop() 
{
  lcd.clear();
  lcd.print("WelcomeToParking!");
  lcd.setCursor(0,1);
  lcd.print("Slot Free: "+String(numberOfFreeSlot));
  int successRead = getIDSuccessRead();
  Serial.println("Reading RFID , successRead : "+String(successRead));
  delay(1000);
  if (successRead)
  {
    if (content.substring(1) == "E3 B1 76 89" || content.substring(1) == "FE 74 78 89") //change here the UID of the card/cards that you want to give access
    {
      Serial.println("Authorized access");
      Serial.println();
      if(parkingIsNotFull())
      {
        lcd.setCursor(0,1);
        lcd.print("You can Park.");
        arrivalEvent = 1; 
        moveGate(servo1,initialServoPos,0,-1);//Open the 1st gate , goes 90 deg to 0 deg
        lcd.clear();
        lcd.print("WelcomeToParking!");
        lcd.setCursor(0,1);
        lcd.print("Slot Free: "+String(numberOfFreeSlot));
      }
      else
      {
        lcd.setCursor(0,1);
        lcd.print("Parking Is Full.");
        delay(1000);
        lcd.clear();
        lcd.print("WelcomeToParking!");
        lcd.setCursor(0,1);
        lcd.print("Slot Free: "+String(numberOfFreeSlot));
      }
    }
    else   
    {
      Serial.println(" Access denied");
      lcd.setCursor(0,1);
      lcd.print("NotAllowedToPark!");
      //digitalWrite(LED_R, HIGH);
      
      //tone(BUZZER, 300);
      delay(1000);
      //digitalWrite(LED_R, LOW);
      //noTone(BUZZER);
      lcd.clear();
      lcd.print("WelcomeToParking!");
      lcd.setCursor(0,1);
      lcd.print("Slot Free: "+String(numberOfFreeSlot));
    }
  }
 
  IR1_hasObstacle = digitalRead(IR1Gate_Pin);
  Serial.print("IR1 Senses: "+String(IR1_hasObstacle)+" | Arrival Event: "+String(arrivalEvent)+"\n ");
  
  if(IR1_hasObstacle == LOW && arrivalEvent == 1)
  {
    Serial.println("IR1 Stop something is ahead!!");
    moveGate(servo1,0,initialServoPos,1); //Close the 1st gate , goes 0 deg to 90 deg
    moveGate(servo2,initialServoPos,0,-1); //Open the 2nd gate , goes 90 deg to 0 deg
    delay(5000);
    moveGate(servo2,0,initialServoPos,1); //Close the 2nd gate, goes from 0 deg to 90 deg
    arrivalEvent = 0;
  }
  
  IR2_hasObstacle = digitalRead(IR2Gate_Pin);
  Serial.print("IR2 Senses: "+String(IR2_hasObstacle)+" | Arrival Event: "+String(arrivalEvent)+"\n ");
  if(IR2_hasObstacle == LOW && arrivalEvent == 0)
  {
    Serial.println("IR2 Stop something is ahead!!");
    moveGate(servo2,initialServoPos,0,-1);//Open the 2nd gate , goes 90 deg to 0 deg
    delay(5000);
    moveGate(servo2,0,initialServoPos,1); //Close the 2nd gate, goes from 0 deg to 90 deg
  }

  IR1_hasObstacle = digitalRead(IR1Gate_Pin);
   Serial.print("IR1 Senses: "+String(IR1_hasObstacle)+" | Arrival Event: "+String(arrivalEvent)+"\n ");
  if(IR1_hasObstacle == LOW && arrivalEvent == 0)
  {
    Serial.println("IR1 Stop something is ahead!!");
    moveGate(servo1,initialServoPos,0,-1);//Open the 1st gate , goes 90 deg to 0 deg
    delay(5000);
    moveGate(servo1,0,initialServoPos,1); //Close the 1st gate , goes 0 deg to 90 deg
  }

  updateParkingSlot();
  
  Serial.println("__");
}

int getIDSuccessRead()
{
  content = "";
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return 0;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return 0;
  }
  //Show UID on serial monitor
  Serial.print("UID tag :");
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  return 1;
}

int moveGate(Servo myServo,int startIndex ,int endIndex ,int amnt)
{
    p = startIndex;
    while(p!=endIndex)
    {
        p = p + amnt;
        myServo.write(p);
        delay(50);
    }
}

int updateParkingSlot()
{
  IR3_hasObstacle = digitalRead(IR3Slot_Pin);
  if(IR3_hasObstacle == LOW){
    parkingSlot[0] = 1;
  }else{
    parkingSlot[0] = 0;
  }
  
  IR4_hasObstacle = digitalRead(IR4Slot_Pin);
  if(IR4_hasObstacle == LOW){
    parkingSlot[1] = 1;
  }else{
    parkingSlot[1] = 0;
  }
  
  IR5_hasObstacle = digitalRead(IR5Slot_Pin);
  if(IR5_hasObstacle == LOW){
    parkingSlot[2] = 1;
  }else{
    parkingSlot[2] = 0;
  }
}

boolean parkingIsNotFull()
{
  int sum = 0;
  for(int i=0;i<numberOfparkingSlot;i++)
  {
    sum = sum + parkingSlot[i];
  }
  numberOfFreeSlot = numberOfparkingSlot-sum;
  if(sum==numberOfparkingSlot)
  {
    return false;
  }

  return true;
}
