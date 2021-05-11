/*
  UNDER CONSTRUCTION...

  ATTENTION: You migh need to calibrate the touch. 

  ABOUT SSB PATCH:  
 
  First of all, it is important to say that the SSB patch content is not part of this library. The paches used here were made available by Mr. 
  Vadim Afonkin on his Dropbox repository. It is important to note that the author of this library does not encourage anyone to use the SSB patches 
  content for commercial purposes. In other words, this library only supports SSB patches, the patches themselves are not part of this library.

  In this context, a patch is a piece of software used to change the behavior of the SI4735 device.
  There is little information available about patching the SI4735 or Si4732 devices. The following information is the understanding of the author of
  this project and it is not necessarily correct. A patch is executed internally (run by internal MCU) of the device.
  Usually, patches are used to fixes bugs or add improvements and new features of the firmware installed in the internal ROM of the device.
  Patches to the SI473X are distributed in binary form and have to be transferred to the internal RAM of the device by
  the host MCU (in this case Arduino). Since the RAM is volatile memory, the patch stored into the device gets lost when you turn off the system.
  Consequently, the content of the patch has to be transferred again to the device each time after turn on the system or reset the device.


  Prototype documentation: https://pu2clr.github.io/SI4735/
  PU2CLR Si47XX API documentation: https://pu2clr.github.io/SI4735/extras/apidoc/html/

  By Ricardo Lima Caratti, May 2021
  
*/

#include <SI4735.h>
#include <SPI.h>
#include <TFT_eSPI.h>      // Hardware-specific library

#include "DSEG7_Classic_Mini_Regular_48.h"
#include "Rotary.h"

// #include "patch_init.h" // SSB patch for whole SSBRX initialization string
// #include "patch_init.h"  // SSB patch full - It is not clear. No difference found if compared with patch_init
#include "patch_3rd.h" // 3rd patch. Taken from DEGEN DE1103 receiver according to the source.

const uint16_t size_content = sizeof ssb_patch_content; // see ssb_patch_content in patch_full.h or patch_init.h

#define DISPLAY_LED 14   // Pin used to control to turn the display on or off
#define DISPLAY_ON   0   
#define DISPLAY_OFF  1

#define MINPRESSURE 200
#define MAXPRESSURE 1000

#define RESET_PIN 12           // Mega2560 digital Pin used to RESET
#define ENCODER_PUSH_BUTTON 33 // Used to switch BFO and VFO or other function
#define AUDIO_MUTE_CIRCUIT 27  // If you have an external mute circuit, use this pin to connect it.

// Enconder PINs (interrupt pins used on DUE. All Digital DUE Pins can be used as interrupt)
#define ENCODER_PIN_A 17
#define ENCODER_PIN_B 16

#define AM_FUNCTION 1
#define FM_FUNCTION 0

#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3

#define MIN_ELAPSED_TIME 250
#define MIN_ELAPSED_RSSI_TIME 200
#define ELAPSED_COMMAND 2000 // time to turn off the last command
#define DEFAULT_VOLUME 45    // change it for your favorite sound volume

#define FM 0
#define LSB 1
#define USB 2
#define AM 3
#define LW 4
#define SSB 1

#define RSSI_DISPLAY_COL_OFFSET 35
#define RSSI_DISPLAY_LIN_OFFSET 90

#define KEYBOARD_LIN_OFFSET 50
#define STATUS_DISPLAY_COL_OFFSET 5
#define STATUS_DISPLAY_LIN_OFFSET 430

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

bool cmdBFO = false;
bool cmdAudioMute = false;
bool cmdSlop = false;
bool cmdMuteRate = false;
bool cmdVolume = false; // if true, the encoder will control the volume.
bool cmdAgcAtt = false;
bool cmdFilter = false;
bool cmdStep = false;
bool cmdBand = false;
bool cmdSoftMuteMaxAtt = false;

bool cmdSync = false;

bool ssbLoaded = false;
bool fmStereo = true;
bool touch = false;

// AGC and attenuation control
int8_t agcIdx = 0;
uint8_t disableAgc = 0;
int8_t agcNdx = 0;
uint16_t antennaIdx = 0;
int8_t softMuteMaxAttIdx = 16;
int16_t slopIdx = 1;
int16_t muteRateIdx = 64;

int currentBFO = 0;
int previousBFO = 0;

long elapsedRSSI = millis();
long elapsedButton = millis();
long elapsedFrequency = millis();

long elapsedCommand = millis();

uint8_t rssi = 0;

// Encoder control variables
volatile int encoderCount = 0;

typedef struct
{
  const char *bandName;
  uint8_t bandType;     // Band type (FM, MW or SW)
  uint16_t minimumFreq; // Minimum frequency of the band
  uint16_t maximumFreq; // maximum frequency of the band
  uint16_t currentFreq; // Default frequency or current frequency
  uint16_t currentStep; // Defeult step (increment and decrement)
} Band;

/*
   Band table
*/
Band band[] = {
    {"FM  ", FM_BAND_TYPE, 8400, 10800, 10390, 10},
    {"LW  ", LW_BAND_TYPE, 100, 510, 300, 1},
    {"AM  ", MW_BAND_TYPE, 520, 1720, 810, 10},
    {"160m", SW_BAND_TYPE, 1800, 3500, 1900, 1}, // 160 meters
    {"80m ", SW_BAND_TYPE, 3500, 4500, 3700, 1}, // 80 meters
    {"60m ", SW_BAND_TYPE, 4500, 5500, 4885, 5},
    {"49m ", SW_BAND_TYPE, 5600, 6300, 6100, 5},
    {"40m ", SW_BAND_TYPE, 6800, 7200, 7100, 1}, // 40 meters
    {"41m ", SW_BAND_TYPE, 7200, 7900, 7205, 5}, // 41 meters
    {"31m ", SW_BAND_TYPE, 9200, 10000, 9600, 5},
    {"30m ", SW_BAND_TYPE, 10000, 11000, 10100, 1}, // 30 meters
    {"25m ", SW_BAND_TYPE, 11200, 12500, 11940, 5},
    {"22m ", SW_BAND_TYPE, 13400, 13900, 13600, 5},
    {"20m ", SW_BAND_TYPE, 14000, 14500, 14200, 1}, // 20 meters
    {"19m ", SW_BAND_TYPE, 15000, 15900, 15300, 5},
    {"18m ", SW_BAND_TYPE, 17200, 17900, 17600, 5},
    {"17m ", SW_BAND_TYPE, 18000, 18300, 18100, 1}, // 17 meters
    {"15m ", SW_BAND_TYPE, 21000, 21499, 21200, 1}, // 15 mters
    {"13m ", SW_BAND_TYPE, 21500, 21900, 21525, 5}, // 15 mters
    {"12m ", SW_BAND_TYPE, 24890, 26200, 24940, 1}, // 12 meters
    {"CB  ", SW_BAND_TYPE, 26200, 27900, 27500, 1}, // CB band (11 meters)
    {"10m ", SW_BAND_TYPE, 28000, 30000, 28400, 1},
    {"All ", SW_BAND_TYPE, 100, 30000, 15000, 1} // All HF in one band
};

const int lastBand = (sizeof band / sizeof(Band)) - 1;
int bandIdx = 0;
int lastSwBand = 7; // Saves the last SW band used

int tabStep[] = {1, 5, 10, 50, 100, 500, 1000};
const int lastStep = (sizeof tabStep / sizeof(int)) - 1;
int idxStep = 0;

uint16_t currentFrequency;
uint16_t previousFrequency;
uint8_t bandwidthIdx = 0;
uint16_t currentStep = 1;
uint8_t currentBFOStep = 25;

// Datatype to deal with bandwidth on AM and SSB in numerical order.
typedef struct
{
  uint8_t idx;      // SI473X device bandwitdth index value
  const char *desc; // bandwitdth description
} Bandwitdth;

int8_t bwIdxSSB = 4;
Bandwitdth bandwitdthSSB[] = {{4, "0.5"},  //  4 = 0.5kHz
                              {5, "1.0"},  //
                              {0, "1.2"},  //
                              {1, "2.2"},  //
                              {2, "3.0"},  //
                              {3, "4.0"}}; // 3 = 4kHz

int8_t bwIdxAM = 4;
const int maxFilterAM = 15;
Bandwitdth bandwitdthAM[] = {{4, "1.0"}, // 4 = 1kHz
                             {5, "1.8"},
                             {3, "2.0"},
                             {6, "2.5"},
                             {2, "3.0"},
                             {1, "4.0"},
                             {0, "6.0"}, // 0 = 6kHz
                             {7, "U07"}, // 7–15 = Reserved (Do not use) says the manual.
                             {8, "U08"},
                             {9, "U09"},
                             {10, "U10"},
                             {11, "U11"},
                             {12, "U12"},
                             {13, "U13"},
                             {14, "U14"},
                             {15, "U15"}};

int8_t bwIdxFM = 0;
Bandwitdth bandwitdthFM[] = {{0, "AUT"}, // Automatic
                             {1, "110"}, // Force wide (110 kHz) channel filter.
                             {2, " 84"},
                             {3, " 60"},
                             {4, " 40"}};

const char *bandModeDesc[] = {"FM ", "LSB", "USB", "AM "};
uint8_t currentMode = FM;

char buffer[255];
char bufferFreq[10];
char bufferStereo[10];
char bufferBFO[15];
char bufferUnit[10];

char bufferMode[15];
char bufferBandName[15];

Rotary encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);
TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
SI4735 si4735;

TFT_eSPI_Button buttonNextBand, 
                buttonPreviousBand, 
                buttonVolumeLevel, 
                buttonSeekUp, 
                buttonSeekDown, 
                buttonStep, 
                buttonAudioMute, 
                buttonAM, 
                buttonLSB, 
                buttonUSB, 
                buttonFM, 
                buttonMW, 
                buttonSW, 
                buttonFilter, 
                buttonAGC, 
                buttonSoftMute, 
                buttonSlop, 
                buttonSMuteRate, 
                buttonBFO, 
                buttonSync;

uint16_t pixel_x, pixel_y; //Touch_getXY() updates global vars
bool Touch_getXY(void)
{
  bool pressed = tft.getTouch(&pixel_x, &pixel_y);
  return pressed;
}

void showBandwitdth(bool drawAfter = false);
void showAgcAtt(bool drawAfter = false);
void showStep(bool drawAfter = false);
void showSoftMute(bool drawAfter = false);
void showMuteRate(bool drawAfter = false);
void showSlop(bool drawAfter = false);
void showVolume(bool drawAfter = false);



void setup(void)
{

  // if you are using the Gerts or Thiago project, the two lines below turn on the display 
  pinMode(DISPLAY_LED, OUTPUT);
  digitalWrite(DISPLAY_LED, DISPLAY_ON);

  // Encoder pins
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_PUSH_BUTTON, INPUT_PULLUP);

  si4735.setAudioMuteMcuPin(AUDIO_MUTE_CIRCUIT);

  // Initialise the TFT screen
  tft.init();
  
  // tft.setRotation(0); //PORTRAIT
  tft.setRotation(2);

  // Calibration code for touchscreen : for 2.8 inch & Rotation = 2
  uint16_t calData[5] = {258, 3566, 413, 3512, 2};
  tft.setTouch(calData);

  tft.fillScreen(BLACK);

  // tft.setFreeFont(&FreeSans12pt7b);
  showText(80, 30, 3, NULL, GREEN, "SI4735");
  showText(80, 90, 3, NULL, YELLOW, "Arduino");
  showText(80, 160, 3, NULL, YELLOW, "Library");
  showText(10, 240, 3, NULL, WHITE, "PU2CLR / RICARDO");
  showText(50, 340, 1, NULL, WHITE, "https://pu2clr.github.io/SI4735/");
  int16_t si4735Addr = si4735.getDeviceI2CAddress(RESET_PIN);
  if (si4735Addr == 0)
  {
    tft.fillScreen(BLACK);
    showText(0, 160, 2, NULL, RED, "Si473X not");
    showText(0, 240, 2, NULL, RED, "detected!!");
    while (1)
      ;
  }
  else
  {
    sprintf(buffer, "The Si473X I2C address is 0x%x ", si4735Addr);
    showText(65, 440, 1, NULL, RED, buffer);
  }
  delay(5000);

  tft.fillScreen(BLACK);

  // Atach Encoder pins interrupt
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);

  si4735.setup(RESET_PIN, 1);
  // si4735.setup(RESET_PIN, 0, POWER_UP_FM, SI473X_ANALOG_AUDIO, XOSCEN_CRYSTAL);

  // Set up the radio for the current band (see index table variable bandIdx )
  delay(100);

  showTemplate();
  
  useBand();
  currentFrequency = previousFrequency = si4735.getFrequency();
  si4735.setVolume(DEFAULT_VOLUME);
  tft.setFreeFont(NULL); // default font
  showStatus();
}

/*
   Use Rotary.h and  Rotary.cpp implementation to process encoder via interrupt

*/
void rotaryEncoder()
{ // rotary encoder events
  uint8_t encoderStatus = encoder.process();
  if (encoderStatus)
  {
    if (encoderStatus == DIR_CW)
    {
      encoderCount = 1;
    }
    else
    {
      encoderCount = -1;
    }
  }
}



/**
 * Disable all commands
 * 
 */
void disableCommands()
{

  // Redraw if necessary
  if (cmdVolume) 
    buttonVolumeLevel.drawButton(true);
  if (cmdStep)
    buttonStep.drawButton(true);
  if (cmdFilter)
    buttonFilter.drawButton(true);
  if (cmdAgcAtt)
    buttonAGC.drawButton(true);
  if (cmdSoftMuteMaxAtt)
    buttonSoftMute.drawButton(true);
  if (cmdSlop)
    buttonSlop.drawButton(true);
  if (cmdMuteRate)
    buttonSMuteRate.drawButton(true);

  cmdBFO = false;
  cmdAudioMute = false;
  cmdSlop = false;
  cmdMuteRate = false;
  cmdVolume = false; // if true, the encoder will control the volume.
  cmdAgcAtt = false;
  cmdFilter = false;
  cmdStep = false;
  cmdBand = false;
  cmdSoftMuteMaxAtt = false;
}

/*
   Shows a text on a given position; with a given size and font, and with a given color

   @param int x column
   @param int y line
   @param int sz font size
   @param const GFXfont *f font type
   @param uint16_t color
   @param char * msg message
*/
void showText(int x, int y, int sz, const GFXfont *f, uint16_t color, const char *msg)
{
  tft.setFreeFont(f);
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(sz);
  tft.print(msg);
}

/*
    Prevents blinking during the frequency display.
    Erases the old char/digit value if it has changed and print the new one.
*/
void printText(int col, int line, int sizeText, char *oldValue, const char *newValue, uint16_t color, uint8_t space)
{
  int c = col;
  char *pOld;
  char *pNew;

  pOld = oldValue;
  pNew = (char *)newValue;

  tft.setTextSize(sizeText);

  // prints just changed digits
  while (*pOld && *pNew)
  {
    if (*pOld != *pNew)
    {
      tft.setTextColor(BLACK);
      tft.setCursor(c, line);
      tft.print(*pOld);
      tft.setTextColor(color);
      tft.setCursor(c, line);
      tft.print(*pNew);
    }
    pOld++;
    pNew++;
    c += space;
  }

  // Is there anything else to erase?
  tft.setTextColor(BLACK);
  while (*pOld)
  {
    tft.setCursor(c, line);
    tft.print(*pOld);
    pOld++;
    c += space;
  }

  // Is there anything else to print?
  tft.setTextColor(color);
  while (*pNew)
  {
    tft.setCursor(c, line);
    tft.print(*pNew);
    pNew++;
    c += space;
  }

  // Save the current content to be tested next time
  strcpy(oldValue, newValue);
}


void setDrawButtons( bool value) {
  buttonNextBand.drawButton(value);
  buttonPreviousBand.drawButton(value);
  buttonVolumeLevel.drawButton(value);
  buttonAudioMute.drawButton(value);
  buttonSeekUp.drawButton(value);
  buttonSeekDown.drawButton(value);
  buttonStep.drawButton(value);
  buttonBFO.drawButton(value);
  buttonFM.drawButton(value);
  buttonMW.drawButton(value);
  buttonSW.drawButton(value);
  buttonAM.drawButton(value);
  buttonLSB.drawButton(value);
  buttonUSB.drawButton(value);
  buttonFilter.drawButton(value);
  buttonAGC.drawButton(value);
  buttonSoftMute.drawButton(value);
  buttonSlop.drawButton(value);
  buttonSMuteRate.drawButton(value);
  buttonSync.drawButton(value);
}

/**
 * Initiates an instance of a given button
 * 
 */
void setButton(TFT_eSPI_Button *button, int16_t col, int16_t lin, int16_t width, int16_t high, char *label, bool drawAfter)
{
  tft.setFreeFont(NULL);
  button->initButton(&tft, col, lin, width, high, WHITE, CYAN, BLACK, (char *)label, 1);
  button->drawButton(drawAfter);
}


void setButtonsFM() {
  setButton(&buttonFilter, 270, KEYBOARD_LIN_OFFSET + 295, 70, 49, (char *) "BW", true);
  setButton(&buttonSoftMute, 45, KEYBOARD_LIN_OFFSET + 350, 70, 49, (char *) "---", true);
  setButton(&buttonSMuteRate, 120, KEYBOARD_LIN_OFFSET + 350, 70, 49, (char *) "---", true);
  setButton(&buttonSlop, 195, KEYBOARD_LIN_OFFSET + 350, 70, 49, (char *) "---", true);
  setButton(&buttonAGC, 270, KEYBOARD_LIN_OFFSET + 240, 70, 49, (char *) "AGC On", true);
}

void showTemplate()
{
  int w = tft.width();
  // Area used to show the frequency
  tft.drawRect(0, 0, w, 75, WHITE);
  tft.drawRect(0, KEYBOARD_LIN_OFFSET + 100, w, 280, CYAN);
  tft.setFreeFont(NULL);

  setButton(&buttonPreviousBand, 45, KEYBOARD_LIN_OFFSET + 130, 70, 49, (char *) "Band-", true);
  setButton(&buttonNextBand, 120, KEYBOARD_LIN_OFFSET + 130, 70, 49, (char *) "Band+", true);
  setButton(&buttonVolumeLevel, 195, KEYBOARD_LIN_OFFSET + 130, 70, 49, (char *) "Vol", true);
  setButton(&buttonAudioMute, 270, KEYBOARD_LIN_OFFSET + 130, 70, 49, (char *) "Mute", true);
  setButton(&buttonSeekDown, 45, KEYBOARD_LIN_OFFSET + 185, 70, 49, (char *) "Seek-", true);
  setButton(&buttonSeekUp, 120, KEYBOARD_LIN_OFFSET + 185, 70, 49, (char *) "Seek+", true);
  setButton(&buttonBFO, 195, KEYBOARD_LIN_OFFSET + 185, 70, 49, (char *) "BFO", true);
  setButton(&buttonStep, 270, KEYBOARD_LIN_OFFSET + 185, 70, 49, (char *) "Step", true);
  setButton(&buttonFM, 45, KEYBOARD_LIN_OFFSET + 240, 70, 49, (char *) "FM", true);
  setButton(&buttonMW, 120, KEYBOARD_LIN_OFFSET + 240, 70, 49, (char *) "MW", true);
  setButton(&buttonSW, 195, KEYBOARD_LIN_OFFSET + 240, 70, 49, (char *) "SW", true);
  setButton(&buttonAGC, 270, KEYBOARD_LIN_OFFSET + 240, 70, 49, (char *) "AGC On", true);
  setButton(&buttonAM, 45, KEYBOARD_LIN_OFFSET + 295, 70, 49, (char *) "AM", true);
  setButton(&buttonLSB, 120, KEYBOARD_LIN_OFFSET + 295, 70, 49, (char *) "LSB", true);
  setButton(&buttonUSB, 195, KEYBOARD_LIN_OFFSET + 295, 70, 49, (char *) "USB", true);
  setButton(&buttonFilter, 270, KEYBOARD_LIN_OFFSET + 295, 70, 49, (char *) "BW", true);
  setButton(&buttonSoftMute, 45, KEYBOARD_LIN_OFFSET + 350, 70, 49, (char *) "---", true);
  setButton(&buttonSMuteRate, 120, KEYBOARD_LIN_OFFSET + 350, 70, 49, (char *) "---", true);
  setButton(&buttonSlop, 195, KEYBOARD_LIN_OFFSET + 350, 70, 49, (char *) "---", true);
  setButton(&buttonSync, 270, KEYBOARD_LIN_OFFSET + 350, 70, 49, (char *) "SYNC", true);

  // Exibe os botões (teclado touch)
  setDrawButtons(true);
  showText(0, 470, 1, NULL, YELLOW, "PU2CLR SI4735 Arduino Library - You can do it better!");
  tft.setFreeFont(NULL);
}

/*
    Prevents blinking during the frequency display.
    Erases the old digits if it has changed and print the new digit values.
*/
void showFrequencyValue(int col, int line, char *oldValue, char *newValue, uint16_t color, uint8_t space, uint8_t textSize)
{
  int c = col;
  char *pOld;
  char *pNew;

  pOld = oldValue;
  pNew = newValue;
  // prints just changed digits
  while (*pOld && *pNew)
  {
    if (*pOld != *pNew)
    {
      tft.drawChar(c, line, *pOld, BLACK, BLACK, textSize);
      tft.drawChar(c, line, *pNew, color, BLACK, textSize);
    }
    pOld++;
    pNew++;
    c += space;
  }
  // Is there anything else to erase?
  while (*pOld)
  {
    tft.drawChar(c, line, *pOld, BLACK, BLACK, textSize);
    pOld++;
    c += space;
  }
  // Is there anything else to print?
  while (*pNew)
  {
    tft.drawChar(c, line, *pNew, color, BLACK, textSize);
    pNew++;
    c += space;
  }
  strcpy(oldValue, newValue);
}

/**
 * Shows the current frequency
 * 
 */
void showFrequency()
{
  uint16_t color;

  char aux[15];
  char sFreq[15];

  tft.setFreeFont(&DSEG7_Classic_Mini_Regular_48);
  tft.setTextSize(1);
  if (si4735.isCurrentTuneFM())
  {
    sprintf(aux, "%5.5d", currentFrequency);
    sFreq[0] = (aux[0] == '0') ? ' ' : aux[0];
    sFreq[1] = aux[1];
    sFreq[2] = aux[2];
    sFreq[3] = aux[3];
    sFreq[4] = '\0';

    tft.drawChar(180, 55, '.', YELLOW, BLACK, 1);
  }
  else
  {
    sprintf(sFreq, "%5d", currentFrequency);
    tft.drawChar(180, 55, '.', BLACK, BLACK, 1);
  }

  color = (cmdBFO) ? CYAN : YELLOW;

  showFrequencyValue(45, 58, bufferFreq, sFreq, color, 45, 1);

  if (currentMode == LSB || currentMode == USB)
  {
    showBFO();
  }
  tft.setFreeFont(NULL); // default font
}

/**
 * Shows the frequency during seek process
 * 
 */
void showFrequencySeek(uint16_t freq)
{
  previousFrequency = currentFrequency = freq;
  showFrequency();
}


/**
 * Checks the stop seeking criterias.  
 * Returns true if the user press the touch or rotates the encoder. 
 */
bool checkStopSeeking() {
  // Checks the touch and encoder
  return (bool) encoderCount || Touch_getXY(); // returns true if the user rotates the encoder or touches on screen
} 


/**
 * Clears status area
 * 
 */
void clearStatusArea()
{
  tft.fillRect(STATUS_DISPLAY_COL_OFFSET, STATUS_DISPLAY_LIN_OFFSET, tft.width() -8, 36, BLACK); // Clear all status area
}

/**
 * Clears frequency area
 * 
 */
void clearFrequencyArea()
{
  tft.fillRect(2, 2, tft.width() - 4, 70, BLACK);
}

int getStepIndex(int st)
{
  for (int i = 0; i < lastStep; i++)
  {
    if (st == tabStep[i])
      return i;
  }
  return 0;
}

/**
 * Clears buffer control variable
 * 
 */
void clearBuffer()
{
  bufferMode[0] = '\0';
  bufferBandName[0] = '\0';
  bufferMode[0] = '\0';
  bufferBFO[0] = '\0';
  bufferFreq[0] = '\0';
  bufferUnit[0] = '\0';
}

/**
 * Soows status
 * 
 */
void showStatus()
{
  clearBuffer();
  clearFrequencyArea();
  clearStatusArea();

  DrawSmeter();

  si4735.getStatus();
  si4735.getCurrentReceivedSignalQuality();

  si4735.getFrequency();
  showFrequency();

  showVolume(true);
  tft.setFreeFont(NULL); // default font
  printText(5, 5, 2, bufferBandName, band[bandIdx].bandName, CYAN, 11);

  if (band[bandIdx].bandType == SW_BAND_TYPE)
  {
    printText(5, 30, 2, bufferMode, bandModeDesc[currentMode], CYAN, 11);
  }
  else
  {
    printText(5, 30, 2, bufferMode, "    ", BLACK, 11);
  }

  if (si4735.isCurrentTuneFM())
  {
    printText(280, 55, 2, bufferUnit, "MHz", WHITE, 12);
    setButtonsFM();
    // setDrawButtons(true);
    return;
  }

  printText(280, 55, 2, bufferUnit, "kHz", WHITE, 12);

  showBandwitdth(true);
  showAgcAtt(true);
  showStep(true);
  showSoftMute(true);
  showSlop(true);
  showMuteRate(true);
  // setDrawButtons(true);
 }

/**
 * SHow bandwitdth on AM or SSB mode
 * 
 */
void showBandwitdth(bool drawAfter)
{
  char bw[20];
    
  if (currentMode == LSB || currentMode == USB)
  {
    sprintf(bw, "BW:%s", bandwitdthSSB[bwIdxSSB].desc);
    showBFO();
  }
  else if (currentMode == AM) {
    sprintf(bw, "BW:%s", bandwitdthAM[bwIdxAM].desc);
  }
  else {
    sprintf(bw, "BW:%s", bandwitdthFM[bwIdxFM].desc);
  }
  setButton(&buttonFilter, 270, KEYBOARD_LIN_OFFSET + 295, 70, 49, bw, drawAfter);
}

/**
 * Shows AGC and Attenuation status
 * 
 */
void showAgcAtt(bool drawAfter)
{
  char sAgc[15];

  if (currentMode == FM) return;

  si4735.getAutomaticGainControl();
  if (agcNdx == 0 && agcIdx == 0)
    strcpy(sAgc, "AGC ON");
  else
  {
    sprintf(sAgc, "ATT: %2d", agcNdx);
  }
  setButton(&buttonAGC, 270, KEYBOARD_LIN_OFFSET +  240, 70, 49, sAgc, drawAfter);
}

/**
 * Draws the Smeter 
 * This function is based on Mr. Gert Baak's sketch available on his github repository 
 * See: https://github.com/pe0mgb/SI4735-Radio-ESP32-Touchscreen-Arduino 
 */
void DrawSmeter()
{
  tft.setTextSize(1);
  tft.setTextColor(WHITE, BLACK);
  for (int i = 0; i < 10; i++)
  {
    tft.fillRect(RSSI_DISPLAY_COL_OFFSET + 15 + (i * 12), RSSI_DISPLAY_LIN_OFFSET + 24, 4, 8, YELLOW);
    tft.setCursor((RSSI_DISPLAY_COL_OFFSET + 14 + (i * 12)), RSSI_DISPLAY_LIN_OFFSET + 13);
    tft.print(i);
  }
  for (int i = 1; i < 7; i++)
  {
    tft.fillRect((RSSI_DISPLAY_COL_OFFSET + 123 + (i * 16)), RSSI_DISPLAY_LIN_OFFSET + 24, 4, 8, RED);
    tft.setCursor((RSSI_DISPLAY_COL_OFFSET + 117 + (i * 16)), RSSI_DISPLAY_LIN_OFFSET + 13);
    if ((i == 2) or (i == 4) or (i == 6))
    {
      tft.print("+");
      tft.print(i * 10);
    }
  }
  tft.fillRect(RSSI_DISPLAY_COL_OFFSET + 15, RSSI_DISPLAY_LIN_OFFSET + 32, 112, 4, YELLOW);
  tft.fillRect(RSSI_DISPLAY_COL_OFFSET + 127, RSSI_DISPLAY_LIN_OFFSET + 32, 100, 4, RED);
}

/**
 * Shows RSSI level
 * This function shows the RSSI level based on the function DrawSmeter above. 
 */
void showRSSI()
{
  int spoint;
  if (currentMode != FM)
  {
    //dBuV to S point conversion HF
    if (rssi <= 1 )
      spoint = 12; // S0
    if (rssi <= 2)
      spoint = 24; // S1
    if (rssi <= 3 )
      spoint = 36; // S2
    if (rssi <= 4)
      spoint = 48; // S3
    if ((rssi > 4) and (rssi <= 10))
      spoint = 48 + (rssi - 4) * 2; // S4
    if ((rssi > 10) and (rssi <= 16))
      spoint = 60 + (rssi - 10) * 2; // S5
    if ((rssi > 16) and (rssi <= 22))
      spoint = 72 + (rssi - 16) * 2; // S6
    if ((rssi > 22) and (rssi <= 28))
      spoint = 84 + (rssi - 22) * 2; // S7
    if ((rssi > 28) and (rssi <= 34))
      spoint = 96 + (rssi - 28) * 2; // S8
    if ((rssi > 34) and (rssi <= 44))
      spoint = 108 + (rssi - 34) * 2; // S9
    if ((rssi > 44) and (rssi <= 54))
      spoint = 124 + (rssi - 44) * 2; // S9 +10
    if ((rssi > 54) and (rssi <= 64))
      spoint = 140 + (rssi - 54) * 2; // S9 +20
    if ((rssi > 64) and (rssi <= 74))
      spoint = 156 + (rssi - 64) * 2; // S9 +30
    if ((rssi > 74) and (rssi <= 84))
      spoint = 172 + (rssi - 74) * 2; // S9 +40
    if ((rssi > 84) and (rssi <= 94))
      spoint = 188 + (rssi - 84) * 2; // S9 +50
    if (rssi > 94)
      spoint = 204; // S9 +60
    if (rssi > 95)
      spoint = 208; //>S9 +60
  }
  else
  {
    //dBuV to S point conversion FM
    if (rssi < 1)
      spoint = 36;
    if ((rssi > 1) and (rssi <= 2))
      spoint = 60; // S6
    if ((rssi > 2) and (rssi <= 8))
      spoint = 84 + (rssi - 2) * 2; // S7
    if ((rssi > 8) and (rssi <= 14))
      spoint = 96 + (rssi - 8) * 2; // S8
    if ((rssi > 14) and (rssi <= 24))
      spoint = 108 + (rssi - 14) * 2; // S9
    if ((rssi > 24) and (rssi <= 34))
      spoint = 124 + (rssi - 24) * 2; // S9 +10
    if ((rssi > 34) and (rssi <= 44))
      spoint = 140 + (rssi - 34) * 2; // S9 +20
    if ((rssi > 44) and (rssi <= 54))
      spoint = 156 + (rssi - 44) * 2; // S9 +30
    if ((rssi > 54) and (rssi <= 64))
      spoint = 172 + (rssi - 54) * 2; // S9 +40
    if ((rssi > 64) and (rssi <= 74))
      spoint = 188 + (rssi - 64) * 2; // S9 +50
    if (rssi > 74)
      spoint = 204; // S9 +60
    if (rssi > 76)
      spoint = 208; //>S9 +60
  }
  tft.fillRect(RSSI_DISPLAY_COL_OFFSET + 15, RSSI_DISPLAY_LIN_OFFSET + 38, (2 + spoint), 6, RED);
  tft.fillRect(RSSI_DISPLAY_COL_OFFSET + 17 + spoint, RSSI_DISPLAY_LIN_OFFSET + 38, 212 - (2 + spoint), 6, GREEN);
}

void showStep(bool drawAfter)
{
  char sStep[15];
  sprintf(sStep, "Stp:%4d", currentStep);
  setButton(&buttonStep, 270, KEYBOARD_LIN_OFFSET +  185, 70, 49, sStep, drawAfter);
}

void showSoftMute(bool drawAfter)
{
  char sMute[15];

  if (currentMode == FM) return;
  
  sprintf(sMute, "SM: %2d", softMuteMaxAttIdx);
  setButton(&buttonSoftMute, 45, KEYBOARD_LIN_OFFSET + 350, 70, 49, sMute, drawAfter);
}

/**
 * Shows the BFO offset on frequency area
 */
void showBFO()
{
  tft.setFreeFont(NULL); // default font
  sprintf(buffer, "%c%d", (currentBFO >= 0) ? '+' : '-', abs(currentBFO));
  printText(262, 6, 2, bufferBFO, buffer, YELLOW, 11);
}

/**
 * Shows the current volume level
 */
void showVolume(bool drawAfter)
{
  char sVolume[15];
  sprintf(sVolume, "Vol: %2.2d", si4735.getVolume());
  setButton(&buttonVolumeLevel, 195, KEYBOARD_LIN_OFFSET + 130, 70, 49, sVolume, drawAfter);
}

/**
 * Shows slop parameter
 */
void showSlop(bool drawAfter)
{
  char sSlop[10];

  if (currentMode == FM) return; 
    
  sprintf(sSlop, "Sl:%2.2u", slopIdx);
  setButton(&buttonSlop, 195, KEYBOARD_LIN_OFFSET + 350, 70, 49, sSlop, drawAfter);
}

void showMuteRate(bool drawAfter)
{
  char sMRate[10];

  if (currentMode == FM) return; 
    
  sprintf(sMRate, "MR:%3.3u", muteRateIdx);
  setButton(&buttonSMuteRate, 120, KEYBOARD_LIN_OFFSET + 350, 70, 49, sMRate, drawAfter);
}

char *rdsMsg;
char *stationName;
char *rdsTime;
char bufferStatioName[255];
char bufferRdsMsg[255];
char bufferRdsTime[32];

void showRDSMsg()
{
  if (strcmp(bufferRdsMsg, rdsMsg) == 0)
    return;
  printText(STATUS_DISPLAY_COL_OFFSET + 5, STATUS_DISPLAY_LIN_OFFSET + 20, 1, bufferRdsMsg, rdsMsg, GREEN, 6);
  delay(100);
}

void showRDSStation()
{
  if (strcmp(bufferStatioName, stationName) == 0)
    return;
  printText(STATUS_DISPLAY_COL_OFFSET + 5, STATUS_DISPLAY_LIN_OFFSET + 5, 1, bufferStatioName, stationName, GREEN, 6);
  delay(250);
}

void showRDSTime()
{
  if (strcmp(bufferRdsTime, rdsTime) == 0)
    return;
  printText(STATUS_DISPLAY_COL_OFFSET + 150, STATUS_DISPLAY_LIN_OFFSET + 5, 1, bufferRdsTime, rdsTime, GREEN, 6);
  delay(100);
}

void checkRDS()
{

  si4735.getRdsStatus();
  if (si4735.getRdsReceived())
  {
    if (si4735.getRdsSync() && si4735.getRdsSyncFound())
    {
      rdsMsg = si4735.getRdsText2A();
      stationName = si4735.getRdsText0A();
      rdsTime = si4735.getRdsTime();
      if (rdsMsg != NULL)
        showRDSMsg();
      if (stationName != NULL)
        showRDSStation();
      if (rdsTime != NULL)
        showRDSTime();
    }
  }
}

/*
   Goes to the next band (see Band table)
*/
void bandUp()
{
  // save the current frequency for the band
  band[bandIdx].currentFreq = currentFrequency;
  band[bandIdx].currentStep = currentStep;

  if (bandIdx < lastBand)
  {
    bandIdx++;
  }
  else
  {
    bandIdx = 0;
  }

  useBand();
}

/*
   Goes to the previous band (see Band table)
*/
void bandDown()
{
  // save the current frequency for the band
  band[bandIdx].currentFreq = currentFrequency;
  band[bandIdx].currentStep = currentStep;
  if (bandIdx > 0)
  {
    bandIdx--;
  }
  else
  {
    bandIdx = lastBand;
  }
  useBand();
}

/*
   This function loads the contents of the ssb_patch_content array into the CI (Si4735) and starts the radio on
   SSB mode.
*/
void loadSSB()
{
  // si4735.reset();
  si4735.queryLibraryId(); // It also calls power down. So it is necessary.
  si4735.patchPowerUp();
  delay(50);
  // si4735.setI2CFastMode(); // Recommended
  si4735.setI2CFastModeCustom(500000); // It is a test and may crash.
  si4735.downloadPatch(ssb_patch_content, size_content);
  si4735.setI2CStandardMode(); // goes back to default (100kHz)

  // delay(50);
  // Parameters
  // AUDIOBW - SSB Audio bandwidth; 0 = 1.2kHz (default); 1=2.2kHz; 2=3kHz; 3=4kHz; 4=500Hz; 5=1kHz;
  // SBCUTFLT SSB - side band cutoff filter for band passand low pass filter ( 0 or 1)
  // AVC_DIVIDER  - set 0 for SSB mode; set 3 for SYNC mode.
  // AVCEN - SSB Automatic Volume Control (AVC) enable; 0=disable; 1=enable (default).
  // SMUTESEL - SSB Soft-mute Based on RSSI or SNR (0 or 1).
  // DSP_AFCDIS - DSP AFC Disable or enable; 0=SYNC MODE, AFC enable; 1=SSB MODE, AFC disable.
  si4735.setSSBConfig(bandwitdthSSB[bwIdxSSB].idx, 1, 0, 0, 0, 1);
  delay(25);
  ssbLoaded = true;
}

/*
   Switch the radio to current band
*/
void useBand()
{
  if (band[bandIdx].bandType == FM_BAND_TYPE)
  {
    currentMode = FM;
    si4735.setTuneFrequencyAntennaCapacitor(0);
    si4735.setFM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
    // si4735.setFMDeEmphasis(1); // 1 = 50 μs. Used in Europe, Australia, Japan;
    si4735.setSeekFmLimits(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq);
    // Define here the best criteria to find a FM station during the seeking process
    // si4735.setSeekFmSpacing(10); // frequency spacing for FM seek (5, 10 or 20. They mean 50, 100 or 200 kHz)
    // si4735.setSeekAmRssiThreshold(0);
    // si4735.setSeekFmSrnThreshold(3);

    cmdBFO = ssbLoaded = false;
    si4735.setRdsConfig(1, 2, 2, 2, 2);
  }
  else
  {
    if (band[bandIdx].bandType == MW_BAND_TYPE || band[bandIdx].bandType == LW_BAND_TYPE)
    {
      antennaIdx = 0;
      si4735.setTuneFrequencyAntennaCapacitor(antennaIdx);
    }
    else
    {
      antennaIdx = 1;
      lastSwBand = bandIdx;
      si4735.setTuneFrequencyAntennaCapacitor(antennaIdx);
    }

    if (ssbLoaded)
    {
      si4735.setSSB(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep, currentMode);
      si4735.setSSBAutomaticVolumeControl(1);
      si4735.setSsbSoftMuteMaxAttenuation(softMuteMaxAttIdx); // Disable Soft Mute for SSB
      showBFO();
    }
    else
    {
      currentMode = AM;
      si4735.setAM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
      si4735.setAmSoftMuteMaxAttenuation(softMuteMaxAttIdx); // // Disable Soft Mute for AM
      cmdBFO = false;
    }

    // Sets the seeking limits and space.
    si4735.setSeekAmLimits(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq);               // Consider the range all defined current band
    si4735.setSeekAmSpacing((band[bandIdx].currentStep > 10) ? 10 : band[bandIdx].currentStep); // Max 10kHz for spacing
  }
  delay(100);

  // Sets AGC or attenuation control
  si4735.setAutomaticGainControl(disableAgc, agcNdx);
  // si4735.setAMFrontEndAgcControl(10,12); // Try to improve sensitivity

  currentFrequency = band[bandIdx].currentFreq;
  currentStep = band[bandIdx].currentStep;
  idxStep = getStepIndex(currentStep);

  showStatus();
}

void switchAgc(int8_t v)
{

  agcIdx = (v == 1) ? agcIdx + 1 : agcIdx - 1;
  if (agcIdx < 0)
    agcIdx = 37;
  else if (agcIdx > 37)
    agcIdx = 0;

  disableAgc = (agcIdx > 0); // if true, disable AGC; esle, AGC is enable

  if (agcIdx > 1)
    agcNdx = agcIdx - 1;
  else
    agcNdx = 0;

  // Sets AGC on/off and gain
  si4735.setAutomaticGainControl(disableAgc, agcNdx);
  showAgcAtt();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}

void switchFilter(uint8_t v)
{
  if (currentMode == LSB || currentMode == USB)
  {
    bwIdxSSB = (v == 1) ? bwIdxSSB + 1 : bwIdxSSB - 1;

    if (bwIdxSSB > 5)
      bwIdxSSB = 0;
    else if (bwIdxSSB < 0)
      bwIdxSSB = 5;

    si4735.setSSBAudioBandwidth(bandwitdthSSB[bwIdxSSB].idx);
    // If audio bandwidth selected is about 2 kHz or below, it is recommended to set Sideband Cutoff Filter to 0.
    if (bandwitdthSSB[bwIdxSSB].idx == 0 || bandwitdthSSB[bwIdxSSB].idx == 4 || bandwitdthSSB[bwIdxSSB].idx == 5)
      si4735.setSBBSidebandCutoffFilter(0);
    else
      si4735.setSBBSidebandCutoffFilter(1);
  }
  else if (currentMode == AM)
  {
    bwIdxAM = (v == 1) ? bwIdxAM + 1 : bwIdxAM - 1;

    if (bwIdxAM > maxFilterAM)
      bwIdxAM = 0;
    else if (bwIdxAM < 0)
      bwIdxAM = maxFilterAM;

    si4735.setBandwidth(bandwitdthAM[bwIdxAM].idx, 1);
  } else {
    bwIdxFM = (v == 1) ? bwIdxFM + 1 : bwIdxFM - 1;
    if (bwIdxFM > 4)
      bwIdxFM = 0;
    else if (bwIdxFM < 0)
      bwIdxFM = 4;

    si4735.setFmBandwidth(bandwitdthFM[bwIdxFM].idx);
  }
  showBandwitdth();
  elapsedCommand = millis();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
}

void switchSync(int8_t v) {
  if (currentMode != FM)
  {
    currentBFO = 0;
    if (!ssbLoaded)
    {
      loadSSB();
    }
    currentMode = (currentMode == LSB)? USB:LSB;
    band[bandIdx].currentFreq = currentFrequency;
    band[bandIdx].currentStep = currentStep;
    useBand();
    si4735.setSSBDspAfc(0);
    si4735.setSSBAvcDivider(3);
  }
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}


void switchStep(int8_t v)
{

  // This command should work only for SSB mode
  if (cmdBFO && (currentMode == LSB || currentMode == USB))
  {
    currentBFOStep = (currentBFOStep == 25) ? 10 : 25;
    showBFO();
  }
  else
  {
    idxStep = (v == 1) ? idxStep + 1 : idxStep - 1;
    if (idxStep > lastStep)
      idxStep = 0;
    else if (idxStep < 0)
      idxStep = lastStep;

    currentStep = tabStep[idxStep];

    si4735.setFrequencyStep(currentStep);
    band[bandIdx].currentStep = currentStep;
    si4735.setSeekAmSpacing((currentStep > 10) ? 10 : currentStep); // Max 10kHz for spacing
    showStep();
  }
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}

void switchSoftMute(int8_t v)
{
  softMuteMaxAttIdx = (v == 1) ? softMuteMaxAttIdx + 1 : softMuteMaxAttIdx - 1;
  if (softMuteMaxAttIdx > 32)
    softMuteMaxAttIdx = 0;
  else if (softMuteMaxAttIdx < 0)
    softMuteMaxAttIdx = 32;

  si4735.setAmSoftMuteMaxAttenuation(softMuteMaxAttIdx);
  showSoftMute();
  elapsedCommand = millis();
}

/**
 * @brief Proccess the volume level
 * 
 * @param v 1 = Up; !1 = down
 */
void doVolume(int8_t v)
{
  if (v == 1)
    si4735.volumeUp();
  else
    si4735.volumeDown();
  showVolume();
  elapsedCommand = millis();
}

void doBFO()
{
  bufferFreq[0] = '\0';
  cmdBFO = !cmdBFO;
  if (cmdBFO)
  {
    buttonBFO.initButton(&tft, 195, 185, 70, 49, WHITE, CYAN, BLACK, (char *)"VFO", 1);
    showBFO();
  }
  else
  {
    buttonBFO.initButton(&tft, 195, 185, 70, 49, WHITE, CYAN, BLACK, (char *)"BFO", 1);
  }
  buttonBFO.drawButton(true);
  showStatus();
  elapsedCommand = millis();
}

void doSlop(int8_t v)
{
  slopIdx = (v == 1) ? slopIdx + 1 : slopIdx - 1;

   if (slopIdx < 1)
    slopIdx = 5;
  else if (slopIdx > 5)
    slopIdx = 1;

  si4735.setAMSoftMuteSlop(slopIdx);
  
  showSlop();
  elapsedCommand = millis();
}

void doMuteRate(int8_t v)
{
  muteRateIdx = (v == 1) ? muteRateIdx + 1 : muteRateIdx - 1;
  if (muteRateIdx < 1)
    muteRateIdx = 255;
  else if (slopIdx > 255)
    muteRateIdx = 1;

  // si4735.setAMSoftMuteRate((uint8_t)muteRateIdx);
  si4735.setAmNoiseBlank(muteRateIdx);
  
  showMuteRate();
  elapsedCommand = millis();
}

/**
 * @brief Checks the touch
 * 
 */
void checkTouch()
{

  bool down = Touch_getXY();
  buttonNextBand.press(down && buttonNextBand.contains(pixel_x, pixel_y));
  buttonPreviousBand.press(down && buttonPreviousBand.contains(pixel_x, pixel_y));
  buttonVolumeLevel.press(down && buttonVolumeLevel.contains(pixel_x, pixel_y));
  buttonBFO.press(down && buttonBFO.contains(pixel_x, pixel_y));
  buttonSeekUp.press(down && buttonSeekUp.contains(pixel_x, pixel_y));
  buttonSeekDown.press(down && buttonSeekDown.contains(pixel_x, pixel_y));
  buttonStep.press(down && buttonStep.contains(pixel_x, pixel_y));
  buttonAudioMute.press(down && buttonAudioMute.contains(pixel_x, pixel_y));
  buttonFM.press(down && buttonFM.contains(pixel_x, pixel_y));
  buttonMW.press(down && buttonMW.contains(pixel_x, pixel_y));
  buttonSW.press(down && buttonSW.contains(pixel_x, pixel_y));
  buttonAM.press(down && buttonAM.contains(pixel_x, pixel_y));
  buttonLSB.press(down && buttonLSB.contains(pixel_x, pixel_y));
  buttonUSB.press(down && buttonUSB.contains(pixel_x, pixel_y));
  buttonFilter.press(down && buttonFilter.contains(pixel_x, pixel_y));
  buttonAGC.press(down && buttonAGC.contains(pixel_x, pixel_y));
  buttonSoftMute.press(down && buttonSoftMute.contains(pixel_x, pixel_y));
  buttonSlop.press(down && buttonSlop.contains(pixel_x, pixel_y));
  buttonSMuteRate.press(down && buttonSMuteRate.contains(pixel_x, pixel_y));
  buttonSync.press(down && buttonSync.contains(pixel_x, pixel_y));
}

/* two buttons are quite simple
*/
void loop(void)
{

  if (encoderCount != 0) // Check and process the encoder
  {
    if (cmdBFO)
    {
      currentBFO = (encoderCount == 1) ? (currentBFO + currentBFOStep) : (currentBFO - currentBFOStep);
      si4735.setSSBBfo(currentBFO);
      showBFO();
      elapsedCommand = millis();
    }
    else if (cmdVolume)
      doVolume(encoderCount);
    else if (cmdAgcAtt)
      switchAgc(encoderCount);
    else if (cmdFilter)
      switchFilter(encoderCount);
    else if (cmdStep)
      switchStep(encoderCount);
    else if (cmdSoftMuteMaxAtt)
      switchSoftMute(encoderCount);
    // else if (cmdSync) 
    //   switchSync(encoderCount);  
    else if (cmdBand)
    {
      if (encoderCount == 1)
        bandUp();
      else
        bandDown();
      elapsedCommand = millis();
    }
    else if (cmdSlop)
      doSlop(encoderCount);
    else if (cmdMuteRate)
      doMuteRate(encoderCount);
    else
    {
      if (encoderCount == 1) 
        si4735.frequencyUp();
      else
        si4735.frequencyDown();
      // currentFrequency = si4735.getFrequency();      // Queries the Si473X device.
      currentFrequency = si4735.getCurrentFrequency(); // Just get the last setFrequency value (faster but can not be accurate sometimes).
      // if (loadSSB) // Checking a bug on SSB patch
      //   si4735.setAutomaticGainControl(disableAgc, agcNdx);
      showFrequency();
      elapsedCommand = millis();
    }
    encoderCount = 0;
  }
  else // Check and process touch
  {
    checkTouch();

    if (buttonNextBand.justPressed()) // Band +
      bandUp();
    else if (buttonPreviousBand.justPressed()) // Band-
      bandDown();
    else if (buttonVolumeLevel.justPressed()) // Volume
    {
      buttonVolumeLevel.drawButton(false);
      disableCommands();
      si4735.setAudioMute(cmdAudioMute);
      cmdVolume = true;
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (buttonAudioMute.justPressed()) // Mute
    {
      cmdAudioMute = !cmdAudioMute;
      si4735.setAudioMute(cmdAudioMute);
      delay(MIN_ELAPSED_TIME);
    }
    else if (buttonBFO.justPressed()) // BFO
    {
      if (currentMode == LSB || currentMode == USB)
      {
        doBFO();
      }
      delay(MIN_ELAPSED_TIME);
    }
    else if (buttonSeekUp.justPressed()) // SEEK UP
    {
      si4735.seekStationProgress(showFrequencySeek, checkStopSeeking, SEEK_UP);
      // si4735.seekNextStation(); // This method does not show the progress
      delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
      currentFrequency = si4735.getFrequency();
      showStatus();
    }
    else if (buttonSeekDown.justPressed()) // SEEK DOWN
    {
      si4735.seekStationProgress(showFrequencySeek, checkStopSeeking, SEEK_DOWN);
      // si4735.seekPreviousStation(); // This method does not show the progress
      delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
      currentFrequency = si4735.getFrequency();
      showStatus();
    }
    else if (buttonSoftMute.justPressed()) // Soft Mute
    {
      buttonSoftMute.drawButton(false);
      disableCommands();
      cmdSoftMuteMaxAtt = true;
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (buttonSlop.justPressed()) // ATU (Automatic Antenna Tuner)
    {
      buttonSlop.drawButton(false);
      disableCommands();
      cmdSlop = true;
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (buttonSMuteRate.justPressed())
    {
      buttonSMuteRate.drawButton(false);
      disableCommands();
      cmdMuteRate = true;
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (buttonAM.justPressed()) // Switch to AM mode
    {
      if (currentMode != FM)
      {
        currentMode = AM;
        ssbLoaded = false;
        cmdBFO = false;
        band[bandIdx].currentFreq = currentFrequency;
        band[bandIdx].currentStep = currentStep;
        useBand();
        showFrequency();
      }
    }
    else if (buttonFM.justPressed()) // Switch to VFH/FM
    {
      if (currentMode != FM)
      {
        band[bandIdx].currentFreq = currentFrequency;
        band[bandIdx].currentStep = currentStep;
        ssbLoaded = false;
        cmdBFO = false;
        currentMode = FM;
        bandIdx = 0;
        useBand();
        showFrequency();
      }
    }
    else if (buttonMW.justPressed()) // Switch to MW/AM
    {
      band[bandIdx].currentFreq = currentFrequency;
      band[bandIdx].currentStep = currentStep;
      ssbLoaded = false;
      cmdBFO = false;
      currentMode = AM;
      bandIdx = 2; // See Band table
      useBand();
    }
    else if (buttonSW.justPressed()) // Switch to SW/AM
    {
      band[bandIdx].currentFreq = currentFrequency;
      band[bandIdx].currentStep = currentStep;
      ssbLoaded = false;
      cmdBFO = false;
      currentMode = AM;
      bandIdx = lastSwBand; // See Band table
      useBand();
    }
    else if (buttonLSB.justPressed()) // Switch to LSB mode
    {
      if (currentMode != FM)
      {
        if (!ssbLoaded)
        {
          loadSSB();
        }
        currentMode = LSB;
        band[bandIdx].currentFreq = currentFrequency;
        band[bandIdx].currentStep = currentStep;
        useBand();
      }
    }
    else if (buttonUSB.justPressed()) // Switch to USB mode
    {
      if (currentMode != FM)
      {
        if (!ssbLoaded)
        {
          loadSSB();
        }
        currentMode = USB;
        band[bandIdx].currentFreq = currentFrequency;
        band[bandIdx].currentStep = currentStep;
        useBand();
      }
    }
    else if (buttonAGC.justPressed()) // AGC and Attenuation control
    {
      buttonAGC.drawButton(false);
      disableCommands();
      cmdAgcAtt = true;
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (buttonFilter.justPressed()) // FILTER
    {
      buttonFilter.drawButton(false);
      disableCommands();
      cmdFilter = true;
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (buttonStep.justPressed()) // STEP
    {
      // switchStep();
      buttonStep.drawButton(false);
      disableCommands();
      cmdStep = true;
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (buttonSync.justPressed())
    {
      cmdSync = !cmdSync;
      switchSync(0);
      delay(MIN_ELAPSED_TIME);
    }
  }

  // ENCODER PUSH BUTTON
  if (digitalRead(ENCODER_PUSH_BUTTON) == LOW)
  {
    if (currentMode == LSB || currentMode == USB)
    {
      doBFO();
    }
    else
    {
      cmdBand = !cmdBand;
      elapsedCommand = millis();
    }
    delay(300);
  }

  // Show RSSI status only if this condition has changed
  if ((millis() - elapsedRSSI) > MIN_ELAPSED_RSSI_TIME * 12)
  {
    si4735.getCurrentReceivedSignalQuality();
    int aux = si4735.getCurrentRSSI();
    if (rssi != aux)
    {
      rssi = aux;
      showRSSI();
    }
    elapsedRSSI = millis();
  }

  if (currentMode == FM)
  {
    if (currentFrequency != previousFrequency)
    {
      clearStatusArea();
      bufferStatioName[0] = bufferRdsMsg[0] = rdsTime[0] = bufferRdsTime[0] = rdsMsg[0] = stationName[0] = '\0';
      showRDSMsg();
      showRDSStation();
      previousFrequency = currentFrequency;
    }
    checkRDS();
  }

  // Disable commands control
  if ((millis() - elapsedCommand) > ELAPSED_COMMAND)
  {
    if (cmdBFO)
    {
      bufferFreq[0] = '\0';
      buttonBFO.initButton(&tft, 195, 185, 70, 49, WHITE, CYAN, BLACK, (char *)"BFO", 1);
      buttonBFO.drawButton(true);
      cmdBFO = false;
      showFrequency();
    }
    disableCommands();
    elapsedCommand = millis();
  }
  delay(5);
}
