/* GROUND CONTROL SOFTWARE 
 *  This program runs in the Ground Control Station for the USIP Monkey balloon
 *  It allows the operator to control the monkey's movement, his voice, and music 
 *  chosen to complement the journey.
 */
// include the SoftwareSerial library so we can use it to talk to the Emic 2 module
#include <SoftwareSerial.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define rxPin   10  // Serial input (connects to Emic 2's SOUT pin)
#define txPin   11  // Serial output (connects to Emic 2's SIN pin)
#define LEDPin  13  // Most Arduino boards have an on-board LED on this pin


// set up a new serial port
SoftwareSerial emicSerial =  SoftwareSerial(rxPin, txPin);

// Global variables

//Callsign of balloon
const String callSign = "AB1JC-11";

//Pins
int ledPin = 13;

// Keyboard Pins
int row0 = 22;
int row1 = 24;
int row2 = 26;
int row3 = 28;
int col0 = 23;
int col1 = 25;
int col2 = 27;
int col3 = 29;
int abortPin = 31;

//Lamp pins
int lite00 = 30;
int lite01 = 32;
int lite02 = 34;
int lite03 = 36;
int lite13 = 38;
int lite23 = 40;
int lite33 = 42;
int lightPin;

// Analog input pins for the 3 pots
int vidLvlPin = A3;
int voiceLvlPin = A4;
int auxLvlPin = A5;
// ...and those are read into these integers
int vidLevel;
int voiceLevel;
int auxLevel;
float videoLevel;
int lastVoiceLevel;

// generic integers
int i;
int j;
int k;
int row_num = 3;
int firstButton;
boolean buttonDown[4];

String tempStr;
String baseCmd = "MONKE ";
unsigned long int loopTime0 = 0;
boolean readtime = false;
boolean shift1 = false;
boolean shift2 = false;
int shiftNum;

// Related to Serial 1 inbound:
boolean string1Complete = false;
boolean string2Complete = false;
const String ONE_Str = "1";
const String ZERO_Str = "0";
const String COMMA_Str = ",";
const String COLON_Str = ":";
const String DOT_Str = ".";
const String oneSpace = " ";
String inStr1 = "";        // a string to hold incoming data from TT4
String inStr2 = "";        // a string to hold incoming command from the Telemetry Processor
String inString = "";      // copied from inStr1, we will parse this string
String cmdString = "";
String subStr1 = "";

int sPtr1;
int sPtr2;


char TelemBit[9] = { '0', '0', '0', '0', '0', '0', '0', '0' };

boolean flash = false;
boolean monkeyBusy;
boolean voiceWaiting;
boolean voiceOK = true;
boolean speaking = false;
boolean abortFlag = false;
boolean abortIsDone = false;
// boolean introFlag;

// the green buttons...
boolean videoOn = false;
boolean speechOn = true;
boolean musicOn = false;
boolean auxFunction3_On = false;
boolean functionOn[4] = {false,false,false,false};
int offAction[4] = {26,29,28,20};
int onAction[4] = {25,29,28,19};
unsigned long int moveTime[6] = {0,3300,3400,6700,10600,9000}; 


//videoOn = functionOn[3];
//speechOn = functionOn[2];
//musicOn = functionOn[1];
//auxFunction3_On = functionOn[0];

char speech[130];
unsigned long int lastMillis = 0;
unsigned long int voiceTime;
unsigned long int startSpeak;
unsigned long int waitTime;
unsigned long int flashTime = 0;
unsigned long int allMoveSpeakTime = 0;
unsigned long int timeTick = 0;


int gpsHours = 0;
int gpsMinutes = 0;
int gpsSeconds = 0;
boolean gpsSync = false;

int misHours = 0;
int misMinutes = 0;
int misSeconds = 0;

int LatDeg;
float LatMin;
int LonDeg;
float LonMin;
float gpsAlt;
float Pressure;

int voiceDelay;
int cmdWaiting = 99;
int cmdExecuting = 99; 
int speechNumber;
int cmdNumSpeeches;
boolean extraSpeechFlag = false;
int allMoveSpeakBase;
int allMoveSpeakNum = 0;
byte spoke[50];
boolean readAltitudeFlag = false;
boolean readTempFlag = false;
boolean readMovesFlag = false;


// telemetry variables and display
long int spokenAltitude = 23650;
float heading = 310.5;
char wordHeading[19] = {"N NEE SES SWW NWN "};
char letterHeading[3] = "  ";
float windSpeed = 43.2658;
int verticalSpeed = 11;
float EbayBatt = 7.8;
float MonkeyBatt = 15.8;
int OTemp = -37;
int ITemp1 = -3;
int ITemp2 = 85;
int TxTemp = 132;
int dataDisplay = 5;
//int lastPage = 3;
int intHalf;
int fracHalf;
float dbLevel;
float temp0;
int temp1;


struct button {
  int cnt;
  bool switchOn;
  bool lastSwitchOn;
//  long unsigned int lastChange;
  int CMD[4];
};
struct button btn[17];


struct command {
  char cmdName[8];
  unsigned int duration;
  int cmdVoiceDelay;
  int numSayings;
  int moveNumber;
};

/////////////////////////////////////////
// These are the REAL times it takes to execute each monkey command:
// Center 00 - real quick, less than 1/2 sec
// Nod 01 - 3.4 sec
// Shake 02 - 3.3 sec
// Thumbs Up 03 - 6.7 sec
// Scan Panel 04 - 10.6 sec
// Wave 05 - 9 sec
// Chest Pound 06 - 8.3 sec
// Touchdown 07 - 7.5 sec
// moveAll 08 - 36.4 sec

struct command presentCommand;
const struct command cmd[50] PROGMEM {
  "Center ",750,0,0,0,        // 0
  "NodHead",3400,500,13,1,    // 1
  "ShakeHd",3300,500,8,2,     // 2
  "ThumbUp",6700,500,4,3,     // 3
  "ScanPnl",22000,0,8,4,      // 4
  "Wave   ",10000,1,4,5,      // 5
  "ChstPnd",12000,100,4,6,    // 6
  "Dizzy  ",9500,100,3,2,     // 7
  "FeelGd ",6700,100,5,3,     // 8
  "FeelBad",9000,100,3,98,    // 9
  "NotUsed",3400,500,0,99,    // 10
  "NotUsed",3300,500,0,99,     // 11
  "NotUsed",6700,250,0,99,     // 12
  "SCANPNL",22000,0,8,4,      // 13
  "NotUsed",10000,100,0,99,    // 14
  "NotUsed",12000,100,0,99,    // 15
  "TOUCHDN",7500,2,1,7,       // 16
  "MOVEALL",36400,0,6,8,      // 17
  "INTRO  ",9500,0,2,5,      // 18
  "Vid_ON ",5000,2,1,19,      // 19
  "Vid_OFF",5000,2,1,20,      // 20
  "Display",250,2,0,99,       // 21  changes data on small display
  "Abort??",7000,2,1,98,      // 22  Abort Warning
  "setVid ",200,2,0,99,       // 23
  "IDENT  ",5600,2,1,98,      // 24
  "Aux3On ",6000,2,1,98,      // 25
  "Aux3Off",6000,2,1,98,      // 26
  "ABORT!!",7000,2,1,98,      // 27  Abort command
  "Voice01",250,2,0,99,       // 28  Voice On/Off
  "Music01",250,2,0,99,       // 29  Music On/Off
  "Cancel!",4000,2,1,98,      // 30  Cancel ABORT 
  "AbtOFF.",4000,2,1,98,      // 31  Switch turned off after abort
  "Cmd. 32",2500,2,1,98,      // 32  Thank you for donation
  "Cmd. 33",2500,2,0,99,      // TBD
  "Cmd. 34",2500,2,0,99,      // TBD
  "Cmd. 35",2500,2,0,99,      // TBD
  "Cmd. 36",2500,2,0,99,      // TBD
  "Cmd. 37",2500,2,0,99,      // TBD
  "Cmd. 38",2500,2,0,99,      // TBD
  "Cmd. 39",2500,2,0,99,      // TBD
  "Cmd. 40",2500,2,0,99,      // TBD
  "Cmd. 41",2500,2,0,99,      // TBD
  "Cmd. 42",2500,2,0,99,      // TBD
  "Cmd. 43",2500,2,0,99,      // TBD
  "Cmd. 44",2500,2,0,99,      // TBD
  "Cmd. 45",2500,2,0,99,      // TBD
  "Cmd. 46",2500,2,0,99,      // TBD
  "Cmd. 47",2500,2,0,99,      // TBD
  "Cmd. 48",2500,2,0,99,      // TBD
  "Cmd. 49",2500,2,0,99       // TBD
  };

    // Monkey speech strings in Program memory
  const char speeches[][128] PROGMEM = {
    "Why YES!", // 0
    "Yes absolootely!",   // 1
    "That is affirmative", // 2
    "I am nodding my head. Yes.",   // 3
    "I copy that - Roger.",    // 4
    "Roger that Control",  // 5
    "Roger Roger.",   // 6
    "Sir - Yes sir !",  // 7
    "Yes Control. Affirmative.",  // 8
    "Y. E. S. Yes",     // 9
    "Yes.",     //10
    "HAM to Control - Affirmative.",    //11
    "I think so Control",    //12
    "Negative",    // 0
    "No.",      // 1
    "Negative Control - Negative!",  // 2
    "No Control -  Negative.",    // 3
    "N. O.  No.",     // 4
    "I don't think so...",   // 5
    "Heck no!",    // 6
    " absolootely not!",    // 7
    "Thumbs UP!",     // 0
    "I'm giving that statement a Thumbs up!",   // 1
    "I feel great. It's a Thumbs up day...",  // 2
    "Thumbs up to all you guys down there on the ground...",    // 3
    "Scanning the instrument Panel -  Let's see...",    // 0
    "Let me read the instrument panel...",    // 1
    "Reading the instruments...",    // 2
    "Time to check the panel...",    // 3
    "Like they say in flight school - you have to watch your instruments carefully...",   // 4
    "Let's check the data...",    // 5
    "Scan the panel",   // 6
    "Yes it's a bad pilot that doesn't check his instruments often...",   // 7
    "Hi kids! I am glad you could all come and watch me today!  What a trip this is! Up to the Stratosphere!",  //0
    "Hello every one who's watching me now.  I feel like a big hero way up in the sky!",   // 1
    "Here's waving at you kids! Can you see me on the monitor? I hope the T V is ok...",    // 2
    "Hey kids! Who's rooting for me down there? Say Go Monkey go! Say Go Ham!", //3
    "Watch me pound my chest! I feel great!",    // 0
    "Say - Who is the best robotic monkey around? It is H. A. M. HAM! Yipeeee!",  // 1 
    "Pounding my chest is something us monkeys like to do. It makes us feel powerfull!",   // 2
    "So. Who is way up high in the Stratosphere? Ham that's who! H. A. M. spells HAM!",   // 3
    "I'm feeling dizzy! Can you see on the monitor? The capsule is spinning around like crazy!",   //0
    "Well - I am a bit dizzy with all this spinning around.",  // 1
    "Say - the next time you guys design a capsule - try to design it so it doesn't spin so much. I get dizzy up here.",  //2
    "In general - I'm feeling pretty good. It's a beautiful day - and I'm flying way up into the Stratosphere!",   // 0
    "Very Good. I'm feeling good. There's nothing like a high altitude balloon ride to lift your spirits!",  // 1
    "HAM to control - I am feeling great! This flight is quite a rush!",   // 2
    "Control - I am feeling on top of the world! Whoopeee!",   // 3
    "I am really excited to be chosen to fly on the U S I P program. It feels great!",    // 4
    "I'll be all right as long as I have a ground crew coming to pick me up when I land.  They are coming - right?",  // 0
    "HAM to control - I feel kind of alone up here - so please keep talking to me on the radio.",  // 1
    "HAM to control - I'm a bit worried about falling! I'm pretty high up here - but my parachute will bring me down safely.",  // 2
//    "Why YES!", // 0
//    "Yes  absolootely!",   // 1
//    "That is affirmative", // 2
//    "I am nodding my head. Yes.",   // 3
//    "I copy that - Roger.",    // 4
//    "Roger that Control",  // 5
//    "Roger Roger.",   // 6
//    "Sir - Yes sir !",  // 7
//    "Yes Control. Affirmative.",  // 8
//    "Y. E. S. Yes",     // 9
//    "Yes.",     //10
//    "Affirmative.",    //11
//    "Aye Captain.",    //12
//    "Aye aye.",    //13
//    "I think so Control",    //14
//    "Negative",    // 0
//    "No no no!",      // 1
//    "Negative Control - Negative!",  // 2
//    "No Control - Negative.",    // 3
//    "N. O.  No.",     // 4
//    "I don't think so...",   // 5
//    "NAY Captain.  Nay.",   // 6
//    "Heck no!",    // 7
//    " absolootely not!",    // 8
//    "Thumbs UP!",     // 0
//    "I'm giving that statement a Thumbs up!",   // 1
//    "I feel great. It's a Thumbs up day...",  // 2
//    "Thumbs up to all you guys down there on the ground...",    // 3
    "Scanning the instrument Panel - Let's see...",    // 0
    "Let me read the instrument panel...",    // 1
    "Reading the instruments...",    // 2
    "Time to check the panel...",    // 3
    "Like they say in flight school - you have to watch your instruments carefully...",   // 4
    "Let's check the data...",    // 5
    "Scan the panel",   // 6
    "Yes it's a bad pilot that doesn't check his instruments often...",   // 7
//    "Hi kids! I am glad you could all come and watch me today!  What a trip this is! Up to the Stratosphere!",  //0
//    "Hello every one who's watching me now.  I feel like a big hero way up in the sky!",   // 1
//    "Here's waving at you kids! Can you see me on the monitor? I hope the Television is ok...",    // 2
//    "Hey kids! Who's rooting for me down there? Say Go Monkey go! Say Go Ham!", //3
//    "Watch me pound my chest! I feel great!",    // 0
//    "Who's the best robotic monkey around? It is H. A. M. HAM! Yipeeee!",  // 1 
//    "Pounding my chest is something us monkeys like to do. It makes us feel powerfull!",   // 2
//    "Who is way up high in the Stratosphere? Ham that's who! H. A. M. spells HAM!",   // 3
    "Oh my gosh! I'm falling! Falling! Boy I hope my parachute is working!",     //0
    "Watch my moves. Shaking my head No.",  //0
    "Nodding my head Yes.",  //  1
    "Giving the Thumbs up sign.",   // 2
    "Scanning the instrument panel.",   // 3
    "Waving to my sports fans.",   // 4
    "And pounding my chest.",   // 5
    "My name is Ham - that is H. A. M. - HAM. I am really excited to be chosen to fly on the U S I P program.",    // 0 
    "Hey kids! I am a robotic monkey. Can you see me waving on the monitor?",   // 1
    "Roger control - Video is now on.",   //0
    "Roger control - Video is now off.",   //0
    ":-)0This command requires confirmation. Press the flashing white button",   //0
    "Roger control - Identification command received.",   //0
    ":-)0Shift2 button is now a shift button",   //0
    ":-)0Shift2 button is now a regular function button.",   //0
    "Roger control - Received ABORT command - activating cutdown sequence.", //0
    ":-)0The ABORT sequence has been Cancelled.",   //0
    ":-)0The ABORT switch has been turned OFF.",     //0
    ":-)0Thank you SO much for your generous donation to the museum!", //32
//    ":-)1Thank you SO much for your generous donation to the museum!", 
//    ":-)0Synchronization with GPS time has been achieved.", 
//    ":-)3Thank you SO much for your generous donation to the museum!", 
//    ":-)4Thank you SO much for your generous donation to the museum!", 
//    ":-)5Thank you SO much for your generous donation to the museum!", 
//    ":-)6Thank you SO much for your generous donation to the museum!", 
//    ":-)7Thank you SO much for your generous donation to the museum!", 
//    ":-)8Thank you SO much for your generous donation to the museum!" // 40
//    ":-)0Command 40."
//    ":-)0Command 41.",
//    ":-)0Command 42.",
//    ":-)0Command 43.",
//    ":-)0Command 44.",
//    ":-)0Command 45.",
//    ":-)0Command 46.",
//    ":-)0Command 47.",
//    ":-)0Command 48.",
//    ":-)0Command 49." 
  };


////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {

  pinMode (row0, OUTPUT);
  pinMode (row1, OUTPUT);
  pinMode (row2, OUTPUT);
  pinMode (row3, OUTPUT);
  
  pinMode(col0, INPUT_PULLUP);
  pinMode(col1, INPUT_PULLUP);
  pinMode(col2, INPUT_PULLUP);
  pinMode(col3, INPUT_PULLUP);
  pinMode(abortPin, INPUT_PULLUP);

  pinMode(vidLvlPin,INPUT);
  pinMode(voiceLvlPin,INPUT);
  pinMode(auxLvlPin,INPUT);
  

  pinMode(lite00, OUTPUT);
  pinMode(lite01, OUTPUT);
  pinMode(lite02, OUTPUT);
  pinMode(lite03, OUTPUT);
  pinMode(lite13, OUTPUT);
  pinMode(lite23, OUTPUT);
  pinMode(lite33, OUTPUT);

  pinMode(ledPin, OUTPUT);
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

  // reserve String space
  inStr1.reserve(99);        // a string to hold incoming data from TT4
  inStr2.reserve(25);        // a string to hold incoming command from the Telemetry Processor
  inString.reserve(99);      // copied from inStr1, we will parse this string
  cmdString.reserve(25);
  subStr1.reserve(25);


// set variables to initial state

  digitalWrite(lite00,LOW);
  digitalWrite(lite01,LOW);
  digitalWrite(lite02,LOW);
  digitalWrite(lite03,LOW);
  digitalWrite(lite13,LOW);
  digitalWrite(lite23,LOW);
  digitalWrite(lite33,LOW);

  digitalWrite(row0,HIGH);
  digitalWrite(row1,HIGH);
  digitalWrite(row2,HIGH);
  digitalWrite(row3,HIGH);

//populate the button structures
  for (i=0;i<19;i++) {
  btn[i].cnt = 0;
  btn[i].switchOn = 0;
  btn[i].lastSwitchOn = 0;
//  btn[i].lastChange = 0;
  }
  // and now map the buttons to commands
  btn[0].CMD[0] = 1;         // Nod Head - YES /Nod Head - YES / TBD / TBD
  btn[0].CMD[1] = 10;
  btn[0].CMD[2] = 32;
  btn[0].CMD[3] = 41;
  
  btn[1].CMD[0] = 2;         // Shake Head - NO / Shake Head - NO /  TBD / TBD
  btn[1].CMD[1] = 11;
  btn[1].CMD[2] = 33;
  btn[1].CMD[3] = 42;
  
  btn[2].CMD[0] = 3;         // Thumbs UP / Thumbs UP/  TBD / TBD
  btn[2].CMD[1] = 12;
  btn[2].CMD[2] = 34;
  btn[2].CMD[3] = 43;
  
  btn[3].CMD[0] = 19;         // Video ON: and also command 20: Video OFF
  btn[3].CMD[1] = 19;
  btn[3].CMD[2] = 19;
  btn[3].CMD[3] = 19;
    
  btn[4].CMD[0] = 4;         // Scan the Instrument Panel with Alt&Batt / Scan with Temps / 
  btn[4].CMD[1] = 13;
  btn[4].CMD[2] = 35;
  btn[4].CMD[3] = 44;

  btn[5].CMD[0] = 5;         // Left Hand Wave / Left Hand Wave / TBD / TBD
  btn[5].CMD[1] = 14;
  btn[5].CMD[2] = 36;
  btn[5].CMD[3] = 45;
  
  btn[6].CMD[0] = 6;         // Monkey Chest Pound / Monkey Chest Pound / TBD / TBD
  btn[6].CMD[1] = 15;
  btn[6].CMD[2] = 37;
  btn[6].CMD[3] = 46;
  
  btn[7].CMD[0] = 28;         // Controls Monkey Voice On/Off
  btn[7].CMD[1] = 28;
  btn[7].CMD[2] = 28;
  btn[7].CMD[3] = 28;

  btn[8].CMD[0] = 7;         // feel dizzy / Touchdown! Falling! / TBD / TBD
  btn[8].CMD[1] = 16;
  btn[8].CMD[2] = 38;
  btn[8].CMD[3] = 47;

  btn[9].CMD[0] = 8;         // Feel Good / Do all Moves / TBD / TBD
  btn[9].CMD[1] = 17;
  btn[9].CMD[2] = 39;
  btn[9].CMD[3] = 48;

  btn[10].CMD[0] = 9;         // Feel Bad / INTRO / TBD / TBD
  btn[10].CMD[1] = 18;
  btn[10].CMD[2] = 40;
  btn[10].CMD[3] = 49;

  btn[11].CMD[0] = 29;         // Controls Music On/Off
  btn[11].CMD[1] = 29;
  btn[11].CMD[2] = 29;
  btn[11].CMD[3] = 29;

  btn[12].CMD[0] = 21;         //Change Display / CENTER MONKEY / TBD / TBD === ALSO CONFIRM ABORT 
  btn[12].CMD[1] = 0;
  btn[12].CMD[2] = 99;
  btn[12].CMD[3] = 99;

  btn[13].CMD[0] = 99;         // not used - always shift key #1 
  btn[13].CMD[1] = 99;
  btn[13].CMD[2] = 99;
  btn[13].CMD[3] = 99;

  btn[14].CMD[0] = 23;         //  Auxiliary command 1 /  Auxiliary command 2 / OR USE AS SHIFT #2 
  btn[14].CMD[1] = 24;
  btn[14].CMD[2] = 99;         // (can't happen)
  btn[14].CMD[3] = 99;         // (can't happen)

  btn[15].CMD[0] = 25;         //  Auxiliary function 3 ON: USE button 14 as shift key #2 === ALSO CMD 26 to turn OFF 
  btn[15].CMD[1] = 25;
  btn[15].CMD[2] = 25;
  btn[15].CMD[3] = 25;

  btn[16].CMD[0] = 22;         //  SET ABORT FLAG, requiring confirmation - see also button 12 
  btn[16].CMD[1] = 22;
  btn[16].CMD[2] = 22;
  btn[16].CMD[3] = 22;
  

  

// flash the LEDs
  for (i=0; i<5; i++)  {
    digitalWrite(ledPin,HIGH);
    delay(100);
    digitalWrite(ledPin,LOW);
    delay(100);
  }
  Serial.begin(57600);       // USB. Debug
  Serial1.begin(19200);     // RS-232 Male. TT4 and Radio, 
  Serial2.begin(19200);      // RS-232 Female. PC, for telemetry Processor
  Serial3.begin(19200);     // TTL. 2-line x 16 character display
    // set the data rate for the SoftwareSerial port
  emicSerial.begin(9600);   // Monkey Voice

  Serial.println("");
  Serial.println("Starting...");
  //Home up display, turn on backlight, and init text-to-speech board 
  
  Serial3.write(12);
  delay(6);
  Serial3.write(17);
  defineUpArrow();    // Special display character
  defineDownArrow();  // Special display character
  defineDegree();     // Special display character
  Serial3.print("Ground Station  USIP ver 1.8 ");
  Serial3.write(1);
  Serial3.write(0);
  Serial3.write(2);
    /*
    When the Emic 2 powers on, it takes about 3 seconds for it to successfully
    initialize. It then sends a ":" character to indicate it's ready to accept
    commands. If the Emic 2 is already initialized, a CR will also cause it
    to send a ":"
  */
  emicSerial.print('\n');             // Send a CR in case the system is already up
  startSpeak = millis();
  while (emicSerial.read() != ':') {   // When the Emic 2 has initialized and is ready, it will send a single ':' character, so wait here until we receive it
    delay(10);                          // Short delay
    if (millis()> startSpeak + 6000) {
      voiceOK = false;
      Serial3.write(12);
      Serial3.write(17);
      Serial3.print("Voice Problem");
    }
  }
  delay(10);                          // Short delay
  emicSerial.flush();                 // Flush the receive buffer


  // Now up the monkey volume to max +18 dBm
  emicSerial.print("V18\n");
    emicSerial.print('\n');             // Send a CR in case the system is already up
  startSpeak = millis();
  while (emicSerial.read() != ':') {   // When the Emic 2 is ready, it will send a single ':' character
    delay(10);                          // Short delay
    if (millis()> startSpeak + 6000) {
      voiceOK = false;
      Serial3.write(12);
      Serial3.write(17);
      Serial3.print("Voice Problem");
    }
  }
  delay(10);                          // Short delay
  emicSerial.flush();                 // Flush the receive buffer

  delay(1000);
  digitalWrite(lite33,HIGH);
  delay(500);
  digitalWrite(lite33,LOW);
  digitalWrite(lite23,HIGH);
  delay(500);
  digitalWrite(lite23,LOW);
  digitalWrite(lite13,HIGH);
  delay(500);
  digitalWrite(lite13,LOW);
  digitalWrite(lite03,HIGH);
  delay(500);
  digitalWrite(lite03,LOW);
  digitalWrite(lite02,HIGH);
  delay(500);
  digitalWrite(lite02,LOW);
  digitalWrite(lite01,HIGH);
  delay(500);
  digitalWrite(lite01,LOW);
  digitalWrite(lite00,HIGH);
  delay(1000);
  digitalWrite(lite00,LOW);

  for (i=0;i<19;i++) {
    readCommand(i);
    printPresentCommand();
  }
   // Now initialize spoke array
  for (i=0;i<50;i++) {
    spoke[i] = 0;
  }
//  randomSeed(analogRead(7));
   
//Home up display and turn on backlight  
  Serial3.write(12);
  delay(6);
  Serial3.write(17);
  Serial3.write(148);
  Serial3.print("Panel now Active");
  
    // Set Speech ON
  speechOn = true;
  functionOn[2] = true;
  digitalWrite(lite23,HIGH);

  // and start the clock
  timeTick = millis()+1000;

}                    // End of setup
///////////////////////////////////////////////////////////////////////////////////////////////////
//define up and down arrows for small display
void defineUpArrow(){
  Serial3.write(249);
  Serial3.write(4);
  Serial3.write(14);
  Serial3.write(14);
  Serial3.write(31);
  Serial3.write(31);
  Serial3.write(4);
  Serial3.write(4);
  Serial3.write(4);
}
////////////////////////////////////////
void defineDownArrow(){
  Serial3.write(248);
  Serial3.write(4);
  Serial3.write(4);
  Serial3.write(4);
  Serial3.write(31);
  Serial3.write(31);
  Serial3.write(14);
  Serial3.write(14);
  Serial3.write(4);
}
/////////////////////////////////////////
void defineDegree(){    // for small display
  Serial3.write(250);
  Serial3.write(12);
  Serial3.write(18);
  Serial3.write(18);
  Serial3.write(12);
  Serial3.write(0);
  Serial3.write(0);
  Serial3.write(0);
  Serial3.write(0);
}
/////////////////////////////////////////
//Clear top line of small display
void clearTopLine() {
  Serial3.write(128);
  Serial3.print("                ");
  Serial3.write(128);
}
/////////////////////////////////////////////
// change the display page
void changeDisplay() {
  dataDisplay++;
  dataDisplay %= 7;   // number of pages
  reprintDisplay();
}
////////////////////////////////////////////
// Reprint the display (to be called every 1 second) to track data as it is changing
void reprintDisplay() {  
  clearTopLine();
  switch (dataDisplay) {
    case 0:
      Serial3.print("Alt ");
      Serial3.print(spokenAltitude);
      Serial3.print(" ");
      if (verticalSpeed < 0) {
        Serial3.write(0);
        Serial3.print(" ");
        Serial3.print(abs(verticalSpeed));
      }
      else {
        Serial3.write(1);
        Serial3.print(" ");
        Serial3.print(verticalSpeed);
      }
      break;

    case 1:
      separate(EbayBatt);
      Serial3.print("EBat");
      Serial3.print(intHalf);
      Serial3.print(DOT_Str);
      Serial3.print(fracHalf);
      Serial3.print(oneSpace);
      separate(MonkeyBatt);
      Serial3.print("MBat");
      Serial3.print(intHalf);
      Serial3.print(DOT_Str);
      Serial3.print(fracHalf);
      break;
      
    case 2:
      Serial3.print("I");
      if (ITemp2 > -9) Serial3.print(COLON_Str);
      Serial3.print(ITemp2);
      Serial3.print(" O:");
      Serial3.print(OTemp);
      Serial3.print(" T:");
      Serial3.print(TxTemp); 
      break;
      
    case 3:
      Serial3.print("Spd:");
      Serial3.print(int(windSpeed * 6076 / 5280));
      Serial3.print(oneSpace);
      Serial3.print(letterHeading[0]);
      Serial3.print(letterHeading[1]);
      Serial3.print(oneSpace);      
      Serial3.print(int(heading));
      Serial3.write(2);
      break;
      
    case 4:
      Serial3.print("GPS ");
      if (gpsHours<100)Serial3.print(oneSpace);
      if (gpsHours<10)Serial3.print(ZERO_Str);     
      Serial3.print(gpsHours);
      Serial3.print(COLON_Str);
      if (gpsMinutes<10)Serial3.print(ZERO_Str);     
      Serial3.print(gpsMinutes);
      Serial3.print(COLON_Str);
      if (gpsSeconds<10)Serial3.print(ZERO_Str);     
      Serial3.print(gpsSeconds);
      break;
      
    case 5:
      Serial3.print("MISSION");
      if (misHours<100)Serial3.print(oneSpace);
      if (misHours<10)Serial3.print(ZERO_Str);
      Serial3.print(misHours);
      Serial3.print(COLON_Str);
      if (misMinutes<10)Serial3.print(ZERO_Str);
      Serial3.print(misMinutes);
      Serial3.print(COLON_Str);
      if (misSeconds<10)Serial3.print(ZERO_Str);
      Serial3.print(misSeconds);
      break;  
      
    case 6:
      vidLevel = analogRead(vidLvlPin);
      if (vidLevel < 1000) Serial3.print(oneSpace);
      if (vidLevel < 100) Serial3.print(oneSpace);
      if (vidLevel < 10) Serial3.print(oneSpace);
      Serial3.print (vidLevel);
      Serial3.print(" ");
      voiceLevel = analogRead(voiceLvlPin);
      if (voiceLevel < 1000) Serial3.print(oneSpace);
      if (voiceLevel < 100) Serial3.print(oneSpace);
      if (voiceLevel < 10) Serial3.print(oneSpace);
      Serial3.print (voiceLevel);
      Serial3.print(oneSpace);
      auxLevel = analogRead(auxLvlPin);
      if (auxLevel < 1000) Serial3.print(oneSpace);
      if (auxLevel < 100) Serial3.print(oneSpace);
      if (auxLevel < 10) Serial3.print(oneSpace);
      Serial3.print (auxLevel);
      
  }
}
////////////////////////////////////////////////
// this subroutine takes a float and separates it into 2 integers
// so you can print the float with only one decimal place

void separate(float floatValue) {
  intHalf = int(floatValue);
  fracHalf = int ((floatValue - intHalf)*10);
}
///////////////////////////////////////////////
void speak(int voiceDelay, int speechNum) {
  voiceTime = millis() + voiceDelay;
  voiceWaiting = true;
  strcpy_P(speech,speeches[speechNum]);
}
////////////////////////////
void waitForSpeakTime() {
  if (loopTime0 >= voiceTime) {
    speaking = true;
    startSpeak = millis();
    voiceWaiting = false;
    emicSerial.print("S:-)2");
    emicSerial.println(speech);
//    Serial2.println(speech);
    emicSerial.flush();
  }
}
/////////////////////////////
void waitForSpeech() {
    if (emicSerial.read() == ':') {
      speaking = false;
      if (cmdWaiting == 98) {
        cmdWaiting = 99;
        if (!monkeyBusy)digitalWrite(lite00,LOW);
      }
      emicSerial.flush();
    }
  if ((speaking)&&(millis()- startSpeak > 20000))  {
    speaking = false;
    if (cmdWaiting == 98) {
      cmdWaiting = 99;
      cmdExecuting = 99;
    }
  }
}
/////////////////////////////////////////////////////
void checkForMoreSpeech()  {
  if ((loopTime0 - startSpeak) >= 6000) {
  if ((!voiceWaiting) && (!speaking) && (monkeyBusy)) {
//      if (readAltitudeFlag) Serial2.print("A");
//      if (readTempFlag) Serial2.print("T");
//      if (readMovesFlag) Serial2.print("M");
//      if (introFlag) Serial2.print("I");
//      Serial2.println(cmdExecuting);
  
      if ((readAltitudeFlag)&&(cmdExecuting == 4)) {
        emicSerial.print("S:-)2");
        emicSerial.print("The Altimeter reads ");
        emicSerial.print(spokenAltitude);
        emicSerial.print(" feet. The E bay battery Voltage is ");
        emicSerial.print(EbayBatt);
        emicSerial.print(" volts - and the Monkey battery voltage is ");
        emicSerial.print(MonkeyBatt);
        emicSerial.println(" volts.");
        speaking = true;
        startSpeak = millis();
        readAltitudeFlag = false;
//        Serial2.print("The Altimeter reads ");
//        Serial2.print(spokenAltitude);
//        Serial2.print(" feet. The E bay battery Voltage is ");
//        Serial2.print(EbayBatt);
//        Serial2.print(" volts - and the Monkey battery voltage is ");
//        Serial2.print(MonkeyBatt);
//        Serial2.println(" volts.");
        extraSpeechFlag = false;
      }
      else if ((readTempFlag)&&(cmdExecuting == 13)) {
        emicSerial.print("S:-)2");
        emicSerial.print("The temperature in my cabin is ");
        emicSerial.print(ITemp2);
        emicSerial.print(" Degrees Fairenheit. The Outside Temperature is ");
        emicSerial.print(OTemp);
        emicSerial.print(" Degrees. The Transmitter Temperature is ");
        emicSerial.print(TxTemp);
        emicSerial.println("  Degrees.");
        speaking = true;
        startSpeak = millis();
        readTempFlag = false;
//        Serial2.print("The temperature in my cabin is ");
//        Serial2.print(ITemp2);
//        Serial2.print(" Degrees Fahrenheit. The Outside Temperature is ");
//        Serial2.print(OTemp);
//        Serial2.print(" Degrees. The Transmitter Temperature is ");
//        Serial2.print(TxTemp);
//        Serial2.println("  Degrees."); 
        extraSpeechFlag = false;
      }
      else if ((readMovesFlag)&&(cmdExecuting == 17)) {
        if (loopTime0 > allMoveSpeakTime) {
          allMoveSpeakNum++;
          if (allMoveSpeakNum < 6) {
//            Serial2.println(allMoveSpeakNum);
            speechNumber = allMoveSpeakBase + allMoveSpeakNum;
            speak(0,speechNumber);
            startSpeak = millis();
            allMoveSpeakTime = loopTime0 + moveTime[allMoveSpeakNum]; 
          }
          else {
            allMoveSpeakNum = 0;
            readMovesFlag = false;
            extraSpeechFlag = false;
          }
        }
      }
//      else if ((introFlag)&&(cmdExecuting == 18)) {
//        if (loopTime0 > allMoveSpeakTime) {
//          allMoveSpeakNum++;
//          if (allMoveSpeakNum < 2) {
//            Serial2.println(allMoveSpeakNum);
//            speechNumber = allMoveSpeakBase + allMoveSpeakNum;
//            speak(0,speechNumber);
//            startSpeak = millis();
//            allMoveSpeakTime = loopTime0 + 7000; 
//          }
//          else {
//            allMoveSpeakNum = 0;
//            introFlag = false;
//            extraSpeechFlag = false;
//          }
//        }
//      }
    }
  }
}
////////////////////////////
void checkflash() {
  if (loopTime0 > flashTime) {
      if (!flash) {
        flash = true;
        flashTime = loopTime0 + 125;
        if (!voiceOK) {
          if (emicSerial.read() == ':') {
            voiceOK = true;
            Serial3.write(12);
            Serial3.write(17);
            Serial3.print("Voice OK");
            emicSerial.flush();                 // Flush the receive buffer
          }
        }
      }
      else {
        flash = false;
        flashTime = loopTime0 + 375;
        if (!voiceOK) {
          emicSerial.print('\n');
          Serial3.write(12);
          Serial3.write(17);
          Serial3.print("Voice Problem");

        }
      }
  }
}
///////////////////////////////////
//struct command {
//  char* cmdName;
//  int duration;
//  int cmdVoiceDelay;
//  int numSayings;
//  int moveNumber;
//};

void readCommand(int cmdNum) {
  presentCommand.duration = pgm_read_word_near(&(cmd[cmdNum].duration));
  presentCommand.cmdVoiceDelay = pgm_read_word_near(&(cmd[cmdNum].cmdVoiceDelay));
  presentCommand.numSayings = pgm_read_word_near(&(cmd[cmdNum].numSayings));
  presentCommand.moveNumber = pgm_read_word_near(&(cmd[cmdNum].moveNumber));
  strcpy_P(presentCommand.cmdName,cmd[cmdNum].cmdName);
}

//////////////////////////////////////
void printPresentCommand() {
  Serial.print(presentCommand.cmdName);
  Serial.print(", duration: ");
  Serial.print(presentCommand.duration);
  Serial.print(", voiceDelay: ");
  Serial.print(presentCommand.cmdVoiceDelay);
  Serial.print(", numSayings: ");
  Serial.print(presentCommand.numSayings);
  Serial.print(", moveNumber: ");
  Serial.println(presentCommand.moveNumber);
}
//////////////////////////////////////////////
void updateMissionTime() {
  if (misSeconds < 59) {
    misSeconds++;
  }  
  else {
    misSeconds = 0;
    if (misMinutes < 59) {
       misMinutes++;
    }
    else {
      misMinutes = 0;
      misHours++;
    }
  }
}
//////////////////////////////////////////////
void updateGpsTime() {
  if (gpsSeconds < 59) {
    gpsSeconds++;
  }  
  else {
    gpsSeconds = 0;
    if (gpsMinutes < 59) {
       gpsMinutes++;
    }
    else {
      gpsMinutes = 0;
      if (gpsHours < 23) {
        gpsHours++;
      }
      else {
        gpsHours = 0;
      }
    }
  }
}
//////////////////////////////////////////////////
//void reprintTelemTime() {
//  Serial.write(13);
//  Serial.print("Mission Time ");
//  if (misHours<100)Serial.print(" ");
//  if (misHours<10)Serial.print("0");
//  Serial.print(misHours);
//  Serial.print(":");
//  if (misMinutes<10)Serial.print("0");
//  Serial.print(misMinutes);
//  Serial.print(":");
//  if (misSeconds<10)Serial.print("0");
//  Serial.print(misSeconds);
//  Serial.print("    ");
//  Serial.print("GPS Time ");
//  if (gpsHours<100)Serial.print(" ");
//  if (gpsHours<10)Serial.print("0");
//  Serial.print(gpsHours);
//  Serial.print(":");
//  if (gpsMinutes<10)Serial.print("0");
//  Serial.print(gpsMinutes);
//  Serial.print(":");
//  if (gpsSeconds<10)Serial.print("0");
//  Serial.print(gpsSeconds);
//
//}
/////////////////////////////////////////////
void processTT4Input() {
  //First copy the String over to make room for any more incoming data
  string1Complete = false;
  inString = inStr1;
  inStr1 = "";
    Serial2.print(inString);     // Pass it along to the Telemetry Processor
  //Now find out if the message is from AB1JC. If not, dump it.
  if (inString.indexOf(callSign) == -1) {
    Serial.println("Not Me");
    inString = "";
  }
  else {       // input IS from AB1JC, process it
    sPtr1 = inString.indexOf(':');
    if (sPtr1 != -1) {     // message has colon: check type
      Serial.print("Has colon, then ");   
      subStr1 = inString.charAt(sPtr1+1);
      Serial.println(subStr1);
      if (subStr1 == "/") {processPositionReport();}
      else if (subStr1 == "T") { processTelemetryReport(); }
      else if (subStr1 == ">") { processStatusReport(); }
      else { }       // probably PARM,UNIT,EQNS or "CMD issued" message. Just pass it along for logging.
    }
    else {Serial.println("No colon");}
  }
}

/////////////////////////////////////////////
void processPositionReport() {
  Serial.print("Pos ");
  sPtr1 += 2;
  subStr1 = inString.substring(sPtr1,sPtr1+2);
  gpsHours = subStr1.toInt();
  sPtr1 += 2;  
  subStr1 = inString.substring(sPtr1,sPtr1+2);
  gpsMinutes = subStr1.toInt();
  sPtr1 += 2;
  subStr1 = inString.substring(sPtr1,sPtr1+2);
  gpsSeconds = subStr1.toInt()+1;
  gpsSync = true;
  sPtr1 += 3;
  subStr1 = inString.substring(sPtr1,sPtr1+2);
  LatDeg = subStr1.toInt();
  sPtr1 += 2;
  sPtr2 = inString.indexOf('N',sPtr1);
  subStr1 = inString.substring(sPtr1,sPtr2);
  LatMin = subStr1.toFloat();
  sPtr1 = sPtr2+2;
  subStr1 = inString.substring(sPtr1,sPtr1+3);
  LonDeg = subStr1.toInt();
  sPtr1 += 3;
  sPtr2 = inString.indexOf('W',sPtr1);
  subStr1 = inString.substring(sPtr1,sPtr2);
  LonMin = subStr1.toFloat();
  sPtr1 = sPtr2 + 2;
  subStr1 = inString.substring(sPtr1,sPtr1+3);
  heading = subStr1.toFloat();
  temp0 = (heading + 22.5)/45;
  temp1 = int(temp0);
  temp1 *= 2;
  letterHeading[0] = wordHeading[temp1];
  letterHeading[1] = wordHeading[temp1+1];
  sPtr1 = sPtr2 + 4;
  subStr1 = inString.substring(sPtr1,sPtr1+3);
  Serial.print(subStr1);
  Serial.println(" <==");
  windSpeed = subStr1.toFloat();
  sPtr1 = inString.indexOf('=');
  subStr1 = inString.substring(sPtr1+1,sPtr1+ 7);
  gpsAlt = subStr1.toFloat();
  spokenAltitude = long(gpsAlt);
  Serial.print(gpsHours);
  Serial.print(COLON_Str);
  Serial.print(gpsMinutes);
  Serial.print(COLON_Str);
  Serial.print(gpsSeconds);
  Serial.print(COMMA_Str);
  Serial.print(LatDeg);
  Serial.print(COMMA_Str);
  Serial.print(LatMin);
  Serial.print(COMMA_Str);
  Serial.print(LonDeg);
  Serial.print(COMMA_Str);
  Serial.print(LonMin);
  Serial.print(COMMA_Str);
  Serial.print(heading);
  Serial.print(COMMA_Str);
  Serial.print(letterHeading[0]);
  Serial.print(letterHeading[1]);
  Serial.print(COMMA_Str);
  Serial.print(windSpeed);
  Serial.print(COMMA_Str);
  Serial.println(spokenAltitude);  
}
/////////////////////////////////////////////
void processTelemetryReport() {
  Serial.print("Telem ");
  sPtr2 = sPtr1;
  sPtr1 = inString.indexOf(',',sPtr2+1);
  sPtr2 =  inString.indexOf(',',sPtr1+1);
  TxTemp = inString.substring(sPtr1+1,sPtr2).toInt();
  temp0 = (float(TxTemp)-100.0) * 1.8 + 32;
  TxTemp = int(temp0); 
  if (TxTemp < -99) TxTemp = -99;
  Serial.print(TxTemp);
  Serial.print("; "); 
  
  sPtr2 = sPtr1;
  sPtr1 = inString.indexOf(',',sPtr2+1);
  sPtr2 =  inString.indexOf(',',sPtr1+1);  
  ITemp1 = inString.substring(sPtr1+1,sPtr2).toInt();
  temp0 = (float(ITemp1)-100.0) * 1.8 + 32;
  ITemp1 = int(temp0);
  if (ITemp1 < -99) ITemp1 = -99;
  Serial.print(ITemp1);
  Serial.print("; "); 
  
  sPtr2 = sPtr1;
  sPtr1 = inString.indexOf(',',sPtr2+1);
  sPtr2 =  inString.indexOf(',',sPtr1+1);
  ITemp2 = inString.substring(sPtr1+1,sPtr2).toInt();
  temp0 = (float(ITemp2)-100.0) * 1.8 + 32;
  ITemp2 = int(temp0);
  if (ITemp2 < -99) ITemp2 = -99;
  Serial.print(ITemp2);
  Serial.print("; "); 
  
  sPtr2 = sPtr1;
  sPtr1 = inString.indexOf(',',sPtr2+1);
  sPtr2 =  inString.indexOf(',',sPtr1+1);
  OTemp = inString.substring(sPtr1+1,sPtr2).toInt();
  temp0 = (float(OTemp)-100.0) * 1.8 + 32;
  OTemp = int(temp0);
  if (OTemp < -99) OTemp = -99;
  Serial.print(OTemp);
  Serial.print("; "); 
  
  sPtr2 = sPtr1;
  sPtr1 = inString.indexOf(',',sPtr2+1);
  sPtr2 =  inString.indexOf(',',sPtr1+1);
  verticalSpeed = inString.substring(sPtr1+1,sPtr2).toFloat();
  verticalSpeed -= 200.0;
  Serial.print(verticalSpeed);
  Serial.print("; "); 

  for (i = 0;i<8; i++) {
    sPtr1 = sPtr2 + 1 + i;
    TelemBit[i] =  inString.charAt(sPtr1);
    Serial.print(TelemBit[i]); 
  }
  Serial.println("");
}

/////////////////////////////////////////////
void processStatusReport() {
  Serial.print("Stat ");
  sPtr1 = inString.indexOf("Bat1:");
  if (sPtr1 >= 0) {         // if sPtr1 == -1, it is not the right status message - dump it.
    sPtr1 += 5;
    sPtr2 = inString.indexOf(" ",sPtr1+1);
    tempStr = inString.substring(sPtr1,sPtr2);
    EbayBatt = tempStr.toFloat();
    Serial.print(EbayBatt);
    Serial.print("/ ");
    sPtr1 = inString.indexOf("Bat2:");
    sPtr1 += 5;
    sPtr2 = inString.indexOf(" ",sPtr1+1);
    tempStr = inString.substring(sPtr1,sPtr2);
    MonkeyBatt = tempStr.toFloat();
    MonkeyBatt /= 10;
    Serial.print(MonkeyBatt);
    Serial.print("/ ");
    sPtr1 = inString.indexOf("Pres:");
    sPtr1 += 5;
    sPtr2 = inString.indexOf(" ",sPtr1+1);
    tempStr = inString.substring(sPtr1,sPtr2);
    Pressure = tempStr.toFloat();
    Serial.print(Pressure);
    Serial.println("");
  }
  else {
    inString = "";
  }
}

/////////////////////////////////////////////
void processTPInput() {
  string2Complete = false;
    //First copy the String over to make room for any more incoming data
  cmdString = inStr2;
  inStr2 = "";
  Serial1.print(cmdString);     // Pass it along to the Balloon
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  loopTime0 = millis();
  if (string1Complete) processTT4Input();  // Input from Balloon - process, and copy to Telemetry Processor on Serial2
  if (string2Complete) processTPInput();   // input from Telemetry Processor (copy to TT4 on Serial1)
  if (loopTime0 >= timeTick) {    // Here is the stuff to do every second
    timeTick = loopTime0 + 1000;
    updateMissionTime();    
    if (gpsSync)updateGpsTime();
    reprintDisplay(); 
    if (musicOn) {
      if ((!monkeyBusy) && (cmdWaiting == 99)) {
        voiceLevel = analogRead(voiceLvlPin);
        dbLevel = float(voiceLevel)/1024 *30 -10;   //command is -10 db to 18 db 
        voiceLevel = int (dbLevel);
        if (voiceLevel > 18) voiceLevel = 18;
        if (voiceLevel != lastVoiceLevel) {
          emicSerial.print("V");
          emicSerial.println(voiceLevel);
          emicSerial.flush();
          Serial3.write(148);
          Serial3.print("                ");
          Serial3.write(148);
          Serial3.print("Audio ");
          Serial3.print(voiceLevel);
          Serial3.print(" dB");
          lastVoiceLevel = voiceLevel;
        }
      }
    }
  }

  checkflash();

  if (abortFlag) {
    if (flash) {
      digitalWrite(lite00,HIGH);
    }
    else {
      digitalWrite(lite00,LOW);      
    }
  }

    if (voiceWaiting) waitForSpeakTime();
    if (speaking) waitForSpeech();
    if (extraSpeechFlag) checkForMoreSpeech();

//  // Now housekeep and scan the keyboard every 2 milliseconds
  if (loopTime0 >= lastMillis + 2){
    lastMillis = loopTime0;  

    if (monkeyBusy) {
      if ((loopTime0 > waitTime)&&(!speaking)) {
        monkeyBusy = false;
        digitalWrite(lite00,LOW);
      }
    }
  
    if (!readtime) {
      row_num++;
      row_num &= 3;
//      Serial.print(row_num);
      if (row_num == 0) {
        pinMode (row0, OUTPUT);
        pinMode (row1, INPUT);
        pinMode (row2, INPUT);
        pinMode (row3, INPUT);
        digitalWrite(row0,LOW);
      }
      if (row_num == 1) {
        pinMode (row0, INPUT);
        pinMode (row1, OUTPUT);
        pinMode (row2, INPUT);
        pinMode (row3, INPUT);
        digitalWrite(row1,LOW);
      }
      if (row_num == 2) {
        pinMode (row0, INPUT);
        pinMode (row1, INPUT);
        pinMode (row2, OUTPUT);
        pinMode (row3, INPUT);
        digitalWrite(row2,LOW);
      }
      if (row_num == 3) {
        pinMode (row0, INPUT);
        pinMode (row1, INPUT);
        pinMode (row2, INPUT);
        pinMode (row3, OUTPUT);
        digitalWrite(row3,LOW);
      }
 
      readtime = true;
    }
    else {    // it IS readtime - read the keyboard
      processRow(row_num);
      readtime = false;
    }

  /////////////////////// now (if not busy) check if command is waiting...
    if (!monkeyBusy) {
      if (cmdWaiting != 99) {
        if (cmdWaiting == 98) {  //We are not moving, but we are still speaking
          waitForSpeech();
        }
        else if (cmdWaiting >= 50) {        //valid commands are less than 50
          Serial.print("Bad Command: ");
          Serial.println(cmdWaiting);
        }
        else {                              // command has valid number
          cmdExecuting = cmdWaiting;
          cmdWaiting = 99;
          Serial3.write(148);
          Serial3.print("                ");
          Serial3.write(148);
          Serial3.print("Cmd ");
          Serial2.print("AB1JC-11>Cmd:: ");
          Serial3.print(cmdExecuting);
          Serial2.print(cmdExecuting);
          Serial3.print(": ");
          Serial2.print(": ");
          monkeyBusy = true;
          readCommand(cmdExecuting);
          waitTime = loopTime0 + presentCommand.duration;
          Serial3.print(presentCommand.cmdName);
          Serial2.print(presentCommand.cmdName);
          Serial2.print(" - ");
          if (cmdExecuting == 21) changeDisplay();
          
          if (cmdExecuting <= 20) {
            if (presentCommand.moveNumber <= 20) {
              Serial1.print("$MONKE ");
              if (presentCommand.moveNumber < 10) Serial1.print("0");
              Serial1.println(presentCommand.moveNumber);
              Serial2.print("$MONKE ");
              if (presentCommand.moveNumber < 10) Serial2.print("0");
              Serial2.println(presentCommand.moveNumber);
            }
          }
          if (cmdExecuting == 23) {
            vidLevel = analogRead(vidLvlPin);
            videoLevel = float(vidLevel);
            videoLevel = videoLevel / 1024 * 63;
            vidLevel = int(videoLevel) + 21;  // the video levels are 0-63, but the commands are 21-84
            if (vidLevel > 84) vidLevel = 84;
            Serial1.print("$MONKE ");
            Serial1.println(vidLevel);
            Serial2.print("$MONKE ");
            Serial2.print(vidLevel);
            Serial3.write(162);
            Serial3.print(vidLevel);
          }
          if (cmdExecuting == 49) {
            auxLevel = analogRead(auxLvlPin);
            auxLevel /= 10;
            if (auxLevel > 99) auxLevel = 99;
            if (((spokenAltitude / 1000) +5) > auxLevel) {
              Serial3.write(156);
              Serial3.print("can't do");
              Serial2.print("$MXALT was not");
            }
            else {
              Serial1.print("$MXALT ");
              Serial1.println(auxLevel);
              Serial2.print("$MXALT ");
              Serial2.println(auxLevel);
              Serial3.write(156);
              Serial3.print("MXALT");
              Serial3.print(auxLevel);
            }
          }
          Serial2.println(" sent.");
          digitalWrite(lite00,HIGH);
          digitalWrite(lite01,LOW);
          digitalWrite(lite02,LOW);
          shift1 = false;
          shift2 = false;
          cmdWaiting = 99;
          if ((voiceOK) && (speechOn) && (presentCommand.numSayings != 0)) {
            speechNumber = 0;
            for (i=1; i< cmdExecuting; i++) {
              cmdNumSpeeches = pgm_read_word_near(&(cmd[i].numSayings));
              speechNumber += cmdNumSpeeches;
            }
            cmdNumSpeeches = pgm_read_word_near(&(cmd[cmdExecuting].numSayings));
            
            if (cmdExecuting == 17) {
              voiceDelay = pgm_read_word_near(&(cmd[cmdExecuting].cmdVoiceDelay));
              speak(voiceDelay,speechNumber);
              allMoveSpeakBase = speechNumber;
              allMoveSpeakNum = 0;
            }
//            else if (cmdExecuting == 18) {
//              voiceDelay = pgm_read_word_near(&(cmd[cmdExecuting].cmdVoiceDelay));
//              speak(voiceDelay,speechNumber);
//              allMoveSpeakBase = speechNumber;
//              allMoveSpeakNum = 0;
//            }
            else {     // Command is regular
              if (presentCommand.numSayings > 1)  spoke[cmdExecuting]++;
              spoke[cmdExecuting] %= cmdNumSpeeches; 
//              speechNumber += random(cmdNumSpeeches);
              speechNumber += spoke[cmdExecuting];
              voiceDelay = pgm_read_word_near(&(cmd[cmdExecuting].cmdVoiceDelay));
              speak(voiceDelay,speechNumber);
            }
            emicSerial.flush();                 // Flush the receive buffer
            if (cmdExecuting == 4) readAltitudeFlag = true;
            if (cmdExecuting == 13) readTempFlag = true;
            if (cmdExecuting == 17) {
              readMovesFlag = true; 
              allMoveSpeakTime = millis() + 3300;           
            }
//            if (cmdExecuting == 18) {
//              introFlag = true; 
//              allMoveSpeakTime = millis() + 6000;           
//            }
//            if ((cmdExecuting==4)||(cmdExecuting==13)||(cmdExecuting==17)||(cmdExecuting==18)) extraSpeechFlag = true;
              if ((cmdExecuting==4)||(cmdExecuting==13)||(cmdExecuting==17)) extraSpeechFlag = true;

          }
        }
      }
    }
  }
}
//////////////////    End of Loop()
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
void processRow(int n) {
  for(k = 0; k < 4; k++) {
    buttonDown[k] = false; 
  }
  if (digitalRead(col0)==LOW) buttonDown[0] = true;
  if (digitalRead(col1)==LOW) buttonDown[1] = true;
  if (digitalRead(col2)==LOW) buttonDown[2] = true;
  if (digitalRead(col3)==LOW) buttonDown[3] = true;

  firstButton = (3-n)*4;
  for (j = 0; j < 3; j++) {
    k = firstButton + j;
    processButton(k,buttonDown[j]);
  }
  // Now process the on/off switch on the right end of each row
  // There will be an action when these are turned ON and another when they are turned off
  
  k = firstButton+3;
  lightPin = 36 + 2 * n;
  processSwitch(k,n,buttonDown[3],lightPin);

  videoOn = functionOn[3];
  speechOn = functionOn[2];
  musicOn = functionOn[1];
  auxFunction3_On = functionOn[0];

}

/////////////////////////////////
void processButton(int x, boolean buttonIsDown) {
            if (buttonIsDown) {
            if (btn[x].cnt < 10) {       // switch closed count up
              btn[x].cnt++;
//              Serial.println(btn[x].cnt);
            }
            else {                      // Count at 10, Execute command
              btn[x].switchOn = true;
              if (!btn[x].lastSwitchOn) {
                btn[x].lastSwitchOn = true;
//                Serial.print("btn[");
//                Serial.print(x);
//                Serial.println("] ON");
//                btn[x].lastChange = millis();

                if (x == 13) {          // shift key has no "command" - process is different
                  if (shift1) {
                    shift1 = false;
                    digitalWrite(lite01,LOW);
                  }
                  else {
                    shift1 = true;
                    digitalWrite(lite01,HIGH);
                  }
                }
                else if ((x == 14) && (auxFunction3_On)) {          // shift key has no "command" - process is different
                  if (shift2) {
                    shift2 = false;
                    digitalWrite(lite02,LOW);
                  }
                  else {
                    shift2 = true;
                    digitalWrite(lite02,HIGH);
                  }
                }
                else if ((x == 12) && (abortFlag)) {
                  cmdWaiting = 27;   // Set the REAL ABORT command
                  Serial1.println("$ABORT");
                  abortFlag = false;
                  digitalWrite(lite00,LOW);
                  abortIsDone = true;
                }
                else {             // not shift key or abortFlag - process 1 of 4 commands:
                                   // shift1 and shift2 decode into 4 possible commands
                  shiftNum = 0;
                  if (shift1) shiftNum += 1;
                  if (shift2) shiftNum += 2;
                  cmdWaiting = btn[x].CMD[shiftNum];
                  if (cmdWaiting == 24) Serial1.println("$IDENT");
                  shift1 = false;
                  digitalWrite(lite01,LOW);
                  shift2 = false;
                  digitalWrite(lite02,LOW);
                }
              }
            }
          }
          else {                          // switch open count down
            if(btn[x].cnt > 0) {
              btn[x].cnt--;
//              Serial.println(btn[x].cnt);
            }
            else {                       // count at zero , mark button as UNpressed
              btn[x].switchOn = false; 
              if (btn[x].lastSwitchOn) {
//                Serial.print("btn[");
//                Serial.print(x);
//                Serial.println("] OFF");
              }
              btn[x].lastSwitchOn = false;
            }
          }
}
///////////////////////////////
void processSwitch(int x,int onRow,boolean buttonIsDown,int litePin) {
            if (buttonIsDown) {
            if (btn[x].cnt < 10) {       // switch closed count up
              btn[x].cnt++;
            }
            else {                      // Count at 10, Flash light
              btn[x].switchOn = true;
              if (!btn[x].lastSwitchOn) {
                btn[x].lastSwitchOn = true;
                Serial.print("btn[");
                Serial.print(x);
                Serial.println("] ON");
              }
              else {       // switch remains on - flash it
                if(functionOn[onRow]) {
                  if (flash) digitalWrite(litePin,LOW);
                  if (!flash) digitalWrite(litePin,HIGH);
                }
                else {
                  if (flash) digitalWrite(litePin,HIGH);
                  if (!flash) digitalWrite(litePin,LOW);                  
                }
              }
            }
          }
          else {                          // switch open count down
            if(btn[x].cnt > 0) {
              btn[x].cnt--;
            }
            else {                       // count at zero , mark button as UNpressed
              btn[x].switchOn = false; 
              if (btn[x].lastSwitchOn) {
                btn[x].lastSwitchOn = false;
                Serial.print("btn[");
                Serial.print(x);
                Serial.println("] OFF");
                if (functionOn[onRow]) {       // function is ON, turn it OFF
                  functionOn[onRow] = false;
                  digitalWrite(litePin, LOW);
                  cmdWaiting = offAction[onRow];
                }
                else {       // video is OFF, turn it ON.
                  functionOn[onRow] = true;
                  digitalWrite(litePin, HIGH);
                  cmdWaiting = onAction[onRow];
                }
              }
            }
          }
/////////////////////////////////////////////////// Now process ABORT
        if (onRow == 0) {
         // Now scan the ABORT switch !
          if (digitalRead(abortPin)==LOW) {
            if (btn[16].cnt < 100) {       // switch closed count up
              btn[16].cnt++;
            }
            else {                      // Count at 100, Execute command
              btn[16].switchOn = true;
              if (!btn[16].lastSwitchOn) {
                btn[16].lastSwitchOn = true;
 //               btn[16].lastChange = millis();
                cmdWaiting = btn[16].CMD[0];
                abortFlag = true;
              }
            }
          }
          else {                          // switch open count down
            if( btn[16].cnt > 0) {
              if (btn[16].cnt < 10) {
                btn[16].cnt = 0;
              }
              else {
                btn[16].cnt -= 10;
              }
            }
            else {                       // count at zero , mark button as UNpressed
              btn[16].switchOn = false; 
              abortFlag = false;
              if (btn[16].lastSwitchOn) {
                btn[16].lastSwitchOn = false;
                if (!abortIsDone) {
                  cmdWaiting = 30;
                }
                else {
                  cmdWaiting = 31;
                }
              digitalWrite(lite00,HIGH);
              }
            }
          }
        }
      }
/////////////////////////////////////////////////////////////////////
               /*
               SerialEvent occurs whenever a new data comes in the\
               hardware serial RX.  This routine is run between each
               time loop() runs, so using delay inside loop can delay
               response.  Multiple bytes of data may be available.
               */
               // Data coming from the TT4 must be copied to the Telemetry Processor
               void serialEvent1() {
               while (Serial1.available()) {
               // get the new byte:
               char inChar1 = (char)Serial1.read();
               // add it to the inputString:
               inStr1 += inChar1;
                  if (inStr1.length() > 97) {
                    inStr1 = "";
                  }
               // if the incoming character is a newline, set a flag
               // so the main loop can do something about it:
               if (inChar1 == '\n') {
               string1Complete = true;
             }
             }
             }
/////////////////////////////////////////////////////////////////////
               /*
               SerialEvent occurs whenever a new data comes in the\
               hardware serial RX.  This routine is run between each
               time loop() runs, so using delay inside loop can delay
               response.  Multiple bytes of data may be available.
               */
               // Data coming from the Telemetry Processor must be copied to the TT4
               void serialEvent2() {
               while (Serial2.available()) {
               // get the new byte:
               char inChar2 = (char)Serial2.read();
               // add it to the inputString:
               inStr2 += inChar2;
                  if (inStr2.length() > 30) {
                    inStr2 = "";
                  }
               // if the incoming character is a newline, set a flag
               // so the main loop can do something about it:
               if (inChar2 == '\n') {
               string2Complete = true;
             }
             }
             }


/////////////////////////////////////////////////////////////////////
