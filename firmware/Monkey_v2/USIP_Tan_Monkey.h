
  /* USIP Tan Monkey header file. 
     This file contains the limits for the Tan Monkey Servos
  */
 #ifndef _USIP_TANMNKY_H
 #define _USIP_TANMNKY_H
/////////// SOFTWARE VERSION NUMBER ///////////////////////////
// Ver 0.1M, 07/22/2023
// USIP Copyright 2023 University of Bridgeport, NASA, CT Space Grant Consortium.
// All rights reserved.
///////////////////////////////////////////////////////////////
#define VERSION "0.1TANMNKY"
#define RT_ELBW_GO_HOME_PWM 200
#define RT_SHLDR_GO_HOME_PWM 275
#define LFT_SHLDR_GO_HOME_PWM 400


///////////////// HOME POSITIONS /////////////////////////////////
int botHead0 = 310;
int topHead1 = 370;
int rightShoulder2 = 295;
int rightElbow3 = 400;
int leftShoulder4 = 460;
int leftElbow5 = 170;

int elbowRange = 120;

int xv;
float wiper_read = 0.0;
uint8_t wiper_value = 0;
int i=0;
/////////////////////////////////////////////////////////////


#endif  /// _USIP_TANMNKY_H
