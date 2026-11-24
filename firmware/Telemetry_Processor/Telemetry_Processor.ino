/////////// SOFTWARE VERSION NUMBER ///////////////////////////
// Ver 1.0MM  11/1/19 Flight Release.
//
// ///////////// 2024 UPDATES 
// Ver 1.1  12/10/24 Added Version Number 
// don't forget to adjust the #define VERSION below! 
#define VERSION "1.1" 
// Copyright © 2023 University of Bridgeport, NASA, CT Space Grant Consortium, USIP.
// All rights reserved.
///////////////////////////////////////////////////////////////

const String callSign = "AB1JC-11"; //HAM Radio Callsign ---- Input who is the main flyer
boolean DaylightSavingsTime = false;
float AzimuthCorrection = 11.0;

String inStr1 = "";
String inStr2 = "";
String inStr3 = "";
String inString;
String tempStr;
String cmdStr;
String radioStr;
const String zeroStr = "0";
const String oneStr = "1";
const String sevenSpace = "       ";
const String oneSpace = " ";
const String Colon = ":";
char inChar1;
char inChar2;
char inChar3;
boolean string1Complete = false;
boolean string2Complete = false;
boolean string3Complete = false;
char TelemBit[9] = {'0','0','0','0','0','0','0','0'};
int rewriteCount = 0;
int loggingCount = 0;

boolean rewriteFlag;
boolean logging = false;
boolean fileOpen = false;

unsigned long int loopTime1 = 0;
unsigned long int loopTime2 = 0;
unsigned long int nextSecond;
unsigned long int nextPosition;
unsigned long int nextTelemetry;
unsigned long int nextStatus;
unsigned long int nextAntennaUpdate = 0;

boolean positionFlag;
boolean telemetryFlag;
boolean statusFlag;

int pcnt = 0;
int tcnt = 0;
int scnt = 0;

int powerOnLitePin = 26;
int loggingLitePin = 28;

int rewriteButton = 22;
int loggingSwitch = 24;

//////////////////////////////////=============================================
int sPtr1;
int sPtr2;
int sPtr3;
String subStr1;
String subStr2;
String subStr3;
float Latitude;               // Balloon latitude as a float
float Longitude;              // Balloon longitude as a float
float gpsAlt;                 // Balloon Altitude from GPS
float MidLatitude;            // Mid-Latitude between Base and Balloon, to calc longitude spacing   
double BaseLatitude = 41.2151667;
double BaseLongitude = 73.2331667;
float BaseAltitude = 280;  // feet ASL
double Northing;           // miles NORTH of Base
double Easting;            // miles EAST of Base
int Rng;                   // miles slant range to balloon
long int spokenAltitude = 23650;
float heading = 310.5;
char wordHeading[19] = {"N NEE SES SWW NWN "};
char letterHeading[3] = "NW";
float windSpeed = 43.2658;
int verticalSpeed = 11;
float EbayBatt = 7.8;
float MonkeyBatt = 15.8;
int outTemp = -37;
int inTemp = 85;
int txTemp = 132;
const String ONE_Str = "1";
const String ZERO_Str = "0";
const String COMMA_Str = ",";
float LatDistPerDegree;    // one Latitude degree = 60 nautical miles - about 69 statute miles
float LonDistPerDegree;    // one Longitude degree is that times the cosine of Latitude                  
//////////////////////////////////================================================

int line1;
int line2; 
int i;
int j;
int k;
int ypos;
int xpos; 
long int Alt = 26200;
char LatDirec = 'N';
char LonDirec = 'W';
int LatDeg = 46;
int LonDeg = 76;
float LatMin = 56.345;
float LonMin = 54.234;
float Vspeed;
float Batt1 = 7.8;
float Batt2 = 16.4;
float Speed;
float Pressure = 101325;
int Spd = 38;
int Vspd = -124;
int Hdg = 310;
int OTemp = -26;
int ITemp1 = -5;
int ITemp2 = 85;
int TxTemp = 124;
boolean GPSok = false;
boolean TimeSync = false;
boolean COMMSok = false;
boolean AltCAL = false;
boolean MonkeyOK = false;
boolean PreFlight = false;
boolean Rising = false;
boolean Falling = false;

double Range = 49.625;
float Azimuth;
double Elevation;
int Az = 180;
int El = 0;
int LastAz = 180;
int LastEl = 0;
double AltMiles;
int gpsHours;
int gpsMinutes;
int gpsSeconds;
boolean gpsSync;

int lclHours = 0;
int lclMinutes = 0;
int lclSeconds = 0;

// String tempStr;
String timeStr;
String lclTimeStr;
float temp0;
int temp1;
int temp2;

///////////////////////////
void incrementLclTime() {
  if (lclSeconds < 59) {lclSeconds++;}
  else {
    lclSeconds = 0;
    if (lclMinutes < 59) {lclMinutes++;}
    else {
      lclMinutes = 0;
      if (lclHours < 23) {lclHours++;}
      else {
        lclHours = 0;
      }
    }
  }
}
///////////////////////////////
void setLclTime() {
  lclHours = gpsHours - 5;
  if (DaylightSavingsTime) lclHours++;
  if (lclHours < 0) lclHours += 24;
  lclMinutes = gpsMinutes;
  lclSeconds = gpsSeconds+1;
}
///////////////////////////
void makeLclTimeStr() {
  lclTimeStr = "";
  if (lclHours < 10) lclTimeStr += zeroStr;
  lclTimeStr += String(lclHours);
  lclTimeStr += Colon;
  if (lclMinutes < 10) lclTimeStr += zeroStr;
  lclTimeStr += String(lclMinutes);
  lclTimeStr += Colon;
  if (lclSeconds < 10) lclTimeStr += zeroStr;
  lclTimeStr += String(lclSeconds);  
}

///////////////////////////
void Home() {
  Serial2.write(27);
  Serial2.print("[H");
}
void Down(int x){
  Serial2.write(27);
  Serial2.print("[");
  Serial2.print(x);
  Serial2.print("B");
}
//////////////////////////
void Over(int x){
  Serial2.write(27);
  Serial2.print("[");
  Serial2.print(x);
  Serial2.print("C");
}
//////////////////////////
void CR() {
  Serial2.write(13);
}
//////////////////////////
void Bold() {
  Serial2.write(27);
  Serial2.print("[1m");  
}
//////////////////////////
void Normal() {
  Serial2.write(27);
  Serial2.print("[0m");  
}
/////////////////////////
void Clearout(int x) {
  for (i = 0;i < x; i++) {
    Serial2.print(oneSpace);
  }
  Serial2.write(27);
  Serial2.print("[");
  Serial2.print(x);
  Serial2.print("D");
}
/////////////////////////
void updateLat() {
    Home();
    Down(2);
    Bold();
    Over(7);
    Clearout(13);
    Serial2.print(LatDirec);
    if (LatDeg < 10) Serial2.print(zeroStr);
    Serial2.print(LatDeg);
    Serial2.print(oneSpace);
    Serial2.println(LatMin);
    Home();
    Down(3);
    Bold();
    Over(7);
    Clearout(13);
    Serial2.print(LatDirec);
    if (LatDeg < 10) Serial2.print(zeroStr);
    Serial2.print(LatDeg);
    Serial2.print(oneSpace);
    Serial2.println(LatMin);
  Normal(); 
  Home(); 
}
/////////////////////////
void updateLon() {
  Home();
    Bold();
    Down(2);
    Over(25);
    Clearout(13);
    Serial2.print(LonDirec);
    if (LonDeg < 10) Serial2.print(zeroStr);
    Serial2.print(LonDeg);
    Serial2.print(oneSpace);
    Serial2.println(LonMin);
    Home();
    Down(3);
    Bold();
    Over(25);
    Clearout(13);
    Serial2.print(LonDirec);
    if (LonDeg < 10) Serial2.print(zeroStr);
    Serial2.print(LonDeg);
    Serial2.print(oneSpace);
    Serial2.println(LonMin);

//}
  Normal(); 
  Home(); 
}
//////////////////////////////
void updateAlt() {
  Home();
  Bold();
  Down(4);
    Over(5);
    Clearout(7);
    Serial2.print(Alt);
    Home();
    Bold();
    Down(5);
    Over(5);
    Clearout(7);
    Serial2.print(Alt);
  Normal(); 
  Home(); 
}
///////////////////////////////
void updateVspd() {
  Home();
  Bold();
  Down(4);
    Over(18);
    Clearout(5);
    Serial2.print(Vspd);
  Home();
  Bold();
  Down(5);
    Over(18);
    Clearout(5);
    Serial2.print(Vspd);
  Normal(); 
  Home(); 
}  
///////////////////////////////
void updateHdg() {
  Home();
  Bold();
  Down(4);
    Over(28);
    Clearout(4);
    Serial2.print(Hdg);
  Home();
  Bold();
  Down(5);
    Over(28);
    Clearout(4);
    Serial2.print(Hdg);
  Home();
  Down(2);
  Over(37);
  Serial2.print (letterHeading[0]);
  Serial2.print (letterHeading[1]);
  Home();
  Down(3);
  Over(37);
  Serial2.print (letterHeading[0]);
  Serial2.print (letterHeading[1]);  
  Normal(); 
  Home(); 
}
///////////////////////////////
void updateSpd() {
  Home();
  Bold();
  Down(4);
    Over(36);
    Clearout(3);
    if (Spd<100) Serial2.print(oneSpace);
    Serial2.print(Spd);
  Home();
  Bold();
  Down(5);
    Over(36);
    Clearout(3);
    if (Spd<100) Serial2.print(oneSpace);
    Serial2.print(Spd);
  Normal(); 
  Home(); 
}
///////////////////////////////
void updateOTemp() {
  Home();
  Bold();
  Down(6);
    Over(7);
    Clearout(4);
    Serial2.print(OTemp);
  Home();
  Bold();
  Down(7);
    Over(7);
    Clearout(4);
    Serial2.print(OTemp);
  Normal(); 
  Home(); 
}
///////////////////////////////
void updateITemp1() {
  Home();
  Bold();
  Down(6);
    Over(16);
    Clearout(4);
    Serial2.print(ITemp1);
  Home();
  Bold();
  Down(7);
    Over(16);
    Clearout(4);
    Serial2.print(ITemp1);
  Normal(); 
  Home(); 
}
///////////////////////////////
void updateITemp2() {
  Home();
  Bold();
  Down(6);
    Over(24);
    Clearout(4);
    if ((ITemp2 >= 0) && (ITemp2 < 100)) Serial2.print(oneSpace);
    Serial2.print(ITemp2);
  Home();
  Bold();
  Down(7);
    Over(24);
    Clearout(4);
    if ((ITemp2 >= 0) && (ITemp2 < 100)) Serial2.print(oneSpace);
    Serial2.print(ITemp2);
    Serial2.write(10);
  Normal(); 
  Home(); 
}
///////////////////////////////
void updateTxTemp() {
  Home();
  Bold();
  Down(6);
    Over(36);
    Clearout(3);
    Serial2.print(TxTemp);
  Home();
  Bold();
  Down(7);
    Over(36);
    Clearout(3);
    Serial2.print(TxTemp);
  Normal(); 
  Home(); 
}
///////////////////////////////
void updateV1() {
  Home();
  Bold();
  Down(8);
    Over(4);
    Clearout(5);
    Serial2.print(Batt1);
  Home();
  Bold();
  Down(9);
    Over(4);
    Clearout(5);
    Serial2.print(Batt1);
  Normal(); 
  Home(); 
}
///////////////////////////////
void updateV2() {
  Home();
  Bold();
  Down(8);
    Over(13);
    Clearout(6);
    Serial2.print(Batt2);
  Home();
  Bold();
  Down(9);
    Over(13);
    Clearout(6);
    Serial2.print(Batt2);
  Normal(); 
  Home(); 
}
///////////////////////////////
void updatePressure() {
  Home();
  Bold();
  Down(8);
    Over(29);
    Clearout(8);
    Serial2.print(Pressure/100,2);
  Home();
  Bold();
  Down(9);
    Over(29);
    Clearout(8);
    Serial2.print(Pressure/100,2);
  Normal(); 
  Home(); 
}
///////////////////////////////
 void updateBits() {
  Home();
  Bold();
  Down(10);
  Clearout(72);
  Over(7);
  if (GPSok) { Serial2.print(oneStr); }
  else { Serial2.print(zeroStr); }
  Over(7);
  if (TimeSync) { Serial2.print(oneStr); }
  else { Serial2.print(zeroStr); }
  Over(7);
  if (COMMSok) { Serial2.print(oneStr); }
  else { Serial2.print(zeroStr); }
  Over(7);
  if (AltCAL) { Serial2.print(oneStr); }
  else { Serial2.print(zeroStr); }
  Over(7);
  if (MonkeyOK) { Serial2.print(oneStr); }
  else { Serial2.print(zeroStr); }
  Over(7);
  if (PreFlight) { Serial2.print(oneStr); }
  else { Serial2.print(zeroStr); }
  Over(7);
  if (Rising) { Serial2.print(oneStr); }
  else { Serial2.print(zeroStr); }
  Over(7);
  if (Falling) { Serial2.print(oneStr); }
  else { Serial2.print(zeroStr); }
  Normal(); 
  Home(); 
}
////////////////////////////
void checkGPSTime() {
  // Just a placeholder for now
}
///////////////////////////////
void updateTime() {
  Home();
  Over(2);
  Clearout(9);
  Bold();
  Serial2.print(lclTimeStr);
  Home();
  Down(1);
  Over(2);
  Clearout(9);
  Bold();
  Serial2.print(lclTimeStr);
  Normal();
  Home();
  updateCommand();
}
///////////////////////////////
void updateRange() {
  Home();
  Over(18);
  Clearout(7);
  Bold();
  Serial2.print(Range);
  Home();
  Down(1);
  Over(18);
  Clearout(7);
  Bold();
  Serial2.print(Range);
  Normal();
  Home();
}
///////////////////////////////
void updateAz() {
  Home();
  Over(29);
  Clearout(4);
  Bold();
  Serial2.print(Az);
  Home();
  Down(1);
  Over(29);
  Clearout(4);
  Bold();
  Serial2.print(Az);
  Normal();
  Home();
}
///////////////////////////////
void updateEl() {
  Home();
  Over(37);
  Clearout(2);
  Bold();
  Serial2.print(El);
  Home();
  Down(1);
  Over(37);
  Clearout(2);
  Bold();
  Serial2.print(El);
  Normal();
  Home();
}
///////////////////////////////
void updatePosition() {
  checkGPSTime();
  updateRange();
  updateAz();
  updateEl();
  updateLat();
  updateLon();
  updateAlt();
  updateHdg();
  updateSpd();
  updateCommand();
}
///////////////////////////////
void updateTelemetry() {
  updateOTemp();
  updateITemp1();
  updateITemp2();
  updateTxTemp();
  updateVspd();
  updateBits();
  updateCommand();
}
///////////////////////////////
void updateStatus() {
  updateV1();
  updateV2();
  updatePressure();
  updateCommand();
}
//////////////////////////////////
void rewriteScreen() {
  rewriteFlag = false;
  writeScreen();
  updatePosition();
  updateTelemetry();
  updateStatus();
}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
void setup() {
  // put your setup code here, to run once:
  pinMode(powerOnLitePin, OUTPUT);
  pinMode(loggingLitePin, OUTPUT);
  pinMode(rewriteButton, INPUT_PULLUP);
  pinMode(loggingSwitch, INPUT_PULLUP);

  digitalWrite( powerOnLitePin, HIGH);
  Serial.begin(19200);
  Serial1.begin(19200);
  Serial2.begin(19200);
  Serial3.begin(9600);
  inStr1.reserve(99);
  inStr2.reserve(23);
  inStr3.reserve(25);
  inString.reserve(99);
  subStr1.reserve(12);
  subStr2.reserve(12);
  subStr3.reserve(12);
//  tempStr.reserve(99);
  timeStr.reserve(11);
  lclTimeStr.reserve(11);

  Serial.println("");
  Serial.println("Starting...");
  writeScreen();
  loopTime1 = millis();
  delay(2500);
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/////////////////////////////////
void makeBigLine(int Line_num) {
  Home();
  if ((Line_num > 0) && (Line_num <= 23)) {
    if (Line_num > 1) {Down(Line_num - 1);}
    Serial2.write(27);
    Serial2.print("#6");
    Serial2.write(27);
    Serial2.print("#3");
    Serial2.write(27);
    Serial2.print("[B");
    Serial2.write(27);
    Serial2.print("#6");
    Serial2.write(27);
    Serial2.print("#4");
    Home();
  }
}
////////////////////////////////////////
void updateCommand() {
  Home();
  Down(12);
  Over(9);
  Clearout(20);
  Bold();
  Serial2.print(inStr2);
//  Normal();
//  Home();
}
///////////////////////////////////////////////////////////////////////
void writeScreen() {
  Serial2.write(27);   // Reset Terminal
  Serial2.print("c");
  Serial2.write(12);   // clear screan, Home up
  Normal();            // Select normal character attributes
  makeBigLine(1);      // make lines 1 & 2 into one line with big letters
  makeBigLine(3);      // make lines 3 & 4 into one line with big letters
  makeBigLine(5);      // make lines 5 & 6 into one line with big letters
  makeBigLine(7);      // make lines 7 & 8 into one line with big letters
  makeBigLine(9);      // make lines 9 & 10 into one line with big letters
  Home();
  Serial2.println(F("  00:00:00 Range: 123.45 Az: 123 El: 12"));
  Serial2.println(F("  00:00:00 Range: 123.45 Az: 123 El: 12"));
  Serial2.println(F("  Lat: N42 25.6255  Lon: W71 52.6586 NW"));
  Serial2.println(F("  Lat: N42 25.6255  Lon: W71 52.6586 NW"));
  Serial2.println(F("Alt: .      Vspd: -125 Hdg: 310 Spd: 80"));
  Serial2.println(F("Alt: .      Vspd: -125 Hdg: 310 Spd: 80"));
  Serial2.println(F("OTemp: -49 IT1: -09 IT2: 86 TxTemp: 129"));
  Serial2.println(F("OTemp: -49 IT1: -09 IT2: 86 TxTemp: 129"));
  Serial2.println(F("V1: 7.8  V2: 16.8  Pressure: 1013.25 mB"));
  Serial2.println(F("V1: 7.8  V2: 16.8  Pressure: 1013.25 mB"));
  
  Serial2.println(F("       1       1       1       1       1       0       0       0   "));
  Serial2.println(F("     GPSok TimeSync COMMSok AltCAL MonkeyOK PreFlight Rising Falling"));
  Serial2.println(F("Command> "));
  Serial2.write(27);
  Serial2.print("[15;24r");
  Serial2.write(27);
  Serial2.print("[H");
  Serial2.write(27);
  Serial2.print("[23B");
  Serial2.println(F("Scrolling Region"));
  Serial2.write(27);
  Serial2.print("[H");
  Serial2.write(27);
  Serial2.print("[12B");
  Serial2.print("Command> ");
  Serial2.write(27);
  Serial2.print("[H");
}
//////////////////////////////
void checkButtons() {
  if (digitalRead(rewriteButton) == LOW) {
    if (rewriteCount < 11) {
      rewriteCount++;
    }
    if (rewriteCount == 10) rewriteFlag = true;      
  }
  else {rewriteCount = 0;}

  if (digitalRead(loggingSwitch) == LOW) {
    if (loggingCount < 10) {loggingCount++;}
    else {
      logging = true;
      digitalWrite(loggingLitePin, HIGH);
    }
  }
  else {
    if (loggingCount > 0) {loggingCount--;}
    else {
      logging = false;
      digitalWrite(loggingLitePin, LOW);
    }
  }
}
//////////////////////////////
void processPosition() {
  inString = ""; 
  positionFlag = false;
//  Home();
//  Down(23);
//  Serial2.print(lclTimeStr);
//  Serial2.print(inString);
  updatePosition();
  Home();
  updateCommand(); 
}
//////////////////////////////
void processTelemetry() {
  inString = ""; 
  telemetryFlag = false;
//  Home();
//  Down(23);
//  Serial2.print(lclTimeStr);
//  Serial2.print(inString);
  updateTelemetry();
  Home();
  updateCommand(); 
}
//////////////////////////////
void processStatus() {
  inString = ""; 
  statusFlag = false;
//  Home();
//  Down(23);
//  Serial2.print(lclTimeStr);
//  Serial2.print(inString);
  updateStatus();
  Home();
  updateCommand(); 
}
//////////////////////////////
void processCommand() {
  string2Complete = false;
  cmdStr = inStr2;  
  inStr2 = "";  
  Home();
  Down(23);
  Serial2.print(lclTimeStr);
  Serial2.print(F(" Received Command: "));
  Serial2.print(cmdStr);
  Serial1.println(cmdStr);
  Serial.print(lclTimeStr);
  Serial.print(" ");
  Serial.print(cmdStr);
  Home();
  updateCommand();  
}
////////////////////////////////////////////
void Log_it() {
    Home();
    Down(23);
    Serial2.print(lclTimeStr);
    Serial2.print(" ");
    Serial2.print(inString);
    Serial.print(lclTimeStr);
    Serial.print(" ");
    Serial.print(inString);
    Home();
    updateCommand();
}
/////////////////////////////////////////////
void processTT4Input() {
  //First copy the String over to make room for any more incoming data
  temp1 = inStr1.length();
  if (temp1 < 15) {
    inStr1 = "";
    Serial.print("Short ");
    Serial.println(temp1);
    inString = "";
    string1Complete = false;
  }
  if (temp1 >= 15) {
    inString = inStr1;
    inStr1 = "";
    string1Complete = false;
    //Now find out if the message is from AB1JC. If not, dump it.
    if (inString.indexOf(callSign) == -1) {     // Message does not have AB1JC - check first character 
      subStr1 = inString.charAt(0);
      if (subStr1 == "T") { 
        Log_it();
        processTelemetryReport(); 
      }
      else if (subStr1 == ">") { 
        Log_it();
        processStatusReport(); 
      }
      else {       // probably PARM,UNIT,EQNS or "CMD issued" message. Just pass it along for logging.
        if (!logging) Serial.print("No match...");
        if (!logging) Serial.println(inString);
      }
    inString = "";
    }
  else {       // input IS from AB1JC, process it
    Log_it();
    sPtr1 = inString.indexOf(':');
    if (sPtr1 != -1) {     // message has colon: check type
      if (!logging) Serial.print("Has colon, then ");   
      subStr1 = inString.charAt(sPtr1+1);
      if (!logging) Serial.println(subStr1);
      if (subStr1 == "/") {processPositionReport();}
      else if (subStr1 == "T") { processTelemetryReport(); }
      else if (subStr1 == ">") { processStatusReport(); }
      else { }       // probably PARM,UNIT,EQNS or "CMD issued" message. Just pass it along for logging.
    }
    inString = "";
  }
}
string1Complete = false;
}
/////////////////////////////////////////////
void processStatusReport() {
  statusFlag = false;
  sPtr1 = inString.indexOf("Bat1:");
  if (sPtr1 >= 0) {         // if sPtr1 == -1, it is not the right status message - dump it.
    sPtr1 += 5;
    sPtr2 = inString.indexOf(" ",sPtr1+1);
    tempStr = inString.substring(sPtr1,sPtr2);
    Batt1 = tempStr.toFloat();
    if (!logging) Serial.print(Batt1);
    if (!logging) Serial.print("/ ");
    sPtr1 = inString.indexOf("Bat2:");
    sPtr1 += 5;
    sPtr2 = inString.indexOf(" ",sPtr1+1);
    tempStr = inString.substring(sPtr1,sPtr2);
    Batt2 = tempStr.toFloat();
//    Batt2 /= 10;
    if (!logging) Serial.print(Batt2);
    if (!logging) Serial.print("/ ");
    sPtr1 = inString.indexOf("Pres:");
    sPtr1 += 5;
    sPtr2 = inString.indexOf(" ",sPtr1+1);
    tempStr = inString.substring(sPtr1,sPtr2);
    Pressure = tempStr.toFloat();
    if (!logging) Serial.print(Pressure);
    if (!logging) Serial.print("/ ");
    statusFlag = true;
  }
  else {
    inString = "";
  }
}
/////////////////////////////////////////////
void processPositionReport() {
  sPtr1 += 2;
  subStr1 = inString.substring(sPtr1,sPtr1+2);
  gpsHours = subStr1.toInt();
  sPtr1 += 2;  
  subStr2 = inString.substring(sPtr1,sPtr1+2);
  gpsMinutes = subStr2.toInt();
  sPtr1 += 2;
  subStr3 = inString.substring(sPtr1,sPtr1+2);
  gpsSeconds = subStr3.toInt();
  gpsSync = true;
  setLclTime();
  sPtr1 += 3;
  subStr1 = inString.substring(sPtr1,sPtr1+2);
  LatDeg = subStr1.toInt();
  sPtr1 += 2;
  sPtr2 = inString.indexOf('N');
  subStr1 = inString.substring(sPtr1,sPtr2);
  LatMin = subStr1.toFloat();
  sPtr1 = sPtr2+2;
  subStr1 = inString.substring(sPtr1,sPtr1+3);
  LonDeg = subStr1.toInt();
  sPtr1 += 3;
  sPtr2 = sPtr1+5;
  subStr1 = inString.substring(sPtr1,sPtr2);
  if (!logging) Serial.print(subStr1);
  if (!logging) Serial.print(" ");
  LonMin = subStr1.toFloat();
  if (!logging) Serial.println(LonMin);
  sPtr1 = sPtr2 + 2;
  subStr1 = inString.substring(sPtr1,sPtr1+3);
  heading = subStr1.toFloat();
  temp0 = (heading + 22.5)/45;
  temp1 = int(temp0);
  temp1 *= 2;
  letterHeading[0] = wordHeading[temp1];
  letterHeading[1] = wordHeading[temp1+1];
  sPtr1 = sPtr1 + 4;
  subStr1 = inString.substring(sPtr1,sPtr1+3);
  windSpeed = subStr1.toFloat();
  sPtr1 = inString.indexOf('=');
  subStr1 = inString.substring(sPtr1+1,sPtr1+ 7);
  gpsAlt = subStr1.toFloat();
  spokenAltitude = long(gpsAlt);
  Alt = spokenAltitude;
  Hdg = int(heading);
  Spd = int(windSpeed * 6076 / 5280);   //speed in knots converted here to mph
  Latitude = float(LatDeg) + LatMin / 60;
  Longitude = float(LonDeg) + LonMin / 60;
  MidLatitude = (BaseLatitude + Latitude) / 2;
  LatDistPerDegree = 60.0 * 6076.0 / 5280.0;    // one Latitude degree = this many statute miles
  LonDistPerDegree = LatDistPerDegree * cos(MidLatitude * PI / 180);  // Longitude degrees are smaller by ~.7
  Northing = (Latitude - BaseLatitude) * LatDistPerDegree;
  Easting = (BaseLongitude - Longitude) * LonDistPerDegree;
  if ((abs(Northing) * 500) < abs(Easting)) {
    if (Easting > 0) {Azimuth = 90;}
    else {Azimuth = 270;}
  }
  else {
    Azimuth = (atan(Easting/Northing))*180 / PI;
    if (Northing < 0) {Azimuth += 180;}
    else if (Easting < 0) {Azimuth += 360;}
  }
  Azimuth += AzimuthCorrection;
  Az = int(Azimuth +.5);
  Range = sqrt(pow(Easting,2) + pow(Northing,2));
  AltMiles = double(Alt-BaseAltitude)/5280;
  Elevation = (atan(AltMiles / Range)) * 180 / PI;
  El = int(Elevation+.5);
  if (El < 0) El = 0;
  Range = sqrt(pow(AltMiles,2) + pow(Range,2));
  Rng = int(Range);
  // Now update the Antenna aimpoint if it has changed:
  if ((Az != LastAz) || (El != LastEl)) {  // Azimuth or Elevation to target has changed. Steer Antenna.
    Serial3.print("W");
    if (Az < 100) Serial3.print(zeroStr);
    if (Az < 10) Serial3.print(zeroStr);
    Serial3.print(Az);
    Serial3.print(oneSpace);
    Serial3.print(zeroStr);
    if (El < 10) Serial3.print(zeroStr);
    Serial3.println(El);
                                 // Now log the fact you commanded the antenna:
    Serial.print(lclTimeStr);
    Serial.print(" Sent command ");
    Serial.print("W");
    if (Az < 100) Serial.print(zeroStr);
    if (Az < 10) Serial.print(zeroStr);
    Serial.print(Az);
    Serial.print(oneSpace);
    Serial.print(zeroStr);
    if (El < 10) Serial.print(zeroStr);
    Serial.print(El);
    Serial.println(" to Antenna.");
    Home();
    Down(23);
    Serial2.print(lclTimeStr);
    Serial2.print(" Sent command ");
    Serial2.print("W");
    if (Az < 100) Serial2.print(zeroStr);
    if (Az < 10) Serial2.print(zeroStr);
    Serial2.print(Az);
    Serial2.print(oneSpace);
    Serial2.print(zeroStr);
    if (El < 10) Serial2.print(zeroStr);
    Serial2.print(El);
    Serial2.println(" to Antenna.");

    LastAz = Az;
    LastEl = El;
  }
  if (!logging) {
    Serial.print(gpsHours);
    Serial.print(COMMA_Str);
    Serial.print(gpsMinutes);
    Serial.print(COMMA_Str);
    Serial.print(gpsSeconds);
    Serial.print(COMMA_Str);
    Serial.print(LatDeg);
    Serial.print(COMMA_Str);
    Serial.print(LatMin);
    Serial.print(COMMA_Str);
    Serial.print(LonDeg);
    Serial.print(COMMA_Str);
    Serial.print(LonMin);
    Serial.println(COMMA_Str);
    Serial.print(Hdg);
    Serial.print(COMMA_Str);
    Serial.print(letterHeading[0]);
    Serial.print(letterHeading[1]);
    Serial.print(COMMA_Str);
    Serial.print(Spd);
    Serial.print(COMMA_Str);
    Serial.println(Alt);  
    Serial.print(COMMA_Str);
    Serial.println(Rng);  
    Serial.print(COMMA_Str);
    Serial.println(Az);  
    Serial.print(COMMA_Str);
    Serial.println(El);  
  }
  positionFlag = true;
}
/////////////////////////////////////////////
void processTelemetryReport() {
  if (!logging) Serial.println("Got here, Telemetry");
  sPtr2 = sPtr1;
  sPtr1 = inString.indexOf(',',sPtr2+1);
  sPtr2 =  inString.indexOf(',',sPtr1+1);
  TxTemp = inString.substring(sPtr1+1,sPtr2).toInt();
  temp0 = (float(TxTemp)-100.0) * 1.8 + 32;
  TxTemp = int(temp0); 
  if (TxTemp < -99) TxTemp = -99;
  if (!logging) Serial.print(TxTemp);
  if (!logging) Serial.print("; "); 
  
  sPtr2 = sPtr1;
  sPtr1 = inString.indexOf(',',sPtr2+1);
  sPtr2 =  inString.indexOf(',',sPtr1+1);  
  ITemp1 = inString.substring(sPtr1+1,sPtr2).toInt();
  temp0 = (float(ITemp1)-100.0) * 1.8 + 32;
  ITemp1 = int(temp0);
  if (ITemp1 < -99) ITemp1 = -99;
  if (!logging) Serial.print(ITemp1);
  if (!logging) Serial.print("; "); 
  
  sPtr2 = sPtr1;
  sPtr1 = inString.indexOf(',',sPtr2+1);
  sPtr2 =  inString.indexOf(',',sPtr1+1);
  ITemp2 = inString.substring(sPtr1+1,sPtr2).toInt();
  temp0 = (float(ITemp2)-100.0) * 1.8 + 32;
  ITemp2 = int(temp0);
  if (ITemp2 < -99) ITemp2 = -99;
  if (!logging) Serial.print(ITemp2);
  if (!logging) Serial.print("; "); 
  
  sPtr2 = sPtr1;
  sPtr1 = inString.indexOf(',',sPtr2+1);
  sPtr2 =  inString.indexOf(',',sPtr1+1);
  OTemp = inString.substring(sPtr1+1,sPtr2).toInt();
  temp0 = (float(OTemp)-100.0) * 1.8 + 32;
  OTemp = int(temp0);
  if (OTemp < -99) OTemp = -99;
  if (!logging) Serial.print(OTemp);
  if (!logging) Serial.print("; "); 
  
  sPtr2 = sPtr1;
  sPtr1 = inString.indexOf(',',sPtr2+1);
  sPtr2 =  inString.indexOf(',',sPtr1+1);
  Vspeed = inString.substring(sPtr1+1,sPtr2).toFloat();
  Vspeed -= 200.0;
  Vspd = int(Vspeed);
  if (!logging) Serial.print(Vspd);
  if (!logging) Serial.print("; "); 

  for (i = 0;i<8; i++) {
  sPtr1 = sPtr2 + 1 + i;
  TelemBit[i] =  inString.charAt(sPtr1);
  if (!logging) Serial.print(TelemBit[i]); 
  if (!logging) Serial.println();
  }
  if (TelemBit[0] == '1') {GPSok = true;}
  else {GPSok = false;}
  if (TelemBit[1] == '1') {TimeSync = true;}
  else {TimeSync = false;}
  if (TelemBit[2] == '1') {COMMSok = true;}
  else {COMMSok = false;}
  if (TelemBit[3] == '1') {AltCAL = true;}
  else {AltCAL = false;}
  if (TelemBit[4] == '1') {MonkeyOK = true;}
  else {MonkeyOK = false;}
  if (TelemBit[5] == '1') {PreFlight = true;}
  else {PreFlight = false;}
  if (TelemBit[6] == '1') {Rising = true;}
  else {Rising = false;}
  if (TelemBit[7] == '1') {Falling = true;}
  else {Falling = false;}
  telemetryFlag = true;  
}
///////////////////////////////////////////////
void updateAntenna() {
  Home();
  Down(12);
  Over(40);
  Clearout(38);
  Serial2.print("Antenna Position ");
  Serial2.print(inStr3);
  inStr3 = "";
  string3Complete = false;
  Home();
  updateCommand();
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void loop() {
  // put your main code here, to run repeatedly:
    loopTime1 = millis();

    // Every 2 msec check the buttons
    if (loopTime1 >= loopTime2 + 2) {
      loopTime2 = loopTime1;
      checkButtons();
    }
    
    if (positionFlag) processPosition();
    if (telemetryFlag) processTelemetry();
    if (statusFlag) processStatus();
    if (string2Complete) processCommand();
    if (string1Complete) processTT4Input();
    if (rewriteFlag) rewriteScreen();
    if (string3Complete)updateAntenna();

    if (loopTime1 >= nextSecond) {
      nextSecond = loopTime1 + 1000;
      incrementLclTime();
      makeLclTimeStr();
      updateTime();
      updateCommand();
    }
    if (loopTime1 >= nextAntennaUpdate) {
      Serial3.println("C2");
      nextAntennaUpdate += 3000;
    }
}
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
                            /*
               SerialEvent occurs whenever a new data comes in the
               hardware serial RX.  This routine is run between each
               time loop() runs, so using delay inside loop can delay
               response.  Multiple bytes of data may be available.
               */
             void serialEvent2() {
               while (Serial2.available()) {
                 // get the new byte:
                 char inChar2 = (char)Serial2.read();
                 // check the new character
                 if ((inChar2 == 8) || (inChar2 == 127)) {  // input character is a backspace. Subtract last char from inStr1.
                   k = inStr2.length();
                   if (k > 0) {
                     inStr2 = inStr2.substring(0,k-1);
                   }
                 }
                          // input char is NOT a backspace. Check for terminator?
                          // if the incoming character is a newline, set a flag so the main loop can do something about it
                 else if ((inChar2 == '\n') || (inChar2 == '\r')) {
                   if (inStr2.length() > 0) {
                     string2Complete = true;
                     inStr2 += "\r\n";
                   }
                 }
                 else {                    // string not complete, add character to input string
                   if ((inChar2 >= 32) && (inChar2 < 127)) {
                     inStr2 += inChar2;
                     if (inStr2.length() > 20)  {inStr2 = "";}
                   }
                 }
               }
               updateCommand(); 
             }
/////////////////////////////////
                            /*
               SerialEvent occurs whenever a new data comes in the
               hardware serial RX.  This routine is run between each
               time loop() runs, so using delay inside loop can delay
               response.  Multiple bytes of data may be available.
               */
             void serialEvent1() {
               while (Serial1.available()) {
                 // get the new byte:
                 char inChar1 = (char)Serial1.read();
                 // add it to the inputString:
                 inStr1 += inChar1;
                  if (inStr1.length() > 98) {
                    inStr1 = "";
                  }
                 // if the incoming character is a newline, set a flag
                 // so the main loop can do something about it:
                 if (inChar1 == '\n') {
                   string1Complete = true;
                   if (!logging) Serial.print(inStr1);
                 }
               }
             }

/////////////////////////////////
                            /*
               SerialEvent occurs whenever a new data comes in the
               hardware serial RX.  This routine is run between each
               time loop() runs, so using delay inside loop can delay
               response.  Multiple bytes of data may be available.
               */
             void serialEvent3() {
               while (Serial3.available()) {
                 // get the new byte:
                 char inChar3 = (char)Serial3.read();
                 // add it to the inputString:
                 inStr3 += inChar3;
                  if (inStr3.length() > 25) {
                    inStr3 = "";
                  }
                 // if the incoming character is a newline, set a flag
                 // so the main loop can do something about it:
                 if (inChar3 == '\n') {
                   string3Complete = true;
                   if (!logging) Serial.println(inStr3);
                 }
               }
             }

/////////////////////////////////
