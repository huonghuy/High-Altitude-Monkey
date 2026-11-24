 /*
// PROGRAM FOR USIP_1  (DISCOVERY III)

 */

#include <Wire.h>
#include <Adafruit_Sensor.h> // adafruit unified sensor library
#include <Adafruit_BME280.h> // adafruit BME280 library

#define SEALEVELPRESSURE_HPA (101325.0) // Updated

//////////// SOFTWARE VERSION NUMBER ////////////////
#define VERSION "0.2"  
////////////////////////////////////////////////////

#define DEBUG // Turn on Debug printing.  This sends debug info to the IDE serial port

  // PARALAX LCD COMMANDS
    #define LCD_BKLITEOFF 18  // TURN OFF LCD BACK LIGHT
    #define LCD_BKLITEON 17   // TURN ON LCD BACK LIGHT
    #define LCD_MOVECURL0P0 128  // move cursor line 0 position 0
    #define LCD_MOVECURL0P1 129  // move cursor line 0 position 1   
    #define LCD_MOVECURL0P2 130  // move cursor line 0 position 2 
    #define LCD_MOVECURL0P3 131  // move cursor line 0 position 3
    #define LCD_MOVECURL0P4 132  // move cursor line 0 position 4
    #define LCD_MOVECURL0P5 133  // move cursor line 0 position 5
    #define LCD_MOVECURL0P6 134  // move cursor line 0 position 6
    #define LCD_MOVECURL0P7 135  // move cursor line 0 position 7
    #define LCD_MOVECURL0P8 136  // move cursor line 0 position 8
    #define LCD_MOVECURL0P9 137  // move cursor line 0 position 9
    #define LCD_MOVECURL0P10 138 // move cursor line 0 position 10
    #define LCD_MOVECURL0P11 139 // move cursor line 0 position 11
    #define LCD_MOVECURL0P12 140 // move cursor line 0 position 12
    #define LCD_MOVECURL0P13 141 // move cursor line 0 position 13
    #define LCD_MOVECURL0P14 142 // move cursor line 0 position 14
    #define LCD_MOVECURL0P15 143 // move cursor line 0 position 15

    #define LCD_MOVECURL1P0 148  // move cursor line 1 position 0
    #define LCD_MOVECURL1P1 149  // move cursor line 1 position 1
    #define LCD_MOVECURL1P2 150  // move cursor line 1 position 2
    #define LCD_MOVECURL1P3 151  // move cursor line 1 position 3
    #define LCD_MOVECURL1P4 152  // move cursor line 1 position 4
    #define LCD_MOVECURL1P5 153  // move cursor line 1 position 5
    #define LCD_MOVECURL1P6 154  // move cursor line 1 position 6
    #define LCD_MOVECURL1P7 155  // move cursor line 1 position 7
    #define LCD_MOVECURL1P8 156  // move cursor line 1 position 8
    #define LCD_MOVECURL1P9 157  // move cursor line 1 position 9
    #define LCD_MOVECURL1P10 158 // move cursor line 1 position 10
    #define LCD_MOVECURL1P11 159 // move cursor line 1 position 11
    #define LCD_MOVECURL1P12 160 // move cursor line 1 position 12
    #define LCD_MOVECURL1P13 161 // move cursor line 1 position 13
    #define LCD_MOVECURL1P14 162 // move cursor line 1 position 14
    #define LCD_MOVECURL1P15 163 // move cursor line 1 position 15

    #define LCD_LEFT 8 // move cursor left one space
    #define LCD_RIGHT 9 // move cursor right one space
    #define LCD_LF 10 // Line Feed cursor
    


Adafruit_BME280 bme;
float SLP = 101325;         //delete?
String ONE_Str = "1";
String ZERO_Str = "0";
String COMMA_Str = ",";
String inStr1 = "";         // a string to hold incoming data from GPS
String inStr0 = "";         // a string to hold incoming data the keyboard

String CMD_Str = "";        // the 2-letter command, then we build the command string

//String PARM_Str = ":AB1JC-11 :PARM.Battery,Itemp,OTemp,PrAlt,Vspd,GPSok,Time,Alt,PRE,UP,CUT,DN,LND\n";
//String UNIT_Str = ":AB1JC-11 :UNIT.volts,deg.C,deg.C,feet,ft/s,valid,sync,CAL\n";
//String EQU_Str = ":AB1JC-11 :EQNS.0,0.0333,0,0,1,-100,0,1,-100,0,333.3,0,0,1,-200\n";
//String BIT_Str = ":AB1JC-11 :BITS.11111111, Discovery2 Balloon\n";


char CMD_char = '0';   // all commands are essentially a single letter
char CMD_char2 = '0';
String GGAStr = "";         // The string from the GPS
String GGAprimeStr;
// String HASPStr = "";
boolean HASP_send_flag = false;
int HASP_send_sec = 00;
String TSTStr = "";
String TempStr = "";
boolean string1Complete = false;  // whether the inStr1 is complete
boolean string0Complete = false;  // whether the inStr0 is complete
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
float ITemp = 0;
float OTemp = 0;
float Batv = 0;
float psTemp = 0;
float psPressure = 0;
float Pressure_alt = 0;
float pa1;
float pa2;
float pa3;
float pa4;
int telem_pres_alt = 0;
int telem_ITemp = 0;
int telem_OTemp = 0;
int telem_Batv = 0;
int telem_PA = 0;
int Tseq = 0;
int telem_Vspeed = 0;
float GPS_in;
float GPS_alt1 = 0;
float GPS_alt2 = 0;
float GPS_alt3 = 0;
float Launch_alt = 0;
float AvgAlt = 0;
float Vspeed = 0;
float VspeedAvg = 0;


//////////////////////////////
//pins
int ledpin = 13;
int ABORT_pin = 2; // abort of the monkey and cuts balloon away
int OT_pin = A0; // outside temp
int IT_pin = A1; // inside temp
int TX_pin = A2; //Transmitter temperature
int V_pin = A3; // battery voltage/3.47 for voltage divider

////////////////////////////////////////
unsigned long lastloop = 0;
unsigned long looptime = 0;
unsigned long looptime1 = 0 ;
unsigned long looptime2 = 0;
unsigned long looptime3 = 0;
unsigned long looptime4 = 0;
unsigned long endAbort;
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
boolean IDENT_flg = false;
boolean MXALT_flg = false;
boolean GPS_valid = false;
boolean PhaseCalc_flg = false;
boolean Alt_Cal = false;
int max_alt = 128;                   /////////////  NOW DEFAULT TO 128,000
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



void setup() {
  pinMode(ABORT_pin,OUTPUT);       // ABORT pin - LOW Active
  digitalWrite(ABORT_pin,HIGH);
  pinMode (ledpin, OUTPUT);
  pinMode(OT_pin, INPUT); //Outside Temp
  pinMode (IT_pin, INPUT); // inside temp
  pinMode(TX_pin, INPUT); // transmitter temp
  pinMode (V_pin, INPUT);

  
  // initialize serial:
  for (x=0; x<5; x++)  {
    digitalWrite(ledpin,HIGH);
    delay(100);
    digitalWrite(ledpin,LOW);
    delay(100);
  }  
  // set up the rates of all the serial communications
  Serial.begin(57600); // monitor in the IDE tool for commands and output
  Serial1.begin(19200);// Tiny Trak 4
  Serial2.begin(19200);// monkey cpu
  Serial3.begin(9600);  //  debug display Paralax Serial LCD Display
  
  Serial.print("USIP Version ");
  Serial.println (VERSION); 
  
// move cursor on LCD to line zero position 0 
  #ifdef DEBUG 
   Serial3.write(LCD_BKLITEON);
   LCD_Clear(); // clear the LCD display
   Serial3.print("Pharoah Larry");

   delay(2000);
   LCD_Clear(); // clear the LCD display
  #endif
  
//  

  // reserve 100 bytes for the input strings inStr1 and inStr2:
  GPS_date.reserve(8);
  inStr1.reserve(100);
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
        Serial.println(bme.sensorID(),16);
  }
}

////////////////////////////////////////////////////////////////

void time_update(int update_source) {    // 0 = internal update, 1 = OWN_GPS, 2 = HASP_GPS
  if (time_sync && !last_time_sync) {     // If we are JUST NOW receiving our first valid GPS message.
    last_time_sync = true;      // we will do this section only once
    time_sync_at = millis();     // Note the time we do it.
    if (update_source == 1) {
      MIS_sec = GPS_sec;  // set the Mission seconds EQUAL to the OWN_GPS seconds
      next_sec = lastGPS + 1000;  // set the "next_second" variable based on the receipt of the OWN_GPS $GPGGA message.
      Serial.print("Time Sync to OWN_GPS at ");
    }
    //else if (update_source == 2) {
    //  Serial.print("Bad update source - HASP - this is HASP Sim - ");
    //}
    else {
      Serial.print("Time Sync Error - ");         // Error - shouldn't get here. Clear time sync and last_time_sync & decr MIS_min
      MIS_min -= 1;
      time_sync = false;
      last_time_sync = false;
      next_sec += 1000;
    }
    Serial.print(time_sync_at/1000);
    Serial.println(" seconds.");
    MIS_min += 1;             // and arbitrarily advance the Mission clock to the next minute.
    if (MIS_min >= 60) {       // then update Mission hours if necessary
      MIS_min = 0;
      MIS_hr += 1;
    }
  }
  else if (!time_sync) {   // We have not received any valid GPS messages yet.
    next_sec += 1000;      // update "next_second" variable based on internal clock
    MIS_sec += 1;          // ...and update Mission time manually
    if (MIS_sec >= 60) {
      MIS_sec = 0;
      MIS_min += 1;
      if (MIS_min >= 60) {
        MIS_min = 0;
        MIS_hr += 1;       // GPS time rolls over at 23:59:59.  Mission time does not - hours keep counting.
      }
    }
  }
  else {   // we are in time sync.  Did the GPS not come in this second?
    if (update_source == 0) {   // No. update GPS time manually...
      if (millis() - lastGPS > 60000) GPS_valid = false;
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
      MIS_sec += 1;     //  ... and also update Mission time...
      if (MIS_sec == 60) {
        MIS_sec = 0;
        MIS_min += 1;
        if (MIS_min == 60) {
          MIS_min = 0;
          MIS_hr += 1;
        }
      }
      next_sec += 1000;     // ...and set "next_second" internally 
      if ((MIS_min % 20 == 5) && (MIS_sec == 0)) PARM_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 10)) UNIT_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 20)) EQU_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 30)) BIT_flg = true;
      if (MIS_sec % 10 == 2) {
        PhaseCalc_flg = true;
      }

    }
    else if (update_source == 1)   { // we are in time sync, and the OWN_GPS has just come in
      if (MIS_sec < 10 && GPS_sec >50) {    // Internal time is ahead of the GPS and has gone to the next minute
        MIS_min -= 1;
        if (MIS_min == -1) {
          MIS_min = 59;
          MIS_hr -= 1;
          if (MIS_hr > 0) MIS_hr = 0;
        }
      }
      else if (MIS_sec > 50 && GPS_sec < 10) {  // Internal time is behind GPS.  GPS has gone to the next minute.
        MIS_min += 1;
        if (MIS_min >= 60) {
          MIS_min = 0;
          MIS_hr += 1;
        }
      }
      else {
      }
      MIS_sec = GPS_sec;  // and then set Mission seconds = GPS seconds.
      next_sec = lastGPS+1000;  // update "next_second" variable to the time the next GPS message is expected.
      if ((MIS_min % 20 == 5) && (MIS_sec == 0)) PARM_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 10)) UNIT_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 20)) EQU_flg = true;
      if ((MIS_min % 20 == 5) && (MIS_sec == 30)) BIT_flg = true;
      if (MIS_sec % 10 == 2) {
        PhaseCalc_flg = true;
        Serial.print("Now --");
      }
    }
    /*else if (update_source == 2) {   // we are in time sync, and the HASP_GPS has just come in
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
    }    */
    else {
    }
  }
  //if (time_sync && MIS_sec == HASP_send_sec) HASP_send_flag = true;
  Serial.print("GPS_Time: ");
  if (GPS_hr < 10) Serial.print(ZERO_Str);
  Serial.print(GPS_hr);
  Serial.print(":");
  if (GPS_min < 10) Serial.print(ZERO_Str);
  Serial.print(GPS_min);
  Serial.print(":");
  if (GPS_sec < 10) Serial.print(ZERO_Str);
  Serial.print(GPS_sec);
  Serial.print("\t");
  Serial.print("Mission_Time: ");
  if (MIS_hr < 10) Serial.print(ZERO_Str);
  Serial.print(MIS_hr);
  Serial.print(":");
  if (MIS_min < 10) Serial.print(ZERO_Str);
  Serial.print(MIS_min);
  Serial.print(":");
  if (MIS_sec < 10) Serial.print(ZERO_Str);
  Serial.println(MIS_sec); 
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
  if (millis()- lastGPS >= 1049) {
    Serial3.print(" I ");
  }
  else {Serial3.print(" G ");
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
  if (time_sync) {
    Serial3.print(" S ");
  }
  else {
    Serial3.print("   ");
  }
  Serial3.write(LCD_MOVECURL1P15);     
}

/////////////////////////////////////////////////////////////////////////
void Process_CMD()   {
  CMD_Str = inStr0;
  inStr0 = "";
  string0Complete = false;
//
  if ((CMD_Str.length() > 6 ) && (CMD_Str.length() < 12)) {
    TSTStr = CMD_Str.substring (0, 6);
      if (TSTStr == "$ABORT") {      //Decode $ABORT message here. If not goto next else_if below.
        ABORT_flg = true;
        aborting = true;
        endAbort = millis() + 4000;
        Serial.println("ABORTING!!!");
        Serial1.println("ABORTING!!!");
      }        
      else if (TSTStr == "$MXALT") {      // Decode $MXALT message here. Set Maximum Altitude and prep response.   If not goto next else_if below.
        MXALT_flg = true;
        max_alt = CMD_Str.substring(7,9).toInt();
        Serial.print("Max Altitude is now ");
        Serial.print(max_alt);
        Serial.println(",000 feet.");
      }        
      else if (TSTStr == "$IDENT") {      // Decode $IDENT message here. Process an output message. If not goto next else_if below.
        IDENT_flg = true;
        Serial.println("Discovery Museum High Altitude Balloon");
      }        
      else {                         // Come here if not any valid message or command. 
        Serial.print(TSTStr);
        Serial.println(" ??");       // and do nothing
      }         
  }
  else {
    Serial.print(TSTStr);
    Serial.print(" ?? (");
    Serial.print(CMD_Str.length());
    Serial.println(")");
  }  
}
//////////////////////////////////////////////////////////////////////////////
void ProcessOwnGPS() {
//    looptime = micros();
//    Serial.print("Got 1 - ");
    indx_dollar = inStr1.indexOf('$');
    if (indx_dollar >= 0)  {
      GGAStr = inStr1.substring(indx_dollar);
      inStr1 = "";
      string1Complete = false;
      Serial.println(GGAStr);
//      looptime1 = micros();
      TSTStr = GGAStr.substring(1, 6);
//      Serial.print(TSTStr);
//      Serial.print(" ");
      if (TSTStr == "GPGGA")  {             // Decode $GPGGA message here. If not, jump to else below.
//        Serial.println("");
//        Serial.print("GPGGA decode, ");
        // first check validity and checksum
//        looptime1 = micros();
        fm_indx = 6;
        for (x = 1; x < 6; x++)  {
          to_indx = GGAStr.indexOf(',', fm_indx + 1);
          fm_indx = to_indx;
        }
        TSTStr = GGAStr.substring(fm_indx + 1, fm_indx + 2);
        if (TSTStr != "0")  {                       // 0 is invalid fix. 1 is GPS fix, 2 is DGPS fix.
//          looptime2 = micros();
          // copy string to character array
          len = GGAStr.length();
          GGAStr.toCharArray(tst_chars,len +1);
//          looptime3 = micros();
          fm_indx = 1;
          to_indx = GGAStr.lastIndexOf('*');
          one_char = tst_chars[to_indx+1] - '0';
          if (one_char > 9) one_char -= 7;
          cksm = one_char << 4;
          one_char = tst_chars[to_indx+2] - '0';
          if (one_char > 9) one_char -= 7;
          cksm += one_char;
          sum2chk = 0;
          for (x = fm_indx; x < to_indx; x++) {
            one_char = tst_chars[x];
            sum2chk ^= one_char;
          }
         if (sum2chk == cksm)  {
          GPS_hr = (GGAStr.substring(7,9)).toInt();
          GPS_min = (GGAStr.substring(9,11)).toInt();
          GPS_sec = (GGAStr.substring(11,13)).toInt();
//          next_sec = millis() + 1000;
          fm_indx = GGAStr.indexOf(",",13) + 1;
          to_indx = GGAStr.indexOf(",",fm_indx);
          Lat_deg = (GGAStr.substring(fm_indx, fm_indx + 2)).toInt();
          Lat_min = (GGAStr.substring(fm_indx + 2, to_indx)).toFloat();
          Lat_direc = GGAStr.charAt(to_indx + 1);
          fm_indx = to_indx + 3;
          to_indx = GGAStr.indexOf(",",fm_indx);
          Lon_deg = (GGAStr.substring(fm_indx, fm_indx + 3)).toInt();
          Lon_min = (GGAStr.substring(fm_indx + 3, to_indx)).toFloat();
          Lon_direc = GGAStr.charAt(to_indx + 1);
          fm_indx = to_indx + 5;
          num_sat = (GGAStr.substring(fm_indx, fm_indx + 2)).toInt();
          to_indx = GGAStr.indexOf(",",fm_indx);
          fm_indx = GGAStr.indexOf(",",to_indx +1) + 1;
          to_indx = GGAStr.indexOf(",",fm_indx +1);
          GPS_alt3 = GPS_alt2;
          GPS_alt2 = GPS_alt1;
          GPS_alt1 = GPS_alt;
          GPS_in = (GGAStr.substring(fm_indx, to_indx)).toFloat();
          GPS_alt = GPS_in * 3.281;
          time_sync = true;
          GPS_valid = true;
          lastGPS = millis();
          time_update(1);

         }                           // END of processing for valid message with good checksum
         else {                       // Come here if checksum doesn't match - ignore message
          Serial.println("Bad checksum");
          GPS_valid = false;
         }
        }                                 // END of processing for a Valid Fix.
      }                                   // END of $GPGGA message processing
      else if (TSTStr == "GPRMC") {                  // Come here if not $GPGGA message.  Decode $GPRMC message here. If not goto else below.
//        Serial.print("RMC decode .... Valid: ");
        fm_indx = 0;
        to_indx = GGAStr.indexOf(",",fm_indx);
        fm_indx = to_indx + 1; 
        to_indx = GGAStr.indexOf(",",fm_indx);
        fm_indx = to_indx + 1;
        valid_char = GGAStr.charAt(fm_indx);
//        Serial.print(valid_char);
        if (valid_char == 'A')  {      // An 'A' here says the message is Valid
          fm_indx =0;
          for (x=0;x<9;x++) {
            to_indx = GGAStr.indexOf(",",fm_indx);
            fm_indx = to_indx + 1; 
          }
          to_indx = GGAStr.indexOf(",",fm_indx);
//          Serial.print("\t String_");
//          Serial.print(GGAStr.substring(fm_indx,to_indx));
          GPS_date = GGAStr.substring(fm_indx,to_indx);       // for valid messages, get the DATE string and keep it.
//          Serial.print("\t Date_");
//          Serial.println(GPS_date);
        }         // if the valid_char was a 'V' (for NOT VALID) drop to here and exit doing nothing.
      }           // END of $GPRMC processing
      else if (TSTStr == "ABORT") {      // Come here if not $GPGGA or $GPRMC message.  Decode $ABORT message here. If not goto next else_if below.
        aborting = true;
        ABORT_flg = true;
        endAbort = millis() + 4000;
        Serial1.println("ABORTING !!!");
      }        
      else if (TSTStr == "MXALT") {      // Decode $MXALT message here. Set Maximum Altitude and prep response.   If not goto next else_if below.
        MXALT_flg = true;
        max_alt = GGAStr.substring(7,9).toInt();
      }        
      else if (TSTStr == "IDENT") {      // Decode $IDENT message here. Process an output message. If not goto next else_if below.
        IDENT_flg = true;
      }        
      else {      // Come here if not any valid message or command.  
        }         // and do nothing
        }         // END of process if  '$'  in message
          else  {       //Come here if no '$' in the string - probably started in middle of message. Discard & reset.
          inStr1 = "";
          string1Complete = false;
          }    
    }
 
///////////////////////////////////////////////////////////////////////////////

 void Process_ABORT()  {
   digitalWrite(ABORT_pin,LOW);
   if (millis() > endAbort) {
     digitalWrite(ABORT_pin,HIGH);
     ABORT_flg = false;
   }
 }  
/////////////////////////////////////////////////////////////////////////////////   
 void Process_MXALT()  {
   MXALT_flg = false;
   Serial1.print("New Max Alt = ");
   Serial1.print(max_alt);
   Serial1.println(",000 ft.");
 }  
///////////////////////////////////////////////////////////////////////////////
/// this routine clears the display and leaves cursor at line 0 position 0
void LCD_Clear()
{
   Serial3.write(LCD_MOVECURL0P0);    // clear display line 0
   Serial3.print("                                "); 
   Serial3.write(LCD_MOVECURL0P0);
}
//////////////////////////////////////////////////////////////////////////////

 void Process_IDENT()  {
//     Serial1.print(GPS_date);
//     Serial1.print(" ");
  
     if (GPS_hr < 10) Serial1.print(ZERO_Str);
     Serial1.print(GPS_hr);
     Serial1.print(":");
     if (GPS_min < 10) Serial1.print(ZERO_Str);
     Serial1.print(GPS_min);
     Serial1.print(":");
     if (GPS_sec < 10) Serial1.print(ZERO_Str);
     Serial1.print(GPS_sec);
   Serial1.print(" MISSION: ");
  Serial1.print(MIS_hr);
  Serial1.print(":");
  if (MIS_min < 10) Serial1.print(ZERO_Str);
  Serial1.print(MIS_min);
  Serial1.print(" ");
  if (GPS_valid) {Serial1.print(ONE_Str);}
  else {Serial1.print(ZERO_Str);}
  if (time_sync) {Serial1.print(ONE_Str);}
  else {Serial1.print(ZERO_Str);}
  if (Alt_Cal) {Serial1.print(ONE_Str);}
  else {Serial1.print(ZERO_Str);}
  if (prelaunch) {Serial1.print(ONE_Str);}
  else {Serial1.print(ZERO_Str);}
  if (rising) {Serial1.print(ONE_Str);}
  else {Serial1.print(ZERO_Str);}
  if (aborting) {Serial1.print(ONE_Str);}
  else {Serial1.print(ZERO_Str);}
  if (falling) {Serial1.print(ONE_Str);}
  else {Serial1.print(ZERO_Str);}
  if (landed) {Serial1.print("1 ");}
  else {Serial1.print("0 ");}
  Serial1.print(max_alt);
  Serial1.print(",000");
  Serial1.println(" Discovery Museum High-Altitude Balloon");
  IDENT_flg = false;
 }  
////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////
  
void loop() {
  if (millis() >= next_sec + 100) time_update(0);
  if (string1Complete) ProcessOwnGPS();
  if (string0Complete) Process_CMD();
  if (ABORT_flg) Process_ABORT();
  if (MXALT_flg) Process_MXALT();
  if (IDENT_flg) Process_IDENT();


  /////////////////////////////////////////////////////   Do every second

  if (millis() > (looptime1 + 1000)) {
    looptime1 = millis();
    psTemp = bme.readTemperature(); 
    psPressure = bme.readPressure();
    Pressure_alt = bme.readAltitude(SLP)*3.281;
    if (PARM_flg) {
      Serial1.print(F(":AB1JC-11 :PARM.Battery,Itemp,OTemp,PrAlt,Vspd,GPSok,Time,Alt,PRE,UP,CUT,DN,LND\n"));
      PARM_flg = false;
    }
    if (UNIT_flg) {
      Serial1.print(F(":AB1JC-11 :UNIT.volts,deg.C,deg.C,feet,ft/s,valid,sync,CAL\n"));
      UNIT_flg = false;
    }
    if (EQU_flg) {
      Serial1.print(F(":AB1JC-11 :EQNS.0,0.0333,0,0,1,-100,0,1,-100,0,333.3,0,0,1,-200\n"));
      EQU_flg = false;
    }
    if (BIT_flg) {
      Serial1.print(F(":AB1JC-11 :BITS.11111111, Discovery2 Balloon\n"));
      BIT_flg = false;
    }

    if ((max_alt != 99) && !aborting) {
      if ((GPS_alt > max_alt * 1000) && rising) {
        ABORT_flg = true;
        aborting = true;
        endAbort = millis() + 4000;
        Serial.println("ABORTING!!!");
        Serial1.println("ABORTING!!!");
      }
      if ((Pressure_alt > max_alt * 1000) && rising) {
        ABORT_flg = true;
        aborting = true;
        endAbort = millis() + 4000;
        Serial.println("ABORTING!!!");
        Serial1.println("ABORTING!!!");
      }
    }
    OTempraw = analogRead(OT_pin);
    ITempraw = analogRead(IT_pin);
    Batvraw = analogRead(V_pin);
    Batv = ((float)Batvraw/1024)*10;
    OTemp = (((float)OTempraw/1024)*500)-273;
    ITemp = (((float)ITempraw/1024)*500)-273;
    
    
/*    Serial.print(psTemp);
    Serial.print(", ");
    Serial.print(psPressure/100,2);
    Serial.print(", ");
    Serial.print(Pressure_alt, 0);
    Serial.print(", ");
    Serial.print(GPS_alt, 0);
    Serial.print(",  ");
    Serial.print(Batv);
    Serial.print(", ");
    Serial.print(OTemp);
    Serial.print(", ");
    Serial.print(ITemp);
    Serial.print(", ");
    Serial.print(max_alt);
    Serial.print(",000 ft.");
    Serial.print("   ");
    if (ABORT_flg) Serial.print("ABORT");
    Serial.print(" "); 
    Serial.print(GPS_date);
    Serial.print(" "); 
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
*/
  }  

  /////////////////////////////////////////////////////   Do every minute
  if (millis() > looptime4 + 60000) {
    looptime4 = millis();
    Tseq += 1;
    Vspeed = ((GPS_alt - GPS_alt1)+((GPS_alt1 - GPS_alt2)/2)) /15;
    telem_OTemp = (int)(OTemp+100);
    telem_ITemp = (int)(ITemp+100);
    telem_Batv = (int)(Batv * 30);
    telem_PA = (int)Pressure_alt/333.33333;
    telem_Vspeed = (int)(Vspeed+200);
// now check limits for 0-255    
    constrain(telem_Batv, 0, 255);
    constrain(telem_ITemp, 0, 255);
    constrain(telem_OTemp, 0, 255);
    constrain(telem_PA, 0, 255);
    constrain(telem_Vspeed, 0, 255);
    
    Serial1.print("T#");
    
    if (Tseq < 100 ) {Serial1.print(ZERO_Str);}
    if (Tseq < 10 ) {Serial1.print(ZERO_Str);}
    Serial1.print(Tseq);
    Serial1.print(COMMA_Str);
    Serial1.print(telem_Batv);
    Serial1.print(COMMA_Str);
    Serial1.print(telem_ITemp);
    Serial1.print(COMMA_Str);
    Serial1.print(telem_OTemp);
    Serial1.print(COMMA_Str);
    Serial1.print(telem_PA);
    Serial1.print(COMMA_Str);
    Serial1.print(telem_Vspeed);
    Serial1.print(COMMA_Str);
    if (GPS_valid) {Serial1.print(ONE_Str);}
    else {Serial1.print(ZERO_Str);}
    if (time_sync) {Serial1.print(ONE_Str);}
    else {Serial1.print(ZERO_Str);}
    if (Alt_Cal) {Serial1.print(ONE_Str);}
    else {Serial1.print(ZERO_Str);}
    if (prelaunch) {Serial1.print(ONE_Str);}
    else {Serial1.print(ZERO_Str);}
    if (rising) {Serial1.print(ONE_Str);}
    else {Serial1.print(ZERO_Str);}
    if (aborting) {Serial1.print(ONE_Str);}
    else {Serial1.print(ZERO_Str);}
    if (falling) {Serial1.print(ONE_Str);}
    else {Serial1.print(ZERO_Str);}
    if (landed) {Serial1.print("1\n");}
    else {Serial1.print("0\n");}
  }
  
  ////////////////////////////////////////////////////// Do every 10 seconds
  if (PhaseCalc_flg) {
    pa4 = pa3;
    pa3 = pa2;
    pa2 = pa1;
    pa1 = Pressure_alt;

    if (GPS_alt3 != 0) {
      VspeedAvg = ((GPS_alt - GPS_alt1) + (GPS_alt1 - GPS_alt2)/2 + (GPS_alt2 - GPS_alt3)/4)/17.5;
    }
    if ((millis() - lastGPS) > 11000) {
      VspeedAvg = ((pa1 - pa2) + (pa2 - pa3)/2 + (pa3 - pa4)/4)/17.5; 
    }
    
    Serial.println (VspeedAvg);
#ifdef DEBUG       
   Serial.println (" printed VspeedAvg\n");
#endif

    if (!prelaunch && !rising && !falling && !landed) {
      if ((VspeedAvg < 2) && (VspeedAvg > -2)) prelaunch = true;
      if (VspeedAvg > 10) rising = true;
      if (VspeedAvg < -10) falling = true;
    }
    if ((prelaunch) && (GPS_alt3 != 0)) {
      if (((GPS_alt - GPS_alt1) > 80) && ((GPS_alt1 - GPS_alt2) > 80) && ((GPS_alt2 - GPS_alt3) > 80)) {
        rising = true;
        prelaunch = false;
        Launch_alt = GPS_alt3;
      }
      if ((VspeedAvg < 1) && (VspeedAvg > -1) && (GPS_alt3 !=0)) {
        AvgAlt = (GPS_alt + GPS_alt1 + GPS_alt2 + GPS_alt3)/4;
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
      if (Pressure_alt < max_alt * 1000) aborting = false;
    }
  PhaseCalc_flg = false;
}
}

               /*
               SerialEvent occurs whenever a new data comes in the\
               hardware serial RX.  This routine is run between each
               time loop() runs, so using delay inside loop can delay
               response.  Multiple bytes of data may be available.
               */
               void serialEvent1() 
               {
                  while (Serial1.available()) 
                  {
                  // get the new byte:
                  char inChar1 = (char)Serial1.read();
                  // add it to the inputString:
                  inStr1 += inChar1;
                  if (inStr1.length() > 97) 
                    {
                      inStr1 = "";
                    }
                  // if the incoming character is a newline, set a flag
                  // so the main loop can do something about it:
                  if (inChar1 == '\n') 
                    {
                      string1Complete = true;
                    }
                  }
             } // end serialEvent1()

                            /*
               SerialEvent occurs whenever a new data comes in the
               hardware serial RX.  This routine is run between each
               time loop() runs, so using delay inside loop can delay
               response.  Multiple bytes of data may be available.
               */
             void serialEvent() {
               while (Serial.available()) {
                 // get the new byte:
                 
// Serial.print("GETTING NEW BYTE\n");  

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
             
                           /*
               SerialEvent occurs whenever a new data comes in the\
               hardware serial RX.  This routine is run between each
               time loop() runs, so using delay inside loop can delay
               response.  Multiple bytes of data may be available.
               */
/*               void serialEvent2() {
               while (Serial2.available()) {
               // get the new byte:
               char inChar2 = (char)Serial2.read();
               // add it to the inputString:
               Serial.print(inChar2);
                }
             }
*/             
