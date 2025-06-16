/*
// PROGRAM FOR USIP_1  (DISCOVERY III)
// USIP Copyright 2025 NASA, University of Bridgeport, Discovery Museum Bridgeport, CT.
// All rights reserved.
//////////// SOFTWARE VERSION NUMBER //////////////////////////
*/
#define VERSION "1.4"
/*
// Ver 0.2, 8/11/19 Updated SEALEVELPRESSURE_HPA
//                  USIP Ver to LCD GTM
// Ver 0.3, 8/12/19 Moved Paralax LCD to include file
// Ver 0.3A, 8/12/19                
// Ver 0.4, 8/14/19 Added basic parse monkey command from PC & Radio
//                  Adjusting telemetry
// Ver 0.5, 8/21/19 Intergrated serial communication between USIP and Monkey Board
// Ver 0.6, 8/22/19 Implemented monkey reply back top USIP board
// Ver 0.7, 9/10/19 Works with Radio APRS
// Ver 0.8, 9/26/19 Fixes made to telemetry, reception of data from Monkey
// Ver 0.9, 10/18/19 Fixes made to telemetry, reception of data from Monkey
// Ver 1.0, 10/20/19 More fixes to telemetry
// Ver 1.1, 11/16/19 FIXED THE ABORT PROBLEM !!!! and other fixes, turned off DEBUG and dbg
// Ver 1.2, 2/8/25 Minor fixes. Init GPS_alt1, 2, and 3 when GPS time sync occurs. 
//     Added if (dbg) print statements to refamiliarize myself with the software.
//     #define DEBUG (line 34 below) is not commented out, and boolean dbg == true.
//     Changed Balloon name from "USIP Discovery3 Balloon" to "Univ.Bridgeport USIP Balloon" for radio transmissions
//     Added comments
// Ver 1.3, 3/22/25 Reconstruction for ESP32 Data Comm, commented out Serial 3, various data pins, USIP LCD commands HTH
// Ver 1.4, 3/23/25 Removed Serial 2 for data com, transmits over SerialBT now
*/
////////////////////////////////////////////////////////////////

#include <Wire.h>
#include <Adafruit_Sensor.h>  // adafruit unified sensor library
#include <Adafruit_BME280.h>  // adafruit BME280 library
#include "BluetoothSerial.h"  // Bluetooth Serial for ESP32 3/20/25 HTH

//#define SEALEVELPRESSURE_HPA (101325.0)     //delete?  We use SLP, below, which is 1013.25

#define DEBUG  // Turn on Debug printing.  This sends debug info to the IDE serial port
boolean dbg = true;

// PARALAX LCD COMMANDS
// #include "USIP_LCD.h"

BluetoothSerial SerialBT;  // 3/20/25 HTH

uint8_t address[6] = { 0x00, 0x14, 0x03, 0x05, 0x01, 0x62 };  // HC-05 MAC address
const char *pin = "1234";                                     // Standard HC-05 PIN
bool connected = false;

Adafruit_BME280 bme;
float SLP = 1013.25;
String ONE_Str = "1";
String ZERO_Str = "0";
String COMMA_Str = ",";
String inStr1 = "";  // a string to hold incoming data from GPS
String inStr0 = "";  // a string to hold incoming data the keyboard

String CMD_Str = "";  // the 2-letter command, then we build the command string
char A_units;

//String PARM_Str = ":AB1JC-11 :PARM.TxTemp,Itmp1,Itmp2,OTemp,Vspd,GPSok,Time,COM,Alt,Mky,PRE,UP,DN\n"; // edited 9/26/19 LJR
//String UNIT_Str = :AB1JC-11 :UNIT.deg.C,deg.C,deg.C,deg.C,ft/s,valid,sync,OK,CAL,OK\n";              // edited 9/26/19 LJR
//String EQU_Str = ":AB1JC-11 :EQNS.0,1,-100,0,1,-100,0,1,-100,0,1,-100,0,1,-200\n";                // edited 9/26/19 LJR
//String BIT_Str = ":AB1JC-11 :BITS.11111111, Univ. Bridgeport USIP Balloon\n";

// BITs for USIP ===>GPSok TimeSync COMMSok AltCAL MonkeyOK PreFlight Rising Falling
//real spelling ---> GPS_valid, time_sync, COMMSok, Alt_CAL, MonkeyOK,  prelaunch, rising falling

String GGAStr = "";  // The string from the GPS
String GGAprimeStr;
String TSTStr = "";
String TempStr = "";
boolean string1Complete = false;  // whether the inStr1 is complete
boolean string0Complete = false;  // whether the inStr0 is complete
boolean string2Complete = false;  // whether the inStr2 is complete

/////////////////////////////////////////
// stuff regarding Monkey data reply
int Data1;
int replyLength;
int sPtr1;
int sPtr2;
int sPtr3;
boolean MonkeyDataReceived;
unsigned long int TimeOfRequest;
////////////////////////////////////////
boolean CMD_good;
int indx_dollar;
int fm_indx;
int to_indx;
int x;
char cksm;
char sum2chk;
char tst_chars[125];
char one_char;
int ITempraw = 0;
int OTempraw = 0;
int Batvraw = 0;
float ITemp1 = 13.0;
float ITemp2 = 13.0;
float OTemp = -44.9;
float TxTemp = 65.4;
float Batv1 = 7.3;
float Batv2 = 14.5;
float psTemp = 18;
float psPressure = 101325;
float Pressure_alt = 0;
float pa1;
float pa2;
float pa3;
float pa4;
//int telem_pres_alt = 0;
int telem_ITemp1 = 0;
int telem_ITemp2 = 0;
int telem_TxTemp = 0;
int telem_OTemp = 0;
int telem_Batv1 = 0;
//int telem_PA = 0;
int Tseq = 0;
int telem_Vspeed = 17;
float GPS_in;
float GPS_alt1 = 0;
float GPS_alt2 = 0;
float GPS_alt3 = 0;
float Expect;
float Launch_alt = 0;
float AvgAlt = 0;
float Vspeed = 0;
float VspeedAvg = 0;
float PA_VspeedAvg = 0;

///////////////////////////Goin' Ape Stuff///////////////
int monkey_cmd = 0;
int monk_cmd = 0;
boolean monkey_busy = false;
String MonkeyStr = "";
boolean MONKEYCMD_flg = false;  // received a monkey command
String inStr2 = "";
String tryit = "Garbage...";
String MonkeyReplyStr = "";
boolean Monkey_reply_flg = false;  // received communication from Monkey
String Xavier2 = "";
String TempMStr = "";
boolean BME_OK = true;
//////////////////////////////
//pins
int ledpin = 13;
int ABORT_pin = 2;  // abort of the monkey and cuts balloon away
//int OT_pin = A0; // outside temp
//int IT_pin = A1; // inside temp
//int TX_pin = A2; //Transmitter temperature
//int V_pin = A3; // battery voltage/3.47 for voltage divider
int MKXMT_pin = 16;
int MKRCV_pin = 17;

////////////////////////////////////////
unsigned long lastloop = 0;
unsigned long looptime = 0;
unsigned long looptime1 = 0;
unsigned long looptime2 = 0;
unsigned long looptime3 = 0;
unsigned long looptime4 = 0;
unsigned long endAbort;
unsigned long LastMonkeyReply = 0;
int len;
int GPS_hr = 0;
int GPS_min = 0;
int GPS_sec = 0;
int HGPS_hr = 0;
int HGPS_min = 0;
int HGPS_sec = 0;
String GPS_time = "00:00:00";
int MIS_hr = 0;
int MIS_min = 0;
int MIS_sec = 0;
String MIS_time = "00:00:00";
boolean time_sync = false;
boolean last_time_sync = false;
unsigned long time_sync_at = 0;
unsigned long lastGPS = 0;
unsigned long next_sec = 1000;
char Lat_direc;
int Lat_deg;
float Lat_min;
char Lon_direc;
int Lon_deg;
float Lon_min;
int num_sat;
float GPS_alt = 0;
char valid_char;
String GPS_date = "000000";
boolean ABORT_flg = false;
int abortCount;
boolean IDENT_flg = false;
boolean MXALT_flg = false;
boolean GPS_valid = false;
boolean PhaseCalc_flg = false;
boolean Alt_Cal = false;
boolean COMMSok = true;
boolean MonkeyOK = false;
int BadCount = 0;

float max_alt = 99;  /////////////  IF max_alt = THE DEFAULT 99,000 - it will NEVER CAUSE AN ABORT
boolean PARM_flg = false;
boolean UNIT_flg = false;
boolean EQU_flg = false;
boolean BIT_flg = false;

///////////////////////////////////////////////////////////////////
// FLIGHT PHASES
boolean prelaunch = false;
boolean rising = false;
boolean aborting = false;
boolean falling = false;
boolean landed = false;

////////////////////////////// begin setup ////////////////////////////

void setup() {
  pinMode(ABORT_pin, OUTPUT);  // ABORT pin - LOW Active
  digitalWrite(ABORT_pin, HIGH);
  pinMode(ledpin, OUTPUT);
  //pinMode(OT_pin, INPUT); //Outside Temp
  //pinMode (IT_pin, INPUT); // inside temp
  //pinMode(TX_pin, INPUT); // transmitter temp
  //pinMode (V_pin, INPUT);
  pinMode(MKXMT_pin, OUTPUT);
  pinMode(MKRCV_pin, INPUT);

  // initialize serial:
  // Blink LED that we are up and running
  for (x = 0; x < 10; x++) {
    digitalWrite(ledpin, HIGH);
    delay(100);
    digitalWrite(ledpin, LOW);
    delay(100);
  }
  // set up the rates of all the serial communications
  Serial.begin(19200);   // monitor in the IDE tool for commands and output
  Serial1.begin(19200);  // Tiny Trak 4 (radio)
  //Serial2.begin(9600);   // monkey cpu
  //Serial3.begin(9600);  //  debug display Paralax Serial LCD Display

  Serial.println("");
  Serial.print("USIP Version ");
  Serial.println(VERSION);

  // back light LCD & move cursor on LCD to line zero position 0 in debug mode
#ifdef DEBUG
/* Removed Serial 3 3/22/25 HTH
  Serial3.write(LCD_BKLITEON);
  LCD_Clear(); // clear the LCD display
  Serial3.print("USIP VERSION ");
  Serial3.print(VERSION);
  //show version for 2 seconds then clear
  delay(2000);
  LCD_Clear(); // clear the LCD display
  */
#endif

  //Begin Serial BT over 19200 3/20/25 HTH
  SerialBT.begin("ESP32test", true);  // ESP32 in master mode

  SerialBT.setPin(pin, strlen(pin));

  Serial.println("The device started in master mode, make sure remote BT device is on!");

  // Try connecting to the HC-05 using MAC address
  for (int attempt = 1; attempt <= 5; attempt++) {  // Try 5 times
    Serial.print("Attempt ");
    Serial.print(attempt);
    Serial.println(": Connecting to HC-05...");

    if (SerialBT.connect(address)) {
      Serial.println("Connected Successfully!");
      connected = true;
      break;
    } else {
      Serial.println("Failed to connect. Retrying...");
      delay(2000);  // Wait before retrying
    }
  }

  if (!connected) {
    Serial.println("Could not connect to HC-05. Check if it is powered on and in pairing mode.");
  }
  // End Serial BT 3/20/25 HTH

  // reserve bytes for the input strings inStr0 (computer keyboard) and inStr1 (tiny trak 4):
  GPS_date.reserve(8);
  inStr1.reserve(100);
  inStr2.reserve(85);
  inStr0.reserve(15);
  GGAStr.reserve(100);
  // HASPStr.reserve(100);
  TSTStr.reserve(25);
  TempStr.reserve(15);
  CMD_Str.reserve(15);

  unsigned status;
  status = bme.begin();
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x");
    Serial.println(bme.sensorID(), 16);
    BME_OK = false;
  } else {
    BME_OK = true;
  }
}
//////////////////////////////////// end Setup ///////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////

void time_update(int update_source) {  // 0 = internal update, 1 = OWN_GPS, 2 = HASP_GPS
  if (time_sync && !last_time_sync) {  // If we are JUST NOW receiving our first valid GPS message.
    last_time_sync = true;             // we will do this section only once
    time_sync_at = millis();           // Note the time we do it.
    if (update_source == 1) {
      MIS_sec = GPS_sec;          // set the Mission seconds EQUAL to the OWN_GPS seconds
      next_sec = lastGPS + 1000;  // set the "next_second" variable based on the receipt of the OWN_GPS $GPGGA message.
      Serial.print("Time Sync to OWN_GPS at ");
    }
    //else if (update_source == 2) {
    //  Serial.print("Bad update source - HASP - this is HASP Sim - ");
    //}
    else {
      Serial.print("Time Sync Error - ");  // Error - shouldn't get here. Clear time sync and last_time_sync & decr MIS_min
      MIS_min -= 1;
      time_sync = false;
      last_time_sync = false;
      next_sec += 1000;
    }
    Serial.print(time_sync_at / 1000);
    Serial.println(" seconds.");
    MIS_min += 1;         // and arbitrarily advance the Mission clock to the next minute.
    if (MIS_min >= 60) {  // then update Mission hours if necessary
      MIS_min = 0;
      MIS_hr += 1;
    }
    GPS_alt3 = GPS_alt;  // Let's initialize these for the EXPECT ALTITUDE computation
    GPS_alt2 = GPS_alt;
    GPS_alt1 = GPS_alt;
  } else if (!time_sync) {  // We have not received any valid GPS messages yet.
    next_sec += 1000;       // update "next_second" variable based on internal clock
    MIS_sec += 1;           // ...and update Mission time manually
    if (MIS_sec >= 60) {
      MIS_sec = 0;
      MIS_min += 1;
      if (MIS_min >= 60) {
        MIS_min = 0;
        MIS_hr += 1;  // GPS time rolls over at 23:59:59.  Mission time does not - hours keep counting.
      }
    }
  } else {                                                // we are in time sync.  Did the GPS not come in this second?
    if (update_source == 0) {                             // No. update GPS time manually...
      if (millis() - lastGPS > 150000) GPS_valid = false; /* 2-1/2 minutes*/
      GPS_sec += 1;
      if (GPS_sec == 60) {
        GPS_sec = 0;
        GPS_min += 1;
        if (GPS_min == 60) {
          GPS_min = 0;
          GPS_hr += 1;
          if (GPS_hr == 24) {
            GPS_hr = 0;
          }
        }
      }
      MIS_sec += 1;  //  ... and also update Mission time...
      if (MIS_sec == 60) {
        MIS_sec = 0;
        MIS_min += 1;
        if (MIS_min == 60) {
          MIS_min = 0;
          MIS_hr += 1;
        }
      }
      next_sec += 1000;  // ...and set "next_second" internally
      if ((MIS_min % 20 == 5) && (MIS_sec == 0)) PARM_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 10)) UNIT_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 20)) EQU_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 30)) BIT_flg = true;
      if (MIS_sec % 10 == 2) {
        PhaseCalc_flg = true;
      }

    } else if (update_source == 1) {       // we are in time sync, and the OWN_GPS has just come in
      if (MIS_sec < 10 && GPS_sec > 50) {  // Internal time is ahead of the GPS and has gone to the next minute
        MIS_min -= 1;
        if (MIS_min == -1) {
          MIS_min = 59;
          MIS_hr -= 1;
          if (MIS_hr < 0) MIS_hr = 0;  // gtm
        }
      } else if (MIS_sec > 50 && GPS_sec < 10) {  // Internal time is behind GPS.  GPS has gone to the next minute.
        MIS_min += 1;
        if (MIS_min >= 60) {
          MIS_min = 0;
          MIS_hr += 1;
        }
      } else {
      }
      MIS_sec = GPS_sec;          // and then set Mission seconds = GPS seconds.
      next_sec = lastGPS + 1000;  // update "next_second" variable to the time the next GPS message is expected.
      if ((MIS_min % 20 == 5) && (MIS_sec == 0)) PARM_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 10)) UNIT_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 20)) EQU_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 30)) BIT_flg = true;
      if (MIS_sec % 10 == 2) {
        PhaseCalc_flg = true;
        if (dbg) Serial.print("Now --");
      }
    }
    /*
    else if (update_source == 2) {   // we are in time sync, and the HASP_GPS has just come in
      if (millis()-lastGPS>180000) {   // We'll use our own GPS for updates if it has been less than 3 minutes since the last OWN_GPS update 
        if (MIS_sec < 10 && HGPS_sec >50) {    // Internal time is ahead of the HASP GPS and has gone to the next minute
          MIS_min -= 1;
          if (MIS_min == -1) {
            MIS_min = 59;
            MIS_hr -= 1;
            if (MIS_hr > 0) MIS_hr = 0;
          }
        } 
        else if (MIS_sec >50 && HGPS_sec < 10) {  // Internal time is behind GPS.  GPS has gone to the next minute.
          MIS_min += 1;
          if (MIS_min >= 60) {
            MIS_min = 0;
            MIS_hr += 1;
          }
        }
        MIS_sec = HGPS_sec;  // and then set Mission seconds = GPS seconds.
        GPS_sec = HGPS_sec;
        GPS_min = HGPS_min;
        GPS_hr = HGPS_hr;      
        next_sec = HlastGPS+1000;  // update "next_second" variable based on receipt of HASP_GPS message.
        Serial.println("HASP update");
      }
    }    
    */
    else {
    }
  }
  if (dbg) {
    /* Removed Serial 3 3/22/25 HTH
    Serial3.write(17);                     // LJR March 2025
    Serial3.write(128);
    Serial3.print("GPS: ");
    if (GPS_hr < 10) Serial3.print(ZERO_Str);
    Serial3.print(GPS_hr);
    Serial3.print(":");
    if (GPS_min < 10) Serial3.print(ZERO_Str);
    Serial3.print(GPS_min);
    Serial3.print(":");
    if (GPS_sec < 10) Serial3.print(ZERO_Str);
    Serial3.print(GPS_sec);
    */
    if (millis() - lastGPS >= 70000) { /* 1 minutes 10 sec */
      //Serial3.print(" I ");
    }
    /* Removed Serial 3 3/22/25 HTH
    else
    {if (time_sync == false)
    {Serial3.print(" I ");}
    else
    {Serial3.print(" G ");}
    }

    Serial3.write(148);
    Serial3.print("MIS: ");
    if (MIS_hr < 10) Serial3.print(ZERO_Str);
    Serial3.print(MIS_hr);
    Serial3.print(":");
    if (MIS_min < 10) Serial3.print(ZERO_Str);
    Serial3.print(MIS_min);
    Serial3.print(":");
    if (MIS_sec < 10) Serial3.print(ZERO_Str);
    Serial3.print(MIS_sec);
    */
    if (time_sync) {
      //  Serial3.print(" S ");
    } else {
      //Serial3.print("   ");
    }
    //Serial3.write(163);
  }

}  ////////////  End of time_update

/////////////////////////////Process Commands From Computer Pre Launch  ////////////////////////////////////////////
void Process_CMD() {
  CMD_Str = inStr0;
  inStr0 = "";
  string0Complete = false;
  //
  if ((CMD_Str.length() > 6) && (CMD_Str.length() < 12)) {
    TSTStr = CMD_Str.substring(0, 6);
    TSTStr.toUpperCase();

    if (TSTStr == "$ABORT") {  //Decode $ABORT message here. If not goto next else_if below.
      ABORT_flg = true;
      aborting = true;
      endAbort = millis() + 4000;
      Serial.println("ABORTING - Local Command!!!");
      Serial1.println("ABORTING - Local Command!!!");
    } else if (TSTStr == "$MXALT") {  // Decode $MXALT message here. Set Maximum Altitude and prep response.   If not goto next else_if below.
      MXALT_flg = true;
      max_alt = CMD_Str.substring(7, 9).toFloat();
      Serial.print("Max Altitude is now ");
      Serial.print(max_alt, 0);
      Serial.println(",000 feet.");
    } else if (TSTStr == "$IDENT") {  // Decode $IDENT message here. Process an output message. If not goto next else_if below.
      IDENT_flg = true;
      Serial.println("Univ.Bridgeport USIP High Altitude Balloon");
    }

    /// Monkey Commands from Computer
    else if (TSTStr == "MONKEY") {  // Decode Monkey Command 01.   If not goto next else_if below.
      MONKEYCMD_flg = true;
      monkey_cmd = CMD_Str.substring(7, 9).toInt();
      if (dbg) Serial.print("Entered Monkey Command: ");
      if (dbg) Serial.println(monkey_cmd);
      MonkeyStr = TSTStr + " " + CMD_Str.substring(7, 9);  // ex. MONKEY 07    // moved here from below if  LJR March 2025

      if (monkey_cmd >= 0 && monkey_cmd <= 84) {
        //          MonkeyStr = TSTStr + " " + CMD_Str.substring(7,9); // ex. MONKEY 07   // moved above if statement LJR March 2025
        //   MonkeyStr = CMD_Str.substring (0, 9);
        //  MonkeyStr = MonkeyStr + "\n";

        //Serial2.println(MonkeyStr);  // removed \n LJR March 2025
        SerialBT.println(MonkeyStr);
        if (dbg) Serial.println("---");
        if (dbg) Serial.print(MonkeyStr + "\n");  // ex. MONKEY 07
        if (dbg) Serial.println("---");
        if (dbg) Serial.println("Received A Monkey Command");
        if (dbg) Serial.println("Monkey is now busy.");
        TSTStr = "";            // clear out test string because we sent command to the monkey
        MONKEYCMD_flg = false;  //  added LJR March 2025
      } else {
        Serial.println("Bad monkey_cmd:");
        Serial.println(MonkeyStr);
        MONKEYCMD_flg = false;
      }
      monkey_busy = true;

      //  MonkeyStr = TSTStr;              LJR March 2025
    } else {  // Come here if not any valid message or command.
      Serial.print(TSTStr);
      Serial.println(": Unknown cmd ok size ??");  // and do nothing
    }
  } else {
    Serial.print(CMD_Str);
    Serial.print(": Unknown cmd bad size  ?? (");
    Serial.print(CMD_Str.length());
    Serial.println(")");
  }
}
//////////////////////////////////////////////////////////////////////////////
void ProcessOwnGPS() {
  //    looptime = micros();
  if (dbg) Serial.print("Got 1 - ");
  if (inStr1.length() > 5) {
    indx_dollar = inStr1.indexOf('$');
    if (indx_dollar >= 0) {
      GGAStr = inStr1.substring(indx_dollar);
      inStr1 = "";
      string1Complete = false;
      if (dbg) Serial.println(GGAStr);
      //      looptime1 = micros();
      TSTStr = GGAStr.substring(1, 6);
      if (dbg) Serial.print(TSTStr);
      if (dbg) Serial.print(" ");
      if (TSTStr == "GPGGA") {  // Decode $GPGGA message here. If not, jump to else below.
                                //        if (dbg) Serial.println("");
                                //        if (dbg) Serial.print("GPGGA decode, ");
        // first check validity and checksum
        //        looptime1 = micros();
        fm_indx = 6;
        for (x = 1; x < 6; x++) {
          to_indx = GGAStr.indexOf(',', fm_indx + 1);
          fm_indx = to_indx;
        }
        TSTStr = GGAStr.substring(fm_indx + 1, fm_indx + 2);
        if (TSTStr != "0") {  // 0 is invalid fix. 1 is GPS fix, 2 is DGPS fix.
          // copy string to character array
          len = GGAStr.length();
          GGAStr.toCharArray(tst_chars, len + 1);
          //          looptime3 = micros();
          fm_indx = 1;
          to_indx = GGAStr.lastIndexOf('*');
          one_char = tst_chars[to_indx + 1] - '0';
          if (one_char > 9) one_char -= 7;
          cksm = one_char << 4;
          one_char = tst_chars[to_indx + 2] - '0';
          if (one_char > 9) one_char -= 7;
          cksm += one_char;
          sum2chk = 0;
          for (x = fm_indx; x < to_indx; x++) {
            one_char = tst_chars[x];
            sum2chk ^= one_char;
          }
          if (sum2chk == cksm) {
            if (dbg) Serial.println("cksum OK");
            GPS_hr = (GGAStr.substring(7, 9)).toInt();
            GPS_min = (GGAStr.substring(9, 11)).toInt();
            GPS_sec = (GGAStr.substring(11, 13)).toInt();
            //          next_sec = millis() + 1000;
            fm_indx = GGAStr.indexOf(",", 13) + 1;
            to_indx = GGAStr.indexOf(",", fm_indx);
            Lat_deg = (GGAStr.substring(fm_indx, fm_indx + 2)).toInt();
            Lat_min = (GGAStr.substring(fm_indx + 2, to_indx)).toFloat();
            Lat_direc = GGAStr.charAt(to_indx + 1);
            fm_indx = to_indx + 3;
            to_indx = GGAStr.indexOf(",", fm_indx);
            Lon_deg = (GGAStr.substring(fm_indx, fm_indx + 3)).toInt();
            Lon_min = (GGAStr.substring(fm_indx + 3, to_indx)).toFloat();
            Lon_direc = GGAStr.charAt(to_indx + 1);
            fm_indx = to_indx + 5;
            num_sat = (GGAStr.substring(fm_indx, fm_indx + 2)).toInt();
            to_indx = GGAStr.indexOf(",", fm_indx);
            fm_indx = GGAStr.indexOf(",", to_indx + 1) + 1;
            to_indx = GGAStr.indexOf(",", fm_indx + 1);
            GPS_in = (GGAStr.substring(fm_indx, to_indx)).toFloat();
            //          fm_indx = GGAStr.indexOf(",",to_indx +1) + 1;
            //          to_indx = GGAStr.indexOf(",",fm_indx +1);
            A_units = GGAStr.charAt(to_indx + 1);
            Expect = ((GPS_alt2 - GPS_alt3) + (GPS_alt1 - GPS_alt2) + (GPS_alt - GPS_alt1)) / 3 + GPS_alt;
            GPS_alt3 = GPS_alt2;
            GPS_alt2 = GPS_alt1;
            GPS_alt1 = GPS_alt;
            if (GPS_in != 0) {
              if (A_units == 'M') {
                GPS_alt = GPS_in * 3.281;
              } else if (A_units == 'F') {
                GPS_alt = GPS_in;
              } else {  // units not M or F => bad message pointer => ignore altitude input
              }
              if ((dbg) && ((GPS_alt > Expect + 50) || (GPS_alt < Expect - 50))) {
                Serial.print("Altitude Bump ");
                Serial.println(GPS_alt - Expect);
              }
            } else {  // GPS altitude is zero => ignore altitude input
            }
            if (dbg) Serial.print("A_units = ");
            if (dbg) Serial.print(A_units);
            if (dbg) Serial.print(" GPS_alt = ");
            if (dbg) Serial.print(GPS_alt);
            if (dbg) Serial.print(" GPS_in = ");
            if (dbg) Serial.println(GPS_in);
            time_sync = true;
            GPS_valid = true;
            lastGPS = millis();
            time_update(1);

          }       // END of processing for valid message with good checksum
          else {  // Come here if checksum doesn't match - ignore message
            if (dbg) Serial.println("Bad checksum");
            GPS_valid = false;
          }
          if ((max_alt != 99) && !aborting) {
            Serial.print(GPS_alt);
            Serial.print(" > ");
            Serial.println((max_alt * 1000), 0);
            if (GPS_alt > (max_alt * 1000)) {
              abortCount++;
              if (dbg) Serial.print("AbortCount now ");
              if (dbg) Serial.println(abortCount);
              if (abortCount >= 12) {
                ABORT_flg = true;
                aborting = true;  //aaa
                endAbort = millis() + 4000;
                Serial.println("ABORTING - above GPS Max Altitude !!!");
                Serial1.println("ABORTING - above GPS Max Altitude !!!");
                abortCount = 0;
              }
            }
            //            else if ((GPS_alt < (max_alt * 1000)) && rising) {
            else if (GPS_alt < (max_alt * 1000)) {
              abortCount = 0;
            }
          }
        }                            // END of processing for a Valid Fix.
      }                              // END of $GPGGA message processing
      else if (TSTStr == "GPRMC") {  // Come here if not $GPGGA message.  Decode $GPRMC message here. If not goto else below.
        if (dbg) Serial.print("RMC decode .... Valid: ");
        fm_indx = 0;
        to_indx = GGAStr.indexOf(",", fm_indx);
        fm_indx = to_indx + 1;
        to_indx = GGAStr.indexOf(",", fm_indx);
        fm_indx = to_indx + 1;
        valid_char = GGAStr.charAt(fm_indx);
        if (dbg) Serial.print(valid_char);
        if (valid_char == 'A') {  // An 'A' here says the message is Valid
          fm_indx = 0;
          for (x = 0; x < 9; x++) {
            to_indx = GGAStr.indexOf(",", fm_indx);
            fm_indx = to_indx + 1;
          }
          to_indx = GGAStr.indexOf(",", fm_indx);
          //          Serial.print("\t String_");
          //          Serial.print(GGAStr.substring(fm_indx,to_indx));
          GPS_date = GGAStr.substring(fm_indx, to_indx);  // for valid messages, get the DATE string and keep it.
          if (dbg) Serial.print("\t Date_");
          if (dbg) Serial.println(GPS_date);
        }                            // if the valid_char was a 'V' (for NOT VALID) drop to here and exit doing nothing.
      }                              // END of $GPRMC processing
      else if (TSTStr == "ABORT") {  // Come here if not $GPGGA or $GPRMC message.  Decode $ABORT message here. If not goto next else_if below.
        aborting = true;
        ABORT_flg = true;
        endAbort = millis() + 4000;
        Serial1.println("ABORTING - Remote Command !!!");
        Serial.println("ABORTING - Remote Command !!!");
      } else if (TSTStr == "MXALT") {  // Decode $MXALT message here. Set Maximum Altitude and prep response.   If not goto next else_if below.
        MXALT_flg = true;
        max_alt = GGAStr.substring(7, 9).toFloat();
      } else if (TSTStr == "IDENT") {  // Decode $IDENT message here. Process an output message. If not goto next else_if below.
        IDENT_flg = true;
        if (dbg) Serial.println("GOT ident COMMAND VIA RADIO");
      } else if (TSTStr == "MONKE") {  // Decode MONKEY message here. Process an output message. If not goto next else_if below.
        if (dbg) Serial.println("GOT MONKEY COMMAND VIA RADIO");

        if (dbg) Serial.println(GGAStr);
        monk_cmd = GGAStr.substring(7, 9).toInt();
        if ((monk_cmd >= 0) && (monk_cmd < 85)) {
          // Serial.println("GOT MONKEY COMMAND VIA RADIO");
          // Serial2.print("MONKEY ");
          // Serial2.println(monk_cmd);
          // Serial2print(TSTStr + GGAStr.substring(7,9))
          TempMStr = "MONKEY ";
          TempMStr.concat(GGAStr.substring(7, 9));
          Serial.println(TempMStr);
          // send command to monkey board
          Serial2.println(TempMStr);  //send command to Monkey
          SerialBT.println(TempMStr);
          TempMStr = "MONKEY ";
        }
      } else {  // Come here if not any valid message or command.
        Serial1.println("USIP: Not Valid Command");
      }     // and do nothing
    }       // END of process if  '$'  in message
    else {  //Come here if no '$' in the string - probably started in middle of message. Discard & reset.
      Serial1.println("USIP: Invalid Command No $");
      BadCount++;
      if (dbg) Serial.println("Bad:");
      if (dbg) Serial.println(inStr1);
      inStr1 = "";
      string1Complete = false;
    }
  } else {  // come here if we have a short string
    if (dbg) Serial.println(inStr1.length());
    inStr1 = "";
    string1Complete = false;
  }
}

///////////////////////////////////////////////////////////////////////////////

void Process_ABORT() {
  digitalWrite(ABORT_pin, LOW);
  if (millis() > endAbort) {
    digitalWrite(ABORT_pin, HIGH);
    ABORT_flg = false;
  }
}
/////////////////////////////////////////////////////////////////////////////////
void Process_MXALT() {
  MXALT_flg = false;
  Serial1.print("New Max Alt = ");
  Serial1.print(max_alt, 0);
  Serial1.println(",000 ft.");
}
///////////////////////////////////////////////////////////////////////////////
// Process the Monkey Commands
// void Process_MONKEYCMD(){
//        MONKEYCMD_flg = false;

//        if (dbg) Serial.println("Processing Monkey command from Process_MonkeyCMD()" );
// #ifdef DEBUG
//       LCD_Clear();
//       Serial3.print("Process\n");
//       Serial3.print(MonkeyStr);
//  #endif
// //      Serial2.print (MonkeyStr + "\n");              // commented out LJR March 2025

// //      Serial.print (MonkeyStr + " "+"DUPE?"+"\n");       // LJR March 2025
//       monkey_busy = true;

// }
//////////////////////////////////////////////////////////////////////////////

void Process_IDENT() {
  Serial1.print(">");
  Serial1.print("Bat1:");
  Serial1.print(Batv1);
  Serial1.print(" Bat2:");
  Serial1.print(Batv2);
  Serial1.print(" P_alt:");
  Serial1.print(Pressure_alt);
  Serial1.print(" Pres:");
  Serial1.print(psPressure, 0);
  Serial1.println(" Univ.Bridgeport USIP Balloon");
  // now send status to terminal
  Serial.print(">");
  Serial.print("Bat1:");
  Serial.print(Batv1);
  Serial.print(" Bat2:");
  Serial.print(Batv2);
  Serial.print(" P_ALT:");
  Serial.print(Pressure_alt, 0);
  Serial.print(" Pres:");
  Serial.print(psPressure, 0);
  Serial.println(" USIP Balloon");
  IDENT_flg = false;
  BadCount = 0;
}
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////PROCESS MONKEY REPLY //////////////////////////////
void Process_MONKEYREPLY() {
  MONKEYCMD_flg = false;
  Monkey_reply_flg = false;
  string2Complete = false;

//      if (dbg) Serial.print("Monkey REPLY: " );
//      if (dbg) Serial.println(Xavier2);
#ifdef DEBUG
  //LCD_Clear();  //ESP32 3/20/25 HTH
  //Serial3.print("Process Monkey  Reply"); //ESP32 3/20/25 HTH
#endif
  // At this point we have two strings: Xavier2, which is "MONKEY xx", and
  // MonkeyReplyStr, which is the whole string.  inStr2 has already been cleared.
  if (Xavier2 == "MONKEY 10") {
    MonkeyOK = true;
    LastMonkeyReply = millis();
    monkey_busy = false;
  }
  if (Xavier2 == "MONKEY 11") {
    MonkeyOK = true;
    monkey_busy = false;
    LastMonkeyReply = millis();
  }
  if (Xavier2 == "MONKEY 17") {
    MonkeyOK = false;
    Serial1.println("Monkey Processor cannot adjust video.");
    Serial.println("Monkey Processor cannot adjust video.");
    monkey_busy = false;
  }
  if (Xavier2 == "MONKEY 13") {
    LastMonkeyReply = millis();
    replyLength = MonkeyReplyStr.length();
    Xavier2 = MonkeyReplyStr.substring(9, replyLength + 1);
    sPtr1 = Xavier2.indexOf(" ");  // zzz
    if (dbg) Serial.println(Xavier2);
    if (dbg) Serial.print("sPtr1=");
    if (dbg) Serial.print(sPtr1);
    sPtr2 = Xavier2.indexOf(",", sPtr1 + 1);
    if (dbg) Serial.print(" sPtr2=");
    if (dbg) Serial.print(sPtr2);
    Data1 = Xavier2.substring(sPtr1 + 1, sPtr2).toInt();
    if (dbg) Serial.print(" Data1=");
    if (dbg) Serial.println(Data1);
    Batv2 = ((float)Data1) / 10.0;  //  Monkey 13 First Data = Monkey Battery voltage
    sPtr1 = sPtr2 + 1;
    sPtr2 = Xavier2.indexOf(",", sPtr1);
    Data1 = Xavier2.substring(sPtr1, sPtr2).toInt();
    if (dbg) Serial.print("sPtr1=");
    if (dbg) Serial.print(sPtr1);
    if (dbg) Serial.print(" sPtr2=");
    if (dbg) Serial.print(sPtr2);
    if (dbg) Serial.print(" Data1=");
    if (dbg) Serial.println(Data1);
    ITemp2 = (float)Data1;  //  Second data is ITemp2, the temp inside the capsule
    sPtr1 = sPtr2 + 1;
    sPtr2 = replyLength;
    Data1 = Xavier2.substring(sPtr1, sPtr2).toInt();
    if (dbg) Serial.print("sPtr1=");
    if (dbg) Serial.print(sPtr1);
    if (dbg) Serial.print(" sPtr2=");
    if (dbg) Serial.print(sPtr2);
    if (dbg) Serial.print(" Data1=");
    if (dbg) Serial.println(Data1);
    TxTemp = (float)Data1;  // Third data id TxTemp, the temperature of the 436 MHz video transmitter
    MonkeyDataReceived = true;
    MonkeyOK = true;
    monkey_busy = false;
  }
}
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////

void loop() {
  if (millis() >= next_sec + 100) time_update(0);
  if (string1Complete) ProcessOwnGPS();
  if (string0Complete) Process_CMD();
  if (ABORT_flg) Process_ABORT();
  if (MXALT_flg) Process_MXALT();
  if (IDENT_flg) Process_IDENT();
  // if (MONKEYCMD_flg) Process_MONKEYCMD();             // commented out, flag not used LJR March 2025
  if (Monkey_reply_flg) Process_MONKEYREPLY();  // received something from monkey

  /////////////////////////////////////////////////////   Do every second  gtm

  if (millis() >= looptime1 + 1000) {
    looptime1 += 1000;
    if (BME_OK) {
      psTemp = bme.readTemperature();
      psPressure = bme.readPressure();
      Pressure_alt = (bme.readAltitude(SLP)) * 3.281;
      //      Pressure_alt = 100 *3.281;                  commented out 11/18/19 LJR
    }
    //if ((MIS_sec % 30 == 0) && (MonkeyOK) && (!monkey_busy)) Serial2.println("MONKEY 12");
    if ((MIS_sec % 30 == 0) && (MonkeyOK) && (!monkey_busy)) SerialBT.println("MONKEY 12");
    //if ((MIS_sec % 20 == 0) && (!MonkeyOK)) Serial2.println("MONKEY 09");
    if ((MIS_sec % 20 == 0) && (!MonkeyOK)) SerialBT.println("MONKEY 09");
    if (millis() - LastMonkeyReply > 65000) MonkeyOK = false;
  }

  if (PARM_flg) {
    Serial1.print(F(":AB1JC-11 :PARM.TxTmp,ITmp1,ITmp2,OTemp,Vspd,GPSok,Time,COM,Alt,Mok,PRE,UP,DN\n"));  // edited 11/18/19 LJR
    PARM_flg = false;
  }
  if (UNIT_flg) {
    Serial1.print(F(":AB1JC-11 :UNIT.deg.C,deg.C,deg.C,deg.C,ft/s,valid,sync,OK,CAL\n"));  // edited 9/26/19 LJR
    UNIT_flg = false;
  }
  if (EQU_flg) {
    Serial1.print(F(":AB1JC-11 :EQNS.0,1,-100,0,1,-100,0,1,-100,0,1,-100,0,1,-200\n"));  // edited 9/26/19 LJR
    EQU_flg = false;
  }
  if (BIT_flg) {
    Serial1.print(F(":AB1JC-11 :BITS.11111111, Univ.Bridgeport USIP Balloon\n"));
    BIT_flg = false;
  }
/* Removed 3 3/22/25 HTH
        if (!GPSok) {
          if ((Pressure_alt > (max_alt * 1000)) && rising) {
            ABORT_flg = true;
            aborting = true;
            endAbort = millis() + 4000;
            Serial.println("ABORTING - above Pressure Max Altitude !!!");
            Serial1.println("ABORTING - above Pressure Max Altitude !!!");
          }
        }

  OTempraw = analogRead(OT_pin);
  ITempraw = analogRead(IT_pin);
  Batvraw = analogRead(V_pin);
  */
  Batv1 = ((float)Batvraw) / 102.85;                  // Calibrated 10/31/19 LJR
  OTemp = ((((float)OTempraw) / 1024) * 500) - 273;   // Calibrated 8/14/19 LJR
  ITemp1 = ((((float)ITempraw) / 1024) * 500) - 273;  // Calibrated 8/14/19 LJR

  // L. Reed added this 8/14/19 to calibrate sensors
  /*    if (dbg) Serial.print("OTemp: ");
    if (dbg) Serial.print(OTemp);
    if (dbg) Serial.print(", ITemp: ");
    if (dbg) Serial.print(ITemp);
    if (dbg) Serial.print(", psPressure: ");
    if (dbg) Serial.print(psPressure);
    if (dbg) Serial.print(", Pressure_alt: ");
    if (dbg) Serial.print(Pressure_alt);
    if (dbg) Serial.print("    ");
*/
  // end of L. Reed adds

  /*    if (dbg) Serial.print(psTemp);
    if (dbg) Serial.print(", ");
    if (dbg) Serial.print(psPressure/100,2);
    if (dbg) Serial.print(", ");
    if (dbg) Serial.print(Pressure_alt, 0);
    if (dbg) Serial.print(", ");
    if (dbg) Serial.print(GPS_alt, 0);
    if (dbg) Serial.print(",  ");
    if (dbg) Serial.print(Batv);
    if (dbg) Serial.print(", ");
    if (dbg) Serial.print(OTemp);
    if (dbg) Serial.print(", ");
    if (dbg) Serial.print(ITemp);
    if (dbg) Serial.print(", ");
    if (dbg) Serial.print(max_alt,0);
    if (dbg) Serial.print(",000 ft.");
    if (dbg) Serial.print("   ");
    if (ABORT_flg) Serial.print("ABORT");
    if (dbg) Serial.print(" "); 
    if (dbg) Serial.print(GPS_date);
    if (dbg) Serial.print(" "); 
    if (dbg) {
    if (GPS_hr < 10) Serial.print(ZERO_Str);
    Serial.print(GPS_hr);
    Serial.print(":");
    if (GPS_min < 10) Serial.print(ZERO_Str);
    Serial.print(GPS_min);
    Serial.print(":");
    if (GPS_sec < 10) Serial.print(ZERO_Str);
    Serial.print(GPS_sec);
    Serial.print(" ");
    if (GPS_valid) {Serial.print(ONE_Str);}
    else {Serial.print(ZERO_Str);}
    Serial.print(COMMA_Str);
    if (time_sync) {Serial.print(ONE_Str);}
    else {Serial.print(ZERO_Str);}
    Serial.print(COMMA_Str);
    if (Alt_Cal) {Serial.print(ONE_Str);}
    else {Serial.print(ZERO_Str);}
    Serial.print(COMMA_Str);
    if (prelaunch) {Serial.print(ONE_Str);}
    else {Serial.print(ZERO_Str);}
    Serial.print(COMMA_Str);
    if (rising) {Serial.print(ONE_Str);}
    else {Serial.print(ZERO_Str);}
    Serial.print(COMMA_Str);
    if (aborting) {Serial.print(ONE_Str);}
    else {Serial.print(ZERO_Str);}
    Serial.print(COMMA_Str);
    if (falling) {Serial.print(ONE_Str);}
    else {Serial.print(ZERO_Str);}
    Serial.print(COMMA_Str);
    if (landed) {Serial.println(ONE_Str);}
    else {Serial.println(ZERO_Str);}
    }
*/
  // if millis() > looptime1 + 1000
  /////////////////////////////////////////////////////   Do every minute  gtm
  if (millis() >= looptime4 + 60000) {
    looptime4 = millis();
    Tseq += 1;
    Vspeed = ((GPS_alt - GPS_alt1) + ((GPS_alt1 - GPS_alt2) / 2)) / 15;
    telem_OTemp = (int)(OTemp + 100);
    telem_ITemp1 = (int)(ITemp1 + 100);
    telem_ITemp2 = (int)(ITemp2 + 100);
    telem_TxTemp = (int)(TxTemp + 100);
    telem_Vspeed = (int)(Vspeed + 200.0);
    // now check limits for 0-255
    constrain(telem_ITemp1, 0, 255);
    constrain(telem_ITemp2, 0, 255);
    constrain(telem_OTemp, 0, 255);
    constrain(telem_TxTemp, 0, 255);
    constrain(telem_Vspeed, 0, 255);

    Serial1.print("T#");
    if (Tseq < 100) Serial1.print(ZERO_Str);
    if (Tseq < 10) Serial1.print(ZERO_Str);
    Serial1.print(Tseq);
    Serial1.print(COMMA_Str);
    Serial1.print(telem_TxTemp);
    Serial1.print(COMMA_Str);
    Serial1.print(telem_ITemp1);
    Serial1.print(COMMA_Str);
    Serial1.print(telem_ITemp2);
    Serial1.print(COMMA_Str);
    Serial1.print(telem_OTemp);
    Serial1.print(COMMA_Str);
    Serial1.print(telem_Vspeed);
    Serial1.print(COMMA_Str);
    // Now send the bits:
    //                 GPS_valid time_sync COMMSok Alt_CAL MonkeyOK  prelaunch rising falling


    if (GPS_valid) {
      Serial1.print(ONE_Str);
    } else {
      Serial1.print(ZERO_Str);
    }
    if (time_sync) {
      Serial1.print(ONE_Str);
    } else {
      Serial1.print(ZERO_Str);
    }
    if (COMMSok) {
      Serial1.print(ONE_Str);
    } else {
      Serial1.print(ZERO_Str);
    }
    if (Alt_Cal) {
      Serial1.print(ONE_Str);
    } else {
      Serial1.print(ZERO_Str);
    }
    if (MonkeyOK) {
      Serial1.print(ONE_Str);
    } else {
      Serial1.print(ZERO_Str);
    }
    if (prelaunch) {
      Serial1.print(ONE_Str);
    } else {
      Serial1.print(ZERO_Str);
    }
    if (rising) {
      Serial1.print(ONE_Str);
    } else {
      Serial1.print(ZERO_Str);
    }
    if (falling) {
      Serial1.println(ONE_Str);
    } else {
      Serial1.println(ZERO_Str);
    }
    //////////////////////////////////////////

    Serial.print("T#");
    if (Tseq < 100) Serial.print(ZERO_Str);
    if (Tseq < 10) Serial.print(ZERO_Str);
    Serial.print(Tseq);
    Serial.print(COMMA_Str);
    Serial.print(telem_TxTemp);
    Serial.print(COMMA_Str);
    Serial.print(telem_ITemp1);
    Serial.print(COMMA_Str);
    Serial.print(telem_ITemp2);
    Serial.print(COMMA_Str);
    Serial.print(telem_OTemp);
    Serial.print(COMMA_Str);
    Serial.print(telem_Vspeed);
    Serial.print(COMMA_Str);
    // Now send the bits:
    //                 GPS_valid time_sync COMMSok Alt_CAL MonkeyOK  prelaunch rising falling


    if (GPS_valid) {
      Serial.print(ONE_Str);
    } else {
      Serial.print(ZERO_Str);
    }
    if (time_sync) {
      Serial.print(ONE_Str);
    } else {
      Serial.print(ZERO_Str);
    }
    if (COMMSok) {
      Serial.print(ONE_Str);
    } else {
      Serial.print(ZERO_Str);
    }
    if (Alt_Cal) {
      Serial.print(ONE_Str);
    } else {
      Serial.print(ZERO_Str);
    }
    if (MonkeyOK) {
      Serial.print(ONE_Str);
    } else {
      Serial.print(ZERO_Str);
    }
    if (prelaunch) {
      Serial.print(ONE_Str);
    } else {
      Serial.print(ZERO_Str);
    }
    if (rising) {
      Serial.print(ONE_Str);
    } else {
      Serial.print(ZERO_Str);
    }
    if (falling) {
      Serial.println(ONE_Str);
    } else {
      Serial.println(ZERO_Str);
    }
  }

  ////////////////////////////////////////////////////// Do every 10 seconds
  if (PhaseCalc_flg) {
    pa4 = pa3;
    pa3 = pa2;
    pa2 = pa1;
    pa1 = Pressure_alt;

    if (GPS_alt3 != 0) {
      VspeedAvg = ((GPS_alt - GPS_alt1) + (GPS_alt1 - GPS_alt2) + (GPS_alt2 - GPS_alt3)) / 15;
      PA_VspeedAvg = ((pa1 - pa2) + (pa2 - pa3) + (pa3 - pa4)) / 30;
      if (dbg) Serial.print("VspeedAvg = ");
      if (dbg) Serial.print(VspeedAvg);
      if (dbg) Serial.print("  PA_VspeedAvg = ");
      if (dbg) Serial.println(PA_VspeedAvg);
    }
    if ((millis() - lastGPS) > 21000) {
      VspeedAvg = ((pa1 - pa2) + (pa2 - pa3) + (pa3 - pa4)) / 30;
      if (dbg) Serial.println("Using PA...");
    }
    if (!prelaunch && !rising && !falling && !landed) {
      if ((VspeedAvg < 2) && (VspeedAvg > -2)) prelaunch = true;
      if (VspeedAvg > 10) rising = true;
      if (VspeedAvg < -10) falling = true;
    }
    if ((prelaunch) && (GPS_alt3 != 0)) {
      if (((GPS_alt - GPS_alt1) > 40) && ((GPS_alt1 - GPS_alt2) > 40) && ((GPS_alt2 - GPS_alt3) > 40)) {
        rising = true;
        prelaunch = false;
        Launch_alt = GPS_alt3;
      }
      if ((VspeedAvg < 1) && (VspeedAvg > -1) && (GPS_alt3 != 0)) {
        AvgAlt = (GPS_alt + GPS_alt1 + GPS_alt2 + GPS_alt3) / 4;
        //        SLP = (bme.readSealevelPressure(AvgAlt/3.28));
        Alt_Cal = true;
      }
    }
    if (rising) {
      if (VspeedAvg < -10) {
        rising = false;
        falling = true;
      }
    }
    if (falling) {
      if ((VspeedAvg > -4) && (VspeedAvg < 4)) {
        falling = false;
        landed = true;
      }
      if (GPS_alt < max_alt * 1000) aborting = false;
      //      if (Pressure_alt < max_alt * 1000) aborting = false;
    }
    PhaseCalc_flg = false;
  }
}

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
    //  echo it
    //  Serial1.print(inChar1);
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

/*
               SerialEvent occurs whenever a new data comes in the
               hardware serial RX.  This routine is run between each
               time loop() runs, so using delay inside loop can delay
               response.  Multiple bytes of data may be available.
               */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar0 = (char)Serial.read();
    // add it to the inputString:
    inStr0 += inChar0;
    if (inStr0.length() > 11) {
      inStr0 = "";
    }
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar0 == '\n') {
      string0Complete = true;
    }
  }
}
///////////////////// Receive from Monkey ////////////////////////
/* SerialEvent2 MONKEY COMMAND occurs whenever a new data comes in the
from USIP Monkey Serial Line.  This routine is run between each
time loop() runs, so using delay inside loop can delay
response.  Multiple bytes of data may be available.

void serialEvent2() {
  //             Serial.println("Entered received from Monkey serialEvent2()");
  while (Serial2.available()) {
    char inChar2 = (char)Serial2.read();  // get the new byte
    inStr2 += inChar2;                    // add it to the inputString:
    if (inStr2.length() > 85) {
      inStr2 = "";
    }
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar2 == '\n') {
      Monkey_reply_flg = true;
      //                   inStr2 =(tryit + inStr2);
      if (dbg) Serial.println("Temp String is: " + inStr2);
      //                   if (dbg) Serial.print("It is this many characters long: ");
      //                   if (dbg) Serial.println(inStr2.length());
      //                   Xavier2 = inStr2.substring(0,9);
      //                   if (dbg) Serial.println("The substring is: " + Xavier2);
      sPtr3 = inStr2.indexOf("MONKEY");
      MonkeyReplyStr = inStr2.substring(sPtr3);  //MonkeyReplyStr must be the WHOLE string to parse data coming back
                                                 //                   Serial.println(MonkeyReplyStr);
      Xavier2 = MonkeyReplyStr.substring(0, 9);
      Serial.print("===> ");
      Serial.print(Xavier2);
      Serial.print(", ");
      Serial.println(MonkeyReplyStr);
      string2Complete = true;
      inStr2 = "";  //Clear out string variable
    }
  }
}
*/

///////////////////// Receive from Monkey ////////////////////////
/* SerialEventBT MONKEY COMMAND occurs whenever a new data comes in the
from USIP Monkey Serial Line.  This routine is run between each
time loop() runs, so using delay inside loop can delay
response.  Multiple bytes of data may be available.
*/
void serialEventBT() {
  while (SerialBT.available()) {  // Read from Bluetooth instead of Serial2
    char inCharBT = (char)SerialBT.read();  // Read incoming byte
    inStr2 += inCharBT;                     // Append to the input string

    if (inStr2.length() > 85) {
      inStr2 = "";  // Reset if the string is too long
    }

    if (inCharBT == '\n') {  // Check for newline (end of command)
      Monkey_reply_flg = true;

      if (dbg) Serial.println("Temp String is: " + inStr2);

      sPtr3 = inStr2.indexOf("MONKEY");
      MonkeyReplyStr = inStr2.substring(sPtr3);

      Xavier2 = MonkeyReplyStr.substring(0, 9);
      Serial.print("===> ");
      Serial.print(Xavier2);
      Serial.print(", ");
      Serial.println(MonkeyReplyStr);

      string2Complete = true;

      inStr2 = "";  // Clear the string buffer
    }
  }
}