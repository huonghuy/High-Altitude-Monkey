
  /* USIP_LCD header file. Tbis file supports the Parallax Liquid 
   serial LCD display 
  */
 #ifndef _USIP_LCD_H
 #define _USIP_LCD_H
/////////// SOFTWARE VERSION NUMBER ///////////////////////////
// Ver 0.1M, 8/11/19
// Ver 0.2M, 9/27/19 Added version number   
// USIP Copyright 2019 University of Bridgeport, Discovery Museum Bridgeport, CT, NASA, CT Space Grant Consortium.
// All rights reserved.
///////////////////////////////////////////////////////////////
#define VERSION "0.2LCD"

 ///////////// COMMANDS
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

////////////////////// CODE
// this routine clears the display and leaves cursor at line 0 position 0
void LCD_Clear()
{
   Serial3.write(LCD_MOVECURL0P0);    // clear display line 0
   Serial3.print("                                "); 
//   Serial3.write(LCD_MOVECURL1P0);    // clear display line 1
 //  Serial3.print("                ");
   Serial3.write(LCD_MOVECURL0P0);
}

#endif  /// _USIP_LCD_H
