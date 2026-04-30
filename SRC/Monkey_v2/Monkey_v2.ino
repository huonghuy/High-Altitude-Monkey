
/////////// SOFTWARE VERSION NUMBER ///////////////////////////
// Ver 0.1M, 8/11/19
// Ver 0.2M, 8/14/19 adjusting communication from Serial2
// Ver 0.3MM  8/15/19 added Process_MONKEYMOVE
// Ver 0.4MM  8/16/19 got command parser to work from IDE
// Ver 0.5MM  8/19/19 ADDING HOME ROUTINE FOR SERVOS             
// Ver 0.6MM, 8/21/19 Intergrated serial communication between USIP and Monkey Board
// Ver 0.7MM  8/22/19 Implemented monkey reply back top USIP board
// Ver 0.8MM  8/23/19 Completed Monkey Touchdown move & monkey default position commands.
// Ver 0.9MM  9/10/19 Works with Radio APRS
// Ver 0.91MM 9/26/19 Added monkey video commands
// Ver 0.92MM 9/26/19 Fixed string values for battey voltage, inside temp & xmtr temp, added IDE comm setup comments
// Ver 0.93MM 10/03/19 Added get_battery_voltage, get_inside_temp, get_tramsmitter_temp routines.
// Ver 0.94MM 10/03/19 Added sent get_battery_voltage, get_inside_temp, get_tramsmitter_temp to USIP board, and video on and off.
// Ver 0.95MM 10/07/19 I2C transmission to video.  Set I2CVIDEOADDR to the I2C address  of the video
// Ver 0.96MM 10/13/19 Set voltage devide by value for battery.  
// Ver 0.97MM 10/13/19 Optimize Monkey Movements for Brown Monkey
// Ver 0.98MM 10/15/19 I2C works for Monkey and for potentiometer.
// Ver 0.99MM 10/15/19 Reads actual battery voltage, inside and Transmitter Temperature & MONKEY 17.
// Ver 1.0MM 10/17/19 Final chits-n-chatz before flight.
// Ver 1.1MM 10/17/19 Added stablize_monkey_slowly(); 
// Ver 1.11MM 10/18/19 Added slowed monkey even more.
// Ver 1.12MM 10/18/19 Monkey nod fine tweekings & left arm lowering.
// Ver 1.13MM 10/18/19 Adjusted video level number calculation.
// Ver 1.14MM 10/19/19 Added error msg monkey 17 if getWiper not equal to requested video level 
// Ver 1.15MM 10/20/19 added video start up for new video chip DS1809.
// Ver 1.16MM 10/21/19 Edited ds1809 code, commented out all references to DS3502 and its library. Decided to put 12k resistor in parallel  
//                     with the DS1809, so we can use all 63 steps and still have total resistor value <= 5.1k ohms
// Ver 1.17MM 11/1/19 Edit stablize_monkey_slowly with weight reduction, modified monkey chest pound & wave. 
// Ver 2.0MM  11/1/19 Flight Release.
//
// ///////////// 2023 UPDATES 
// Ver 2.1MM  07/22/23 Added include files for Brown & Green Monkey Servo Limits definitions
//
// ///////////// 2026 UPDATES 
// Ver 2.2MM 4/29/26 Fixed some stuff I found
//
// don't forget to adjust the #define VERSION below! 
#define VERSION "2.2MM" 
// Copyright © 2026 University of Bridgeport, NASA, CT Space Grant Consortium, USIP.
// All rights reserved.
///////////////////////////////////////////////////////////////

#include <Adafruit_PWMServoDriver.h>      //Download from Github
#include <Wire.h>                         //Included into Arduino library
#include <EEPROM.h>                       // for persisting video RF level across boots

/////// MODIFY HERE - CHOOSE WHICH MONKEY TO COMPILE FOR - THIS DETERMINES WHICH HEADER FILE TO INCLUDE - do not include both ///////////////////////////
//#define GREEN_MONKEY
#define TAN_MONKEY
////////////////////////////////////////////////////////////////////////////////////////////


////////////// ADJUST MONKEY ABOVE NOT BELOW ////////////////////////////
#ifdef TAN_MONKEY // DO NOT MODIFY THIS STATEMENT // 
   #include "USIP_Tan_Monkey.h"              // include file with brown monkey servo definitions
#elif defined GREEN_MONKEY
   #include "USIP_Green_Monkey.h"              // include file with Green monkey servo definitions
#else
  #error "Undefined Monkey color. No Servo limits included. -GTM"
 #endif
//////////////////include USIP LCD ///// comment out if you do not wish to include /////
//  #include "USIP_LCD.h"
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//Declarations
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN 150 // min pulse lenghth count
#define SERVOMAX 600 // max pulse lenghth count

uint8_t servo1neck = 0;

int pos=0;
float fpos = 0.0;
int pos2=300;

int right=0;
int right2=300;
int right3=-60;

int left=0;
int left2=0;
int left3=0;

int head_up;

int ledpin = 13; // Led to blink
int x;
int MKRCV_pin = 16;
int MKXMT_pin = 17;

///////////////////// Monkey Home Positions - moved to individual Header files /////////////////////

///////////////////// GENERAL DEFINES ///////////////////////////
// Command pipeline. Was previously a tangle of Arduino String objects (heap churn,
// substring/concat allocations on every byte). Now: fixed char[] buffer set by the
// serialEvent / serialEvent2 ISRs after prefix validation, dispatched by loop().
#define MONKEY_CMD_LEN 9          // "MONKEY XX" — exactly 9 chars (not counting newline)
char MonkeyCmd_buf[MONKEY_CMD_LEN + 1] = {0};   // null-terminated command string for the dispatcher
boolean MonkeyFlag = false;       // true => MonkeyCmd_buf has a fresh command for loop() to dispatch
boolean validcmd_flg = false;
boolean stablize_monkey_flg = false; // used to determine when to stabilize the monkey
unsigned long tee;
boolean wire_start = false;       // used when beginning Wire.begin on I2C bus to see if it began

#define MK1 "MONKEY"
#define MK01 "MONKEY 01"


int monkey_cmd_nbr = 0;  // used to parse monkey commands for video commands
int video_level = 0; // used to hold calculated video command number

////////////////////////////////////////////////////////////////////////////////////////
// Battery Voltage defines
// Schematic (2026-04): A2 sits at junction of R6 (10k, top, to V_bat) and R4 (3.9k, bottom, to GND).
//   V_pin / V_bat = R4 / (R6+R4) = 3900 / 13900 = 0.2806
//   V_bat = V_pin * (1/0.2806) = V_pin * 3.564     <-- theoretical
// DivideByCircuit below is bench-calibrated (1.4% lower than theoretical). If R6/R4 are
// replaced or the board changes, re-measure with a DMM at a known V_bat and update.
int Vbat = 0;    // battery voltage read
const float BAT_DIV_THEORETICAL = 3.564f;       // (R6+R4)/R4 from schematic; reference only
const float DivideByCircuit     = 3.515f;       // bench-calibrated multiplier — measure & update if hardware changes
int BatteryVoltagePin = A2; // Battery voltage connected to analog pin 2, outside leads to ground and +5V
int BatRead1  = 0;           // 1st value read of battery
float BatV1   = 0.0;         // variable to store the 1st value read
int BatRead2  = 0;           // 2nd value read of battery
float BatV2   = 0.0;         // variable to store the 1st value read
int BatRead3  = 0;           // 3rd value read of battery
float BatV3   = 0.0;         // variable to store the 1st value read
float BatSum  = 0.0;         // Battery Sum voltage used to calculate arithmetic mean
float BatMean = 0.0;         // arithmetic mean of battery voltage
int TenTimes  = 0;           // This is the voltage multiplied by 10 and then made into an integer.

////////////////////////////////////////////////////////////////////////////////////////
// Inside Temperature defines
int InsideTempPin = A0; // Inside Temp sensor is connected to analog pin 0.
int InsideT = 0;   // Inside temp integer read
float TRaw = 0;    // raw temperature read
float TempK = 0.0; // is raw temperature converted to degrees Kelvin
float TempC = 0.0; //is Temperature Degrees Centigrade

////////////////////////////////////////////////////////////////////////////////////////
// Transmitter Temperature defines
int XmtrTempPin = A1; // Inside Temp sensor is connected to analog pin A1.
int XmtrT = 0;   // transmitter temp Integer read
float XTRaw  = 0.0; //analogRead(XmtrTempPin);      // read transmitter temperature
float XTempK = 0.0; // = (XTRaw/2.046);
float XTempC = 0.0; // = XTempK - 273.15;
int xmtr_temp = 0; //  = (int) XTempC;

////////////////////////////////////////////////////////////////////////////////////////
// Video Defines
// video ON/OFF is pin 22.   A ONE turns on the video, a ZERO turns it off
///////////////////////////////////////////////////////////////////////////////
#define VIDEO_pin 22

//DS1809 video chip
int DS1809_Val = 15; // used for value of the chip wiper.  use DS1809_GetTrakVal and DS1809_SetTrakVal
int videoRFLevel = 15;
#define DS1809_pinup 24
#define DS1809_pindown 26
int itest = 0;

// EEPROM persistence — the DS1809 wiper is volatile, so without this a brown-out reset
// in flight loses the operator's last RF-level setting and snaps back to the default 15.
#define EEPROM_VIDEO_LEVEL_ADDR 0
#define VIDEO_LEVEL_DEFAULT 15

uint8_t load_video_level_from_eeprom() {
    uint8_t v = EEPROM.read(EEPROM_VIDEO_LEVEL_ADDR);
    if (v > 63) return VIDEO_LEVEL_DEFAULT;   // 0xFF on a fresh chip, or corrupt
    return v;
}


//////////////////////////// SET UP ////////////////////////////////

void setup() {
  // setup code here, runs once:

  pinMode(ledpin, OUTPUT);
  pinMode(MKRCV_pin, INPUT);
  pinMode(MKXMT_pin, OUTPUT);
  pinMode(VIDEO_pin, OUTPUT); // video on off
//  pinMode(WIPER_pin, INPUT); // wiper pin        No longer use LJR 10/21
  
//DS1809 video chip
pinMode (DS1809_pinup, OUTPUT);
pinMode (DS1809_pindown, OUTPUT);
digitalWrite(DS1809_pinup, HIGH);
digitalWrite(DS1809_pindown, HIGH);
digitalWrite(DS1809_pindown, LOW);
digitalWrite(DS1809_pindown, HIGH);

  // initialize serial: 
  // Blink LED that we are up and running
  for (x=0; x<5; x++)  {
    digitalWrite(ledpin,HIGH);
    delay(100);
    digitalWrite(ledpin,LOW);
    delay(100);
  }
/////////////////////////////////////////////////////////////////////////////////////////
// IMPORTANT for IDE Communications, this configuration is important
// Autoscroll is checked
// Show Timestamp is unchecked
// first pull down is set to NEWLINE
// Baud pull down is set to whatever you set "Serial.begin" to in the statement below, in this case 9600
// Clear Output is not a settable button, It clears the output screen 
Serial.begin(9600);
Serial2.begin(9600);//Monkey cpu
//Serial.print("2 Channel servo test");
//Serial.println("");
Serial.print("USIP Monkey ");
Serial.println (VERSION); 

////////////////////////////////////////////////////////////////////////////////////////
pwm.begin();
pwm.setPWMFreq(60);  //Analog servos run at ~60 Hz updates

//////////////////////////////////////////////////////////////////////////////
// I2C bus set up
Wire.begin(); // join i2c bus (address optional for master)

// Set the DS1809 chip to Zero for start  count down for 65 ticks
DS1809_SettoZero();
// Restore the operator's last RF level from EEPROM (or default 15 on a fresh chip).
set_video_level(load_video_level_from_eeprom());

//////////////////////////////////////////////////////////////////////////////
// bring monkey to default positions AT THE START 
stablize_slowly();
} // end of setup
//////////////////////////////////////////// END OF SET UP ////////////////////////////////

//////////////////////////////////////// LOOP /////////////////////////////////////////////////////
void loop() {      // Main code here, runs repeatedly

if (MonkeyFlag == true) {
  Process_MONKEYMOVE(MonkeyCmd_buf);
  MonkeyCmd_buf[0] = '\0';
  MonkeyFlag = false;
}

  
//servohead_no_nod();
//servohead_yes_nod();
//servohead2(); //head half scan
//scanhead(); //Full scan
//right_arm2(); // Thumbs up
//left_arm (); // Wave
//monkey_chest();// Chest pound

/////not needed:////////
//head_scratch();
//monkey_reset();
//autopilot cycle();
} ////////////////////////////////////// END LOOP ////////////////////////////////////////////////////

//////////////////////// Selecting a Monkey Command ////////////////////////////////////////////////////

// Dispatch a validated "MONKEY XX" command. Caller (loop / serialEventX) guarantees
// the buffer is exactly 9 chars and prefix-validated as "MONKEY ".
// Branches use else-if so command 21..84 (video level) cannot also fire alongside an
// equals() match — important when reserved ranges grow in the future.
void Process_MONKEYMOVE(const char* MonkCMDStr){
  validcmd_flg = false;
  stablize_monkey_flg = false;
    Serial.println(" ");
    Serial.print("ENTERED MONKEY MOVE/video set AND MonkCMDStr is: ");
    Serial.println(MonkCMDStr);

// Is it stablize?
if (strcmp(MonkCMDStr, "MONKEY 00") == 0){
    Serial.println("Command equals MONKEY 00 - Setting Monkey to default positions");
    // set any flags
    // call Monkey Motions
       stablize_slowly(); // bring monkey to default positions
  
    // send MONKEY 11 (done) to USIP
    Serial.print("MONKEY 11 (done) Signal");
    Serial2.println("MONKEY 11");
    validcmd_flg = true;
}
   
// Is it Head Nod Yes?
else if (strcmp(MonkCMDStr, "MONKEY 01") == 0){
   Serial.println("Command equals MONKEY 01");
    // set any flags
    // call Head Nod Yes
   servohead_yes_nod();
   servohead_yes_nod();
    Serial.println("RETURNED FROM yes_nod()MONKEY01");    
    monkey_rest(); // bring monkey to default positions
    
    // send MONKEY 11 (done) to USIP 
    Serial.print("MONKEY 11 (done) Signal");
    Serial2.println("MONKEY 11");
    stablize_monkey_flg = false;
    validcmd_flg = true;
}

 
// Is it Head Nod No?
else if (strcmp(MonkCMDStr, "MONKEY 02") == 0){
   Serial.println("Command equals MONKEY 02");
    // set any flags
    // call Head Nod No
  //    servohead_no_nod();
 servohead_no_nod();
 Serial.println("RETURNED FROM no_nod()MONKEY02");   
    
 monkey_rest(); // bring monkey to default positions

 // send MONKEY 11 (done) to USIP 
    Serial.print("MONKEY 11 (done) Signal");
    Serial2.println("MONKEY 11");
      validcmd_flg = true;
      stablize_monkey_flg = false;
}

// Is it Right Arm Thumbs Up?
else if (strcmp(MonkCMDStr, "MONKEY 03") == 0){
    Serial.println("Command equals MONKEY 03");
    // set any flags
    // call Right Arm Thumbs Up
    right_arm2();
 Serial.println("RETURNED FROM right_arm2()MONKEY03"); 
    
    // send MONKEY 11 (done) to USIP 
      //monkey_rest();zzzz // bring monkey to default positions 
    Serial.print("MONKEY 11 (done) Signal");
    Serial2.println("MONKEY 11");
    validcmd_flg = true;
      stablize_monkey_flg = false;
}

// Is it Scan Head?
else if (strcmp(MonkCMDStr, "MONKEY 04") == 0){
    Serial.println("Command equals MONKEY 04");
    // set any flags
    // call Scan Head
    scanhead();
     Serial.println("RETURNED FROM scan_head()MONKEY04"); 
    // send MONKEY 11 (done) to USIP
      monkey_rest(); // bring monkey to default positions
    Serial.print("MONKEY 11 (done) Signal");
  Serial2.println("MONKEY 11");
    validcmd_flg = true;
      stablize_monkey_flg = false;
}
// Is it Left Hand Wave?
else if (strcmp(MonkCMDStr, "MONKEY 05") == 0){
    Serial.println("RETURNED FROM left_hand_wave()MONKEY05"); 
    // set any flags
    // call Left Hand Wave
    left_arm ();
    // send MONKEY 11 (done) to USIP
      monkey_rest(); // bring monkey to default positions
    Serial.print("MONKEY 11 (done) Signal");
    Serial2.println("MONKEY 11");
    validcmd_flg = true;
      stablize_monkey_flg = false;
}
// Is it a chest pound?
else if (strcmp(MonkCMDStr, "MONKEY 06") == 0){
    Serial.println("RETURNED FROM chest_pound()MONKEY06"); 
    // set any flags
    // call Chest Pound
    monkey_chest();
    // send MONKEY 11 (done) to USIP
      //monkey_rest(); // bring monkey to default positions
      monkey_rest(); // bring monkey to default positions
    Serial.print("MONKEY 11 (done) Signal");
   Serial2.println("MONKEY 11");
   validcmd_flg = true;
     stablize_monkey_flg = false;
}
// Is it a touchdown signal?
else if (strcmp(MonkCMDStr, "MONKEY 07") == 0){
    Serial.println("Command equals MONKEY 07");
    // set any flags
     monkey_touchdown(); // monkey to raise both arms up at once
     Serial.println("MONKEY 07 (touchdown) Signal");
     Serial2.println("MONKEY 11");
      //monkey_rest(); // bring monkey to default positions
      //monkey_rest(); // bring monkey to default positions

    validcmd_flg = true;
      stablize_monkey_flg = true;
}
// Is it all motions executed once?
else if (strcmp(MonkCMDStr, "MONKEY 08") == 0){
    Serial.println("Command equals MONKEY 08");
    // set any flags
    // call Monkey Motions
       servohead_no_nod();
       delay(1500);
       servohead_yes_nod();
       servohead_yes_nod();
       delay(2200);
       right_arm2(); // Thumbs up right arm
//       stablize_slowly();
       delay(2000);
       scanhead(); //Full scan
       left_arm (); // Wave left arm
       delay(4000);
       monkey_chest();  // Chest pound

  

       //monkey_rest(); // bring monkey to default positions
    // send MONKEY 11 (done) to USIP
    monkey_rest(); // bring monkey to default positions
    Serial.print("MONKEY 11 (done) Signal");
    Serial2.println("MONKEY 11");
    validcmd_flg = true;
      stablize_monkey_flg = true;
}
//I am Here
else if (strcmp(MonkCMDStr, "MONKEY 09") == 0){
    Serial.println("Command equals MONKEY 09");
    Serial2.println("MONKEY 11");
        Serial.println("Sent MONKEY 11");
        stop_monkey_servos(); // stop the servos
    validcmd_flg = true;
      stablize_monkey_flg = false;
}
//shutdown
else if (strcmp(MonkCMDStr, "MONKEY 18") == 0){
    Serial.println("Command equals MONKEY 18");
    Serial2.println("MONKEY 10");
        Serial.println("Sent MONKEY 10");
        stop_monkey_servos();
    validcmd_flg = true;
      stablize_monkey_flg = false;
}

// Is it a REQUEST FOR DATA?
else if (strcmp(MonkCMDStr, "MONKEY 12") == 0){
    Serial.println("Command equals MONKEY 12");
    // Read appropriate Data
    // write data into appropriate variables
    // Vbat = battery voltage Integer
    // InsideT = Inside temp integer
    // XmtrT = transmitter temp Integer

      Vbat = get_bat_voltage(); // this routine reads the battery voltage
          Serial.println("battery voltage read");
          Serial.println(Vbat);
      InsideT = get_inside_temp(); // this routine reads the inside temperature
          Serial.println("Inside temperature read");
            Serial.println(InsideT);        
      XmtrT = get_xmtr_temp();   // this routine reads the transmitter temperature
          Serial.println("transmitter temperature read");
           Serial.println(XmtrT);     
//
//    tee = millis();
//Serial.print ("end time = ");
//Serial.println (tee);


    // send back MONKEY 13    
    // send data to USIP board via Serial 2
            Serial2.print("MONKEY 13 ");
    // send back Vbat InsideT and XmtrT
            Serial2.print(Vbat);
            Serial2.print(",");
            Serial2.print(InsideT);
            Serial2.print(",");
            Serial2.println(XmtrT);   
          // send monkey done
          // send MONKEY 11 (done) to USIP
          // Serial.print("MONKEY 11 (done) Signal");
         //  Serial2.println("MONKEY 11");
  
    validcmd_flg = true;
      stablize_monkey_flg = false;
}

// Is it a turn video OFF command?
else if (strcmp(MonkCMDStr, "MONKEY 20") == 0){
    Serial.println("Command equals MONKEY 20");
    // set any flags
     Serial.println("MONKEY 20 turn video OFF");
    video_off(); // turn off video
    Serial.print("MONKEY 11 (done) Signal");
    Serial2.println("MONKEY 11");
    validcmd_flg = true;
}


// Is it a turn video ON command?
else if (strcmp(MonkCMDStr, "MONKEY 19") == 0){
    Serial.println("Command equals MONKEY 19");
    Serial.println("MONKEY 19 turn video ON");
    video_on();  // no readback path yet; just acknowledge
    Serial.print("MONKEY 11 (done) Signal");
    Serial2.println("MONKEY 11");
    validcmd_flg = true;
}



// Could still be a video-level command (MONKEY 21..MONKEY 84 → level 0..63).
// atoi parses the two ASCII digits at byte 7-8; non-digits give 0 which is out of range.
else {
    monkey_cmd_nbr = atoi(MonkCMDStr + 7);
    if ((monkey_cmd_nbr >= 21) && (monkey_cmd_nbr <= 84)){
        Serial.println("calculating monkey video level number");
        video_level = (monkey_cmd_nbr - 21);
        Serial.println("setting monkey video level number to:");
        Serial.println(video_level);
        set_video_level(video_level);
        Serial.print("set monkey video (done)");
        Serial.print("MONKEY 11 (done) Signal");
        Serial2.println("MONKEY 11");
        validcmd_flg = true;
    }
}
 






/////////////////////////////////////////////////////////////////////// 
// etc do all the valid monkey xx commands
// then do invalid command
// is it invalid command?
//if (validcmd_flg==false && MonkCMDStr.equals("MONKEY 01") ){
 //  Serial.println("Invalid MONKEY ");

if( validcmd_flg == false){
 Serial.println("Invalid MONKEY Sent Dumped Command");
 Serial2.println("MONKEY 14"); //Dumped Command
 // dump the command it's no good
// send MONKEY 14 to USIP  I dumped it.
// transmit dump command
}

// Caller (loop) clears MonkeyFlag and MonkeyCmd_buf after we return.
monkey_cmd_nbr = 0;
if (stablize_monkey_flg == true){
    stablize_slowly();
}
stablize_monkey_flg = false;
}








/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////






/////////////Monkey Commands//////////////////





///////////////////////////////////////////was 10 10////3////////////////////////////////
void stablize(){
 Serial.println("Stablizing Monkey");
  for(pos = rightShoulder2; pos <= rightShoulder2 + 15; pos += 1){
    pwm.setPWM(2,0,pos);
    delay(3);
    pwm.setPWM(3,0,pos + 125);
    delay(3);
    pwm.setPWM(4,0,pos + 125);
    delay(3);
    pwm.setPWM(5,0,pos - 150);
    delay(3);
  }
  for(pos = rightShoulder2 + 15; pos >= rightShoulder2; pos -= 1){
    pwm.setPWM(2,0,pos);
    delay(3);
    pwm.setPWM(3,0,pos + 125);
    delay(3);
    pwm.setPWM(4,0,pos + 125);
    delay(3);
    pwm.setPWM(5,0,pos - 150);
    delay(3);
  }
  
}
///////////////////////////////////////////was 10 10////3////////////////////////////////
//int botHead0 = 300;
//int topHead1 = 350;
//int rightShoulder2 = 275;
//int rightElbow3 = 400;
//int leftShoulder4 = 400;
//int leftElbow5 = 125;
//



void stablize_slowly(){
  pwm.setPWM(0,0,botHead0);
  pwm.setPWM(1,0,topHead1);
  pwm.setPWM(2,0,rightShoulder2);
  pwm.setPWM(3,0,rightElbow3);
  pwm.setPWM(4,0,leftShoulder4);
  pwm.setPWM(5,0,leftElbow5);
  delay(150);
 
// Serial.println("Slowly Stablizing Monkey");
//  for(pos = rightShoulder2; pos <= rightShoulder2 + 15; pos += 1){
//    pwm.setPWM(2,10,pos);
//    delay(15);
//    pwm.setPWM(3,10,pos + 200);
//    delay(15);
//    pwm.setPWM(4,10,pos + 200);
//    delay(15);
//    pwm.setPWM(5,10,pos - 225);
//    delay(15);
//  }
//  for(pos = rightShoulder2 + 15; pos >= rightShoulder2; pos -= 1){
//    pwm.setPWM(2,10,pos);
//    delay(15);
//    pwm.setPWM(3,10,pos + 200);
//    delay(15);
//    pwm.setPWM(4,10,pos + 200);
//    delay(15);
//    pwm.setPWM(5,10,pos - 225);
//    delay(15);
//  }
//   servohead_no_nod_slowly();
//  servohead_yes_nod();
stop_monkey_servos();
}



/////////////////////////////////////////////////////////////

void servohead_yes_nod() {
//Servo1 left and right motion
Serial.println("Entered servohead_yes_nod()MONKEY 01");
for (pos = topHead1; pos <= topHead1 + 120; pos += 2) { // goes from 0 degrees to 180 degrees was 150
  //Range = 150
    // in steps of 1 degree

    pwm.setPWM(0,0,botHead0); //start bottom servo head at middle position
    delay(2);
   

    pwm.setPWM(1,0,pos); //move top servo head down in increments
    delay(5);

 }

//for (pos = 400; pos >= 300; pos -= 1) { 
  for (pos = topHead1 + 90; pos >= topHead1; pos -= 1) {

      pwm.setPWM(1,0,pos); //move top servo head up in increments
      delay(5);
  
  }// 1st round

//2nd round:


//for (pos = 300; pos <= 400; pos += 1) { // goes from 0 degrees to 180 degrees
//    // in steps of 1 degree
//
//    pwm.setPWM(0,0,300); //start bottom servo head at middle position
//    //pos2=SERVOMIN;
//    delay(2);
//   
//    pwm.setPWM(1,0,pos-150); //move top servo head down in increments
//      delay(2);
//
//  }
//
//for (pos = 400; pos >= 300; pos -= 1) { 
//
//
//      pwm.setPWM(1,0,pos-150); //move top servo head up in increments
//      delay(2);
//  
//}


} //end servohead_yes_nod

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void servohead_no_nod() {
//Servo1 left and right motion
 pwm.setPWM(1,0,topHead1); //top servo head  //up-and-down servo in base position
      delay(5);
for (pos = botHead0; pos <= botHead0 + 90; pos += 1) { // goes from 0 degrees to 180 degrees
   //Range = 90
 pwm.setPWM(0,0,pos); //bottom servo head    
    delay(6);    
  }

for (pos = botHead0 + 90; pos >= botHead0 - 90; pos -= 1) { 
  pwm.setPWM(0,0,pos);
  delay(6); 
}  //1st round

//2nd round:



for (pos = botHead0 - 90; pos <= botHead0; pos += 1) { // goes from 0 degrees to 180 degrees  
 pwm.setPWM(0,0,pos); //bottom servo head
    //pos2=SERVOMIN;
    delay(6);
//    pwm.setPWM(1,0,topHead1); //top servo head
//      delay(5);
  }

for (pos = botHead0; pos <= botHead0 + 90; pos += 1) { // goes from 0 degrees to 180 degrees
   //Range = 90
 pwm.setPWM(0,0,pos); //bottom servo head
   delay(6);   
// pwm.setPWM(1,0,topHead1); //top servo head
//      delay(5);
  }
for (pos = botHead0 + 90; pos >= botHead0; pos -= 1) { 
  pwm.setPWM(0,0,pos);
  delay(6);
}  //1st round

//2nd round:   (deleted)
//for (pos = botHead0 - 90; pos <= botHead0; pos += 1) { // goes from 0 degrees to 180 degrees   
// pwm.setPWM(0,0,pos); //bottom servo head
//    //pos2=SERVOMIN;
//    delay(20);
//    pwm.setPWM(1,0,topHead1); //top servo head
//      delay(5);
//  }

// kill the power
   pwm.setPWM(0,0,4096);
   pwm.setPWM(1,0,4096);
}//end of no nod


//////////////////////////////////////////////////////////////////////////////////////////////////////////


void left_arm (){
 
  for (left = leftShoulder4; left >= leftShoulder4 - 325; left -= 5) { // 1ft move up shoulder
    //Range == -250
   //if(left>100){
   pwm.setPWM(4,0,left);
   //pos2=300;
   delay(7);
   }

/////////////////////// hand wave 1st cycle//////////////////////////////////////////////
//hand wave 1st cycle:
// go OUT first

for (left = leftElbow5; left >= leftElbow5 - 40; left -= 2) { // C1 fix: was `<=` so loop body never ran. Bug went undetected because cycles 2/3 partially compensate.
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(7);
   }
for (left = leftElbow5 - 40; left <= leftElbow5 - 40 + elbowRange; left += 2) { // start from where the previous loop ended (was `leftElbow5` which jumped the elbow forward 40 ticks at the seam)
  //Range == +60
   //if(left>100){
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(7);
   }
for (left = leftElbow5 - 40 + elbowRange; left >= leftElbow5 - 40; left -= 2) { // 2nd move left side arm  1 of 2
   //if(left>100){
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(7);
   }
  

/////////////////////// hand wave 2nd cycle//////////////////////////////////////////////
//hand wave 2nd cycle

for (left = leftElbow5 - 40; left <= leftElbow5 -40 + elbowRange; left += 2) { // 2nd move right side arm 2 of 2
  //Range == +60
   //if(left>100){
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(7);
   }
for (left = leftElbow5 - 40 + elbowRange; left >= leftElbow5 - 40; left -= 2) { // 2nd move left side arm  1 of 2
   //if(left>100){
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(7);
   }

/////////////////////// hand wave 3rd cycle//////////////////////////////////////////////
//hand wave 3rd cycle:

for (left = leftElbow5 -40; left <= leftElbow5 - 40 + elbowRange; left += 2) { // 2nd move right side arm 2 of 2
  //Range == +60
   //if(left>100){
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(7);
   }
for (left = leftElbow5 - 40 + elbowRange; left >= leftElbow5; left -= 2) { // 2nd move left side arm  1 of 2
   //if(left>100){
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(7);
   }
  

////////////////////////////////////

//////////Returning shoulder back to original position///////////////////////

  for (left = leftShoulder4 - 325; left <= leftShoulder4; left += 5) { // 3rd move down slowly shoulder
   //if(left>100){
   pwm.setPWM(4,0,left);
   //pos2=300;
   delay(5);
   }
   delay(10);
   pwm.setPWM(4,0,4096);
//  for (left = leftShoulder4 - 50; left <= leftShoulder4; left += 1) { // 3rd move down slowly shoulder
//   //if(left>100){
//   pwm.setPWM(4,0,left);
//   //pos2=300;
//   delay(9);
//   }

}//end of left arm

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void servohead2(){      //Tilt,look down and reset
//Servo1 left and right motion
for (pos = botHead0; pos <= botHead0 + 100; pos += 1) { // goes from 0 degrees to 180 degrees
  //Range == +100
    // in steps of 1 degree

    pwm.setPWM(0,0,pos); //bottom servo head
    //pos2=SERVOMIN;
    delay(5);
   

    pwm.setPWM(1,0,pos-150); //top servo head RH
      delay(5);


  }

for (pos = botHead0 + 100; pos >= botHead0; pos -= 1) { 

  pwm.setPWM(0,0,pos);
  delay(15);

      pwm.setPWM(1,0,pos-150); //top servo head
      delay(5);
  
}

  
}//end of servohead2 

//////////////////////////////////////////////////////SCAN THE CONSOLE///////////////////////////////////
void scanhead(){       //ACtually scanning the console
//Servo1 left and right motion
for (pos = botHead0; pos <= botHead0 + 100; pos += 1) { // goes from 0 degrees to 180 degrees
  //Range = +100
    // in steps of 1 degree
    // Head tilts down as it turns left

    pwm.setPWM(0,0,pos); //bottom servo head
    //pos2=SERVOMIN;
    delay(15);
   

    pwm.setPWM(1,0,pos+70); //top servo head
      delay(15);


  }

for (pos = botHead0 + 100; pos >= botHead0 - 100; pos -= 1) { 
  // Head locks angle of neck and looks right
  pwm.setPWM(0,0,pos);
  delay(13);

      //pwm.setPWM(1,0,pos-150); //top servo head
      //delay(5);
  
}
for (pos = botHead0 - 100; pos <= botHead0 + 100; pos += 1) { 
  // Head looks back left
  pwm.setPWM(0,0,pos);
  delay(13);

      //pwm.setPWM(1,0,pos-150); //top servo head
      //delay(5);
  
}
for (pos = botHead0 + 100; pos >= botHead0; pos -= 1) { 
  // Head returns to normal
  pwm.setPWM(0,0,pos);
  delay(5);

      pwm.setPWM(1,0,pos+70); //top servo head
      delay(5);
}
pwm.setPWM(0,0,botHead0);
pwm.setPWM(1,0,topHead1);
delay(10);
pwm.setPWM(0,0,4096);
pwm.setPWM(1,0,4096);
}
///////////////////end of scan head

//////////////////////////////////////////////////////////THUMBS UP////////////////////////////////////////
void right_arm2 (){   //Thumbs up action
  for (right = rightShoulder2; right <= rightShoulder2 + 170 ; right += 2) { // goes from 0 degrees to 180 degrees
    //Range = + 200
    pwm.setPWM(2,0,right); //right shoulder servo head
    delay(7);
    
  }

  for (right = rightElbow3; right >= rightElbow3 - 125; right -= 1) {
    //Range = -95
      pwm.setPWM(3,0,right); //right arm servo head
      delay(10); 
  }
  
  for (right = rightElbow3 - 125; right <= rightElbow3; right += 1) {
      pwm.setPWM(3,0,right); //right arm servo head
      delay(10); 
  }


  

  for (right = rightShoulder2 + 170; right >= rightShoulder2; right -= 2) { // goes from 180 degrees to 0 degrees
  
   pwm.setPWM(2,0,right);
   //pos2=300;
   delay(7);
   }
   delay(10);
   pwm.setPWM(2,0,4096);

} //end right_arm


////////////////////////////////////////CHEST POUND ////////////////////////////////////////////////

void monkey_chest(){   //Monkey beating chest action
//Range = +170
  for (right = rightShoulder2; right <= rightShoulder2 + 150; right += 2) { // move up right shoulder
    pwm.setPWM(2,0,right); //right shoulder servo head
    delay(7);
    
  }

  ///>>>>>>>>>Right arm Movement>>>>>////////

for (left = leftShoulder4; left >= leftShoulder4 - 160; left -= 2) { // 1ft move up left shoulder
  //Range = -200
   //if(left>100){
   pwm.setPWM(4,0,left);
   //pos2=300;
   delay(7);
   }
///////////////////////////////// 

//Right forearm motion: 1st cycle

  for (right = rightElbow3; right >= rightElbow3 - 240; right -= 1) {
    //Range = - 225
      pwm.setPWM(3,0,right); //right arm servo head
      delay(1); 
  }
 
  for (right = rightElbow3 - 240; right <= rightElbow3; right += 1) {
      pwm.setPWM(3,0,right); //right arm servo head
      delay(1); 
 }
 


// left forearm motion: 1st cycle


for (left = leftElbow5; left <= leftElbow5 + 270; left += 1) { // 2nd move right side arm 2 of 2
  //Range = +200
   //if(left>100){
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(1);
   }
for (left = leftElbow5 + 270 ; left >= leftElbow5; left -= 1) { // 2nd move left side arm  1 of 2
   //if(left>100){
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(1);
   }
  
   
/////////////////////////////////////////////
//////// 2nd cycle of forearm chest movement



//Right forearm motion: 2nd motion

  for (right = rightElbow3; right >= rightElbow3 - 240; right -= 1) {
    //Range = - 225
      pwm.setPWM(3,0,right); //right arm servo head
      delay(1); 
  }
 
  for (right = rightElbow3 - 240; right <= rightElbow3; right += 1) {
      pwm.setPWM(3,0,right); //right arm servo head
      delay(1); 
 }
 


// left forearm motion: 2nd motion


for (left = leftElbow5; left <= leftElbow5 + 270; left += 1) { // 2nd move right side arm 2 of 2
  //Range = 200
   //if(left>100){
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(1);
   }
for (left = leftElbow5 + 270 ; left >= leftElbow5; left -= 1) { // 2nd move left side arm  1 of 2
   //if(left>100){
   pwm.setPWM(5,0,left);
   //pos2=300;
   delay(1);
   }


////////

  for (right = rightShoulder2 + 150; right >= rightShoulder2; right -= 2) { // bring left shoulder back to original position
  
   pwm.setPWM(2,0,right);
   //pos2=300;
   delay(7);
   }
   delay(10);
   pwm.setPWM(2,0,4096);



for (left = leftShoulder4 - 160; left <= leftShoulder4; left += 1) { // bring left shoulder back to original position
   pwm.setPWM(4,0,left);
   delay(3);
   }
   delay(10);
   pwm.setPWM(4,0,4096);
   


} //end chest movement

///////////////////////// SERIAL COMMUNICATIONS SECTION ///////////////////////////////////////////

//Communication section - taking in initial commands

// --> one from keyboard (IDE - Serial.print() )
/* SerialEvent occurs whenever a new data comes in the
   hardware serial RX.  This routine is run between each
   time loop() runs, so using delay inside loop can delay
   response.  Multiple bytes of data may be available.
*/
// Receive bytes from USB Serial. Accepts only newline-terminated frames that begin with
// "MONKEY " and are at least 9 chars (the protocol's full "MONKEY XX" form). Garbage
// frames are dropped silently — at altitude, RF noise should not be acknowledged.
// Per-byte Serial.print debug spam was removed; it cost cycles and could swamp the
// USB log on a fast sender. (See feedback if you need to bring it back behind a #ifdef.)
static void monkey_rx_feed(char c, char* buf, uint8_t* idx, uint8_t bufsize) {
    if (c == '\n' || c == '\r') {
        buf[*idx] = '\0';
        if (*idx >= MONKEY_CMD_LEN && strncmp(buf, "MONKEY ", 7) == 0) {
            memcpy(MonkeyCmd_buf, buf, MONKEY_CMD_LEN);
            MonkeyCmd_buf[MONKEY_CMD_LEN] = '\0';
            MonkeyFlag = true;
        }
        *idx = 0;
    } else if (*idx < bufsize - 1) {
        buf[(*idx)++] = c;
    } else {
        // Overflow on a frame missing newline — drop and resync.
        *idx = 0;
    }
}

void serialEvent() {
    static char rx_buf0[16];
    static uint8_t rx_idx0 = 0;
    while (Serial.available()) {
        monkey_rx_feed((char)Serial.read(), rx_buf0, &rx_idx0, sizeof(rx_buf0));
    }
}

void serialEvent2() {
    static char rx_buf2[16];
    static uint8_t rx_idx2 = 0;
    while (Serial2.available()) {
        monkey_rx_feed((char)Serial2.read(), rx_buf2, &rx_idx2, sizeof(rx_buf2));
    }
}

//////////////////////////////////////////////////////////////////////////
// Head Servo Left & Right
void bottom_head_servo_go_home(){  //pin 0 - on servo board                  
Serial.println("Entered bottom_head_servo_go_home(): Moving top servo to home ");
   pwm.setPWM(0,0,300); // 300 is facing forward. Greater moves right, less moves left
  
}
/////////////////////////////////////////////////////////////////////////////////
// Head Servo Up & Down
void top_head_servo_go_home(){  //pin 1 - on servo board
Serial.println("Entered top_head_servo_go_home(): Moving bottom servo to home ");
   pwm.setPWM(1,0,220);  // greater is up, less is down
   
}

/////////////////////////////////////////////////////////////////////////////////
// Brings Right shoulder down
void right_shoulder_go_home(){  //pin 2 - on servo board 
Serial.println("Entered right_shoulder_go_home() ");
 pwm.setPWM(2,0,RT_SHLDR_GO_HOME_PWM ); // greater is up, less is down
 

}

/////////////////////////////////////////////////////////////////////////////////
// brings right elbow outwards
void right_elbow_go_home(){  //pin 3 - on servo board 
  Serial.println("Entered right_elbow_go_home() ");
   pwm.setPWM(3,0,RT_ELBW_GO_HOME_PWM); // less is inward
   
}

/////////////////////////////////////////////////////////////////////////////////
// Brings Left shoulder down
void left_shoulder_go_home(){  //pin 4 - on servo board 
  pwm.setPWM(4,0,LFT_SHLDR_GO_HOME_PWM); // less is up, greater is down
  
Serial.println("Entered left_shoulder_go_home() ");
}


/////////////////////////////////////////////////////////////////////////////////
// brings left elbow outwards
void left_elbow_go_home(){  //pin 5 - on servo board 
  Serial.println("Entered left_elbow_go_home() ");
 pwm.setPWM(5,0,leftElbow5); // greater is outwards
 
}

////////////////////////// TOUCH DOWN ////////////////////////////////////////////
// Monkey Touchdown
void monkey_touchdown(){  //pin 5 - on servo board 
  Serial.println("Entered monkey touch down() ");

  touchdown_head_bottom_servo_();
  touchdown_head_top_servo_();
  touchdown_right_elbow_go_home();
  left_elbow_go_home(); 
  touchdown_both_arms_go_up();  
  touchdown_wave();
  touchdown_wave();
  touchdown_wave();
//  right_arm_go_up();
  touchdown_both_arms_go_down();
  pwm.setPWM(4,0,4096);
//  touchdown_right_arm_go_down();
  pwm.setPWM(2,0,4096);

  }
  /////////////////////////////////////////////////////////////////////////////////
// MONKEY LEFT ARM GO UP
// assumes left elbow in home position
void left_arm_go_up(){  
    for (left = leftShoulder4; left >= leftShoulder4 - 400; left -= 1) { // 1ft move up shoulder
      //Range = -400
   //if(left>100){
   pwm.setPWM(4,0,left);
   //pos2=300;
   delay(7);
   }
  Serial.println("Entered monkey left arm go up() ");


  }

  ///////////////////////////////////////////////////////////////////////////////////
  void touchdown_both_arms_go_up(){  
    right = rightShoulder2;
    head_up = topHead1;
    for (left = leftShoulder4; left >= leftShoulder4 - 350; left -= 10) { // 1ft move up shoulder 
      pwm.setPWM(4,0,left);
      pwm.setPWM(2,0,right);   
      pwm.setPWM(1,0,head_up);   
      right += 20;
      head_up += 4; 
      delay(7);
    }
  Serial.println("Entered monkey touchdown left arm go up() ");


  }
  void touchdown_right_arm_go_down(){ 
  /////////////////////////////////////////////////////////////////////////////////
  // touchdown MONKEY RIGHT ARM GO down
  //int botHead0 = 300;
  //int topHead1 = 350;
  //int rightShoulder2 = 275;
  //int rightElbow3 = 400;
  //int leftShoulder4 = 400;
  //int leftElbow5 = 125;
  //int elbowRange = 50;
  // assumes right elbow in home position
 
  Serial.println("Entered touchdown right arm go down() ");
  
  for (right = rightShoulder2+720; right >= rightShoulder2+50; right -= 1) { // move down right shoulder  //690
    //Range = + 710
    pwm.setPWM(2,0,right); //right shoulder servo head
    
    delay(4);
    
  }
    for (right = rightShoulder2+50; right >= rightShoulder2; right -= 1) { // move down right shoulder  //690
    //Range = + 710
    pwm.setPWM(2,0,right); //right shoulder servo head
    
    //delay(1);
    
  }
  }
// MONKEY RIGHT ARM GO UP
// assumes right elbow in home position
void right_arm_go_up(){  
  Serial.println("Entered monkey right arm go up() ");
  
  for (right = rightShoulder2; right <= rightShoulder2 + 710; right += 1) { // move up right shoulder  //690
    //Range = + 710
    pwm.setPWM(2,0,right); //right shoulder servo head
    
    delay(5);
    
  }
  }
/////////////////////////////////////////////////////////////////////////////////
void touchdown_wave() {
  //delay (1000);
  /////////////////////// hand wave 1st cycle//////////////////////////////////////////////
//hand wave 1st cycle:
// go OUT first
right = rightElbow3;
for (left = leftElbow5; left <= leftElbow5 - 40; left -= 2) { 
   pwm.setPWM(5,0,left);
   pwm.setPWM(3,0,right);
   right -= 2;
   delay(7);
}
right -= 2;  
for (left = leftElbow5; left <= leftElbow5 - 40 + elbowRange; left += 2) { 
   pwm.setPWM(5,0,left);
   pwm.setPWM(3,0,right);
   right += 2;  
   delay(7);
}
right += 2;  
for (left = leftElbow5 - 40 + elbowRange; left >= leftElbow5; left -= 2) { 
   pwm.setPWM(5,0,left);
   pwm.setPWM(3,0,right);   
   right -= 2;  
   delay(7);
}
   pwm.setPWM(5,0,leftElbow5);
   pwm.setPWM(3,0,rightElbow3);   
}
  
//  ///////////////////////////////////////////////////////////////////////////////////
//int botHead0 = 300;
//int topHead1 = 350;
//int rightShoulder2 = 275;
//int rightElbow3 = 400;
//int leftShoulder4 = 400;
//int leftElbow5 = 125;
//int elbowRange = 50;

  void touchdown_both_arms_go_down(){  
    right = rightShoulder2+700;
    head_up = topHead1 + 140;
    for (left = leftShoulder4 - 350; left <= leftShoulder4 ; left += 10) { // 1ft move DOWN shoulder  
      pwm.setPWM(4,0,left);
      pwm.setPWM(2,0,right);
      right -= 20;
      if (right < rightShoulder2) right = rightShoulder2;
      delay(7);
   }
  Serial.println("Entered monkey touchdown left arm go down() ");
  
//  for (left = leftShoulder4 - 15; left <= leftShoulder4 ; left += 1) { // 1ft move DOWN shoulder  
////      //Range = -300
//   //if(left>100){
//   pwm.setPWM(4,0,left);
//   //pos2=300;
//   delay(12);
//   }

    delay(350);
    for (head_up = topHead1 + 140; head_up >= topHead1; head_up -= 4) {
      pwm.setPWM(1,0,head_up);
      delay(7);
    }
     pwm.setPWM(1,0,4096);
  }
  /////////////////////////////////////////////////////////////////////////////////
// brings right elbow outwards for touchdown
void touchdown_right_elbow_go_home(){  //pin 3 - on servo board 
   Serial.println("Entered touchdown_right_elbow_go_home() ");
   pwm.setPWM(3,0,rightElbow3); // 390-400 LOOKS GOOD
}

//////////////////////////////////////////////////////////////////////////////////////
void touchdown_head_bottom_servo_(){ //MOVES HEAD TO THE MIDDLE POSITION (HORIZONTALLY)
  Serial.println("Entered touchdown_head_bottom_servo_() ");
  pwm.setPWM(0,0,botHead0); // 390-400 LOOKS GOOD
  
}
////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////
void touchdown_head_top_servo_(){   ////MOVES HEAD DOWN A LITTLE (VERTICALLY)
  Serial.println("Entered touchdown_head_top_servo_() ");
   pwm.setPWM(1,0,topHead1); // 390-400 LOOKS GOOD
  
}
////////////////////////////////////////////////////////////////////////////////////////



// Return monkey to its home pose and power down servo PWM. Called at the end of
// MONKEY 01/02/04/05/06 (each individual move). Previously a no-op, which left servos
// powered at whatever position the move ended at (drained battery, risk of cooking a
// stalled servo). Now delegates to stablize_slowly() which writes home positions then
// kills PWM via stop_monkey_servos().
void monkey_rest(){
    stablize_slowly();
}

///////////////////////////////////////////////////////////////////////////////////////
//this issues the command to turn the video OFF

void video_off(){
  Serial.println("Entered video_off()");
      digitalWrite(VIDEO_pin, LOW);
}

///////////////////////////////////////////////////////////////////////////////////////
//this issues the command to turn the video ON
// This should then test to see if the video is operating
// if yes then set the level to 
// if no, then report that the video is "not on"
// make this a function which returns 1 = good, -1 = bad not turned on

// Turn video ON. There is currently NO hardware path to verify the camera actually came up
// (DS1809 has no readback, and there is no sense line on the 12V rail). When that lands,
// add the check here and have this function return success/failure.
void video_on(){
    Serial.println("Entered video_on()");
    itest = videoRFLevel;          // initially, videoRFLevel = 15, about 1/3 of the max level of 63 (not linear)
    DS1809_SettoZero();                   // take it to zero,
    set_video_level(itest);               // and then restore it to what it was before it was turned off - or to 15 on first turn-on.
    digitalWrite(VIDEO_pin, HIGH);        // This turns on the 13.8V power supply to the video components LJR 10/21
    delay (100);
}

///////////////// Set Video Level ////////////////////////////////////////////////
//
//
void set_video_level(int requested_level){
   int current_loc = 0;
   int diff = 0;
      Serial.println("Entered set_video_level()");

// find out where it is
// then find out difference i.e. how many to go
// then call CountUp or CountDown

// 
//     current_loc = DS1809_GetTrakVal();
     current_loc = videoRFLevel;                              // GLOBAL variable   LJR 10/21
     requested_level = constrain( requested_level, 0, 63);
     videoRFLevel = requested_level;                         // GLOBAL variable      LJR 10/21
     // Persist so a mid-flight brown-out doesn't lose the ground operator's last setting.
     // EEPROM.update() only writes if the byte changed — keeps cell wear minimal.
     EEPROM.update(EEPROM_VIDEO_LEVEL_ADDR, (uint8_t)requested_level);
     Serial.print("Video Level to be set to ");
     Serial.println(requested_level);
     Serial.print("Current Level is ");
     Serial.println(current_loc);

     if (requested_level < current_loc) {
      diff = current_loc - requested_level;
      Serial.print("Down ");
      Serial.println(diff);
      DS1809_CountDown (diff);
     }
     
     if (requested_level > current_loc) {
      diff = requested_level - current_loc;
      Serial.print("Up ");
      Serial.println(diff);
      DS1809_CountUp (diff);  
     }
      if (requested_level == current_loc) {
      Serial.print("requested video level is alread set to that value.");  
     }
     DS1809_SetTrakVal(requested_level);             // Set the Tracked Value "DS1809_Val" to the new value.    LJR 10/21
     Serial.print("Video Level is ");
     Serial.println(videoRFLevel);              // GLOBAL variable      LJR 10/21
      
//      //Wire.write(video_level);                        // this section below commented out - we no longer use this chip   LJR 10/21
//      ds3502.setWiper(video_level); 
//
////      read the pin 
////    wiper_read = analogRead(WIPER_pin);
////    Serial.print ("wiper read value from pin =  ");
////    Serial.println (wiper_read);
//      
//      Serial.println("Reading the wiper");
//      wiper_value = ds3502.getWiper();
//      Serial.print ("wiper read value from get.Wiper() =  ");
//      Serial.println (wiper_value);
//      if(wiper_value != video_level) {
//        Serial.println ("getWiper() read not equal to video level sent");
//        Serial.println ("sending MONKEY 17 error message");
//        Serial2.print ("MONKEY 17");
//      }
//      else {
//        Serial.println ("getWiper() read IS EQUAL to video level sent");
//        Serial.println ("sending MONKEY 11 done message");
//        Serial2.print ("MONKEY 11");
//      }
 
/* test software

 for (i=21; i<84; i++){
 video_level = i;
      //Wire.write(video_level);
 ds3502.setWiper(i); 
 wiper_read = analogRead(WIPER_pin);
 Serial.print ("i =  ");
 Serial.print (i);
 Serial.print (" wiper read value from pin =  ");
 Serial.print (wiper_read);
 Serial.print("  ");     
 Serial.print("Reading the wiper"); 
 wiper_value = ds3502.getWiper();
 Serial.print ("wiper read value from get.Wiper() =  ");
 Serial.println (wiper_value);
 }
 */
      
}

/////////////////////////////// VCC self-measurement (band-gap) ////////////////////////////////
// Read the actual VCC by measuring the 1.1V band-gap reference against AVCC.
// Why: every analogRead() with the default reference returns (VCC * sample/1023). When
// VCC sags during heavy servo current draw or as the battery discharges, the battery
// voltage reading and both LM135 temperature readings all drift the same direction —
// the very telemetry you'd use to diagnose the sag becomes unreliable as it sags.
// Calling readVccMillivolts() per-read corrects for this within ~1 mV.
// AVR-specific: when migrating to Teensy or ESP32, replace with the platform's
// equivalent VCC measurement (or use an external precision reference).
long readVccMillivolts() {
    uint8_t admux_saved = ADMUX;
    uint8_t adcsrb_saved = ADCSRB;
    // AVCC as reference, MUX = 11110 = ADC channel 30 (1.1V internal band-gap)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    ADCSRB &= ~_BV(MUX5);
    delay(2); // band-gap settling
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC)) ; // wait for conversion
    long result = ADC;
    ADMUX = admux_saved;
    ADCSRB = adcsrb_saved;
    if (result == 0) return 5000;       // sanity fallback if ADC didn't convert
    // VCC_mV = 1.1V * 1023 * 1000 / ADC_reading = 1125300 / ADC_reading
    return 1125300L / result;
}

/////////////////////////////// this routine reads the battery voltage ///////////////////////////////////////////////////
int get_bat_voltage(){
// battery voltage
// voltage = 0.0 - 5.0v
// ADC = 0.0 - 1023
// Battery voltage range is 0.0 - 16.8 volts
// There will be a divide by 4 circuit between the pin and the sensor
// Actual battery voltage = (calculated voltage read) * (Divide by circuit = 4) 
//       y      =  m       *  X            + b
// voltage read = (5/1023) * (ADC reading) + 0
// Take 3 readings 1/10 second apart.
// convert the readings to voltage
// convert the voltages to battery voltages via DivideByCircuit factor
// sum the voltages then take arithmetic mean by dividing by 3
// multiply by 10 as we are sending the voltage and the first decimal digit
// then convert to integer and send the integer of 10 times the voltage
// Battery Voltage defines
// int Vbat = 0;    // battery voltage read
// const float DivideByCircuit = 3.6; // divide by circuit factor
// int BatteryVoltagePin = A2;  // Battery voltage connected to analog pin 2, outside leads to ground and +5V
// int BatRead1  = 0;           // 1st value read of battery
// float BatV1   = 0.0;         // variable to store the 1st value read
// int BatRead2  = 0;           // 2nd value read of battery
// float BatV2   = 0.0;         // variable to store the 1st value read
// int BatRead3  = 0;           // 3rd value read of battery
// float BatV3   = 0.0;         // variable to store the 1st value read
// float BatSum  = 0.0;         // Battery Sum voltage used to calculate arithmetic mean
// float BatMean = 0.0;         // arithmetic mean of battery voltage
// int TenTimes  = 0;           // This is the voltage multiplied by 10 and then made into an integer.
//
//tee = millis();
//Serial.print ("time before read = ");
//Serial.println (tee);


// Three consecutive reads on the same channel — ADC settling is ~100us, no need for
// the 100ms delays that used to be here (they blocked Bluetooth RX during MONKEY 12
// telemetry replies, exactly when the ground station was listening).
// Discard the first read after the mux switches in from another channel.
(void)analogRead(BatteryVoltagePin);
BatRead1 = analogRead(BatteryVoltagePin);
BatRead2 = analogRead(BatteryVoltagePin);
BatRead3 = analogRead(BatteryVoltagePin);

// Use measured VCC instead of the hardcoded 5.0 V. v_per_count is volts-per-ADC-count.
long vcc_mv = readVccMillivolts();
float v_per_count = (float)vcc_mv / 1023.0f / 1000.0f;

BatV1 = (float)BatRead1 * v_per_count * DivideByCircuit;
BatV2 = (float)BatRead2 * v_per_count * DivideByCircuit;
BatV3 = (float)BatRead3 * v_per_count * DivideByCircuit;
//Serial.println(BatV1);
//Serial.println(BatV2);
//Serial.println(BatV3);
BatSum = BatV1+BatV2+BatV3;
BatMean = (BatSum/3.0);
TenTimes = (int)(BatMean * 10.0);

return (TenTimes);

}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////// this routine reads the inside temperature //////////////////////////////////////////////////////
// Read appropriate Data
// Inside Temp is read on pin A0
// voltage = 0.0 - 5.0v
// ADC = 0.0 - 1023
// Temperature is 0 - 500 K (degrees Kelvin)
// We should send USIP T degrees C
// Actual temp = (calculated voltage read)
//       y      =  m       *  X            + b
// temp read = (500/1023) * (ADC reading) + 0
// convert the readings to temp
// int InsideTempPin = A0; // Inside Temp sensor is connected to analog pin 0.
// int InsideT = 0; // Inside temp integer read
// float TRaw raw temperature read
// float TempK is raw temperature converted to degrees Kelvin
// float TempC is Temperature Degrees Centigrade

 int get_inside_temp(){
      int T_inside = 0;
      TRaw  = analogRead(InsideTempPin);  // read the inside temp
      Serial.println(TRaw);
      // LM135H produces 10 mV per Kelvin. Was: TempK = TRaw / 2.046 (assumed VCC = 5.0V).
      // Now: scale by measured VCC so the reading stays accurate as the rail sags.
      long vcc_mv = readVccMillivolts();
      TempK = (TRaw * (float)vcc_mv) / (1023.0f * 10.0f);
      TempC = TempK - 273.15f;
      T_inside = (int) TempC;
 return (T_inside);
      }
 

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// this routine reads the Transmitter Temp ///////////////////////////////////////////////////
// Xmtr Temp is read on pin A1
// voltage = 0.0 - 5.0v
// ADC = 0.0 - 1023
// Temperature is 0 - 500 K (degrees Kelvin)
// We should send USIP T degrees C
// Actual temp = (calculated voltage read)
//       y      =  m       *  X            + b
// temp read = (500/1023) * (ADC reading) + 0
// convert the readings to temp
// int XmtrTempPin = A1; // Inside Temp sensor is connected to analog pin A1.
// int XmtrT = 0;   // transmitter temp Integer read
// float XTRaw  = 0.0; //analogRead(XmtrTempPin);      // read transmitter temperature
// float XTempK = 0.0; // = (XTRaw/2.046);
// float XTempC = 0.0; // = XTempK - 273.15;
// int xmtr_temp = 0; //  = (int) XTempC;

 int get_xmtr_temp(){   // this routine reads the transmitter temperature
      int xmtr_temp = 0;
      XTRaw  = analogRead(XmtrTempPin);      // read transmitter temperature
      Serial.println(XTRaw);
      // LM135H = 10 mV/K, scaled by measured VCC (see get_inside_temp).
      long vcc_mv = readVccMillivolts();
      XTempK = (XTRaw * (float)vcc_mv) / (1023.0f * 10.0f);
      XTempC = XTempK - 273.15f;
      xmtr_temp = (int) XTempC;
 return (xmtr_temp);
      }
//////////////////////////////////////////////////////////////////////////////
/////////////////////////////HEAD NOD SLOWLY/////////////////////////////////////////////////////////////////////////////
///////////////////// Tan Monkey Home Positions/////////////////////
//int botHead0 = 300;
//int topHead1 = 350;
//int rightShoulder2 = 275;
//int rightElbow3 = 400;
//int leftShoulder4 = 400;
//int leftElbow5 = 125;
//#define SERVOMIN 150 // min pulse lenghth count
//#define SERVOMAX 600 // max pulse lenghth count
//uint8_t servo1neck = 0;
//int pos=0;
//float fpos = 0.0;
//int pos2=300;
//int right=0;
//int right2=300;
//int right3=-60;
//int left=0;
//int left2=0;
//int left3=0;
//int elbowRange = 50;

void servohead_no_nod_slowly() {
//Servo1 left and right motion

 pwm.setPWM(1,0,topHead1); //top servo head UP/DOWN
      delay(5);
      
for (pos = botHead0; pos <= botHead0 + 20; pos += 1) { // goes from 0 degrees to 180 degrees
   //Range = 20
    pwm.setPWM(0,0,pos); //bottom servo head   
    delay(10);  
  

  }

for (pos = botHead0 + 20; pos >= botHead0 - 20; pos -= 1) { 
  pwm.setPWM(0,0,pos); //bottom servo head 
  delay(10); 
}  //1st round

//2nd round:


//
for (pos = botHead0 - 20; pos <= botHead0; pos += 1) { // goes from 0 degrees to 9? degrees  
 pwm.setPWM(0,0,pos); //bottom servo head
    //pos2=SERVOMIN;
    delay(10);
    pwm.setPWM(1,0,topHead1); //top servo head
      delay(5);
  }

for (pos = botHead0; pos <= botHead0 + 20; pos += 1) { // goes from 0 degrees to 180 degrees
   //Range = 90
 pwm.setPWM(0,0,pos); //bottom servo head
   delay(30);   
 pwm.setPWM(1,0,topHead1); //top servo head
      delay(5);
  }
for (pos = botHead0 + 20; pos >= botHead0 - 20; pos -= 1) { 
  pwm.setPWM(0,0,pos);
  delay(30);
}  //1st round

//2nd round:
for (pos = botHead0 - 20; pos <= botHead0; pos += 1) { // goes from 0 degrees to 180 degrees   
 pwm.setPWM(0,0,pos); //bottom servo head
    //pos2=SERVOMIN;
    delay(30);
    pwm.setPWM(1,0,topHead1); //top servo head
      delay(5);
  }
  
}//end of no nod slowly

////////////////////////// DS1809 CHIP PROCEDURES ///////////////////
// Set the DS1809 chip to Zero for start  Low-active "pindown" for 65 ticks
void DS1809_SettoZero(){
  for (x=1; x<=65; x++)  {
    digitalWrite(DS1809_pindown,LOW);  // TICK COUNTS ON LOW
    delay(2);
    digitalWrite(DS1809_pindown,HIGH); // remain in HIGH state
    delay(2);
  }
    DS1809_SetTrakVal(0);
    videoRFLevel = 0;
}
///////////////DS1809 COUNT UP N TIMES /////////////
void DS1809_CountUp(int number){
     
//  if (number > 32){          // Took care of this in the calling routine intead
//    number = 32;
//  }
  for (x=1; x <= number; x++)  {
    digitalWrite(DS1809_pinup,LOW);  // TICK COUNTS ON LOW
    delay(2);
    digitalWrite(DS1809_pinup,HIGH);  // remain in HIGH state       Corrected error - was pindown - so was not counting up    LJR 10/21
    delay(2);
  }
//  itest = DS1809_GetTrakVal();         // Took care of this in the calling routine intead   LJR 10/21
//  itest += number;
//  DS1809_SetTrakVal(itest);
}
///////////////DS1809 COUNT DOWN N TIMES ///////////
void DS1809_CountDown(int number) {
//    if (number >32){          // Took care of this in the calling routine intead   LJR 10/21
//    number = 32;
//  }
  for (x=1; x <= number; x++)  {
    digitalWrite(DS1809_pindown,LOW);  // TICK COUNTS ON LOW
    delay(2);
    digitalWrite(DS1809_pindown,HIGH);  // remain in HIGH state
    delay(2);
  }
//  itest = DS1809_GetTrakVal();         // Took care of this in the calling routine intead   LJR 10/21
//  itest -= number;
//  DS1809_SetTrakVal(itest);
}
////////////////////GET DS1809 CURRENT VALUE ///////////////
int DS1809_GetTrakVal(){
  return (DS1809_Val);  // returns actual value not bound by 32
}

/////////////////////////////
////////////////////SET DS1809 CURRENT VALUE ///////////////
void DS1809_SetTrakVal(int number){
//   if (number >32){          // (0 <= number <= 32)   "requested_value" in the calling routine now constrained 0 to 63     LJR 10/21
//    number = 32;
//  }
    DS1809_Val = number;
}
////////////////////////////////////////////////////////////////////////

/////////////////////stop_monkey_servos//////////////////////////////////////
void stop_monkey_servos(){
  Serial.print ("Entered stop monkey servos routine ");
   pwm.setPWM(0,0,4096); //bottom servo head 
   pwm.setPWM(1,0,4096); //top servo head UP/DOWN
   pwm.setPWM(2,0,4096);

    pwm.setPWM(3,0,4096);

    pwm.setPWM(4,0,4096);

    pwm.setPWM(5,0,4096);
  //  exit (0);
   
}
/////////////////////////////////////////END////////////////