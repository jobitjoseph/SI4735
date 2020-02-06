/* 
  This sketch uses the mcufriend TFT touct Display on Arduino Mega2580.
   
  Features:
  1) This sketch has been successfully tested on Arduino Mega2560;
  2) It uses the touch screen interface provided by mcufriend TFT;
  3) Encoder;
  4) FM, AM (MW and SW) and SSB (LSB and USB);
  5) Audio bandwidth filter 0.5, 1, 1.2, 2.2, 3 and 4Khz;
  6) BFO Control; and
  7) Frequency step switch (1, 5 and 10KHz).
  
  This sketch will download a SSB patch to your SI4735 device (patch_init.h). It will take about 8KB of memory.
  In this context, a patch is a piece of software used to change the behavior of the SI4735 device.
  There is little information available about patching the SI4735. The following information is the understanding of the author of
  this project and it is not necessarily correct. A patch is executed internally (run by internal MCU) of the device.
  Usually, patches are used to fixes bugs or add improvements and new features of the firmware installed in the internal ROM of the device.
  Patches to the SI4735 are distributed in binary form and have to be transferred to the internal RAM of the device by
  the host MCU (in this case the Arduino Mega2560). Since the RAM is volatile memory, the patch stored into the device gets lost when you turn off the system.
  Consequently, the content of the patch has to be transferred again to the device each time after turn on the system or reset the device.

  ATTENTION: The author of this project does not guarantee that procedures shown here will work in your development environment.
  Given this, it is at your own risk to continue with the procedures suggested here.
  This library works with the I2C communication protocol and it is designed to apply a SSB extension PATCH to CI SI4735-D60.
  Once again, the author disclaims any liability for any damage this procedure may cause to your SI4735 or other devices that you are using.

  Libraries used: SI4735; Adafruit_GFX; MCUFRIEND_kbv; FreeDefaultFonts; TouchScreen; 

  By Ricardo Lima Caratti, Feb 2020
 */

#include <SI4735.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <FreeDefaultFonts.h>
#include <TouchScreen.h>
#include "Rotary.h"

#include "patch_init.h" // SSB patch for whole SSBRX initialization string

#define MINPRESSURE 200
#define MAXPRESSURE 1000

#define RESET_PIN 22 // Mega2560 digital Pin used to RESET
// Enconder PINs (interrupt pins used on Mega2560)
#define ENCODER_PIN_A 18
#define ENCODER_PIN_B 19

#define AM_FUNCTION 1
#define FM_FUNCTION 0

#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3

#define MIN_ELAPSED_TIME 100
#define MIN_ELAPSED_RSSI_TIME 150
#define DEFAULT_VOLUME 50 // change it for your favorite sound volume

#define FM 0
#define LSB 1
#define USB 2
#define AM 3
#define LW 4
#define SSB 1

const uint16_t size_content = sizeof ssb_patch_content; // see ssb_patch_content in patch_full.h or patch_init.h

bool bfoOn = false;
bool disableAgc = true;
bool ssbLoaded = false;
bool fmStereo = true;
bool touch = false;

int currentBFO = 0;
int previousBFO = 0;

long elapsedRSSI = millis();
long elapsedButton = millis();
long elapsedFrequency = millis();


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
  {"60m ", SW_BAND_TYPE, 4500, 5500, 4850, 5},
  {"49m ", SW_BAND_TYPE, 5600, 6300, 6000, 5},
  {"41m ", SW_BAND_TYPE, 6800, 7800, 7100, 5}, // 40 meters
  {"31m ", SW_BAND_TYPE, 9200, 10000, 9600, 5},
  {"30m ", SW_BAND_TYPE, 10000, 11000, 10100, 1}, // 30 meters
  {"25m ", SW_BAND_TYPE, 11200, 12500, 11940, 5},
  {"22m ", SW_BAND_TYPE, 13400, 13900, 13600, 5},
  {"20m ", SW_BAND_TYPE, 14000, 14500, 14200, 1}, // 20 meters
  {"19m ", SW_BAND_TYPE, 15000, 15900, 15300, 5},
  {"18m ", SW_BAND_TYPE, 17200, 17900, 17600, 5},
  {"17m ", SW_BAND_TYPE, 18000, 18300, 18100, 1},  // 17 meters
  {"15m ", SW_BAND_TYPE, 21000, 21900, 21200, 1},  // 15 mters
  {"12m ", SW_BAND_TYPE, 24890, 26200, 24940, 1},  // 12 meters
  {"CB  ", SW_BAND_TYPE, 26200, 27900, 27500, 1},  // CB band (11 meters)
  {"10m ", SW_BAND_TYPE, 28000, 30000, 28400, 1}
};

const int lastBand = (sizeof band / sizeof(Band)) - 1;
int bandIdx = 0;

uint16_t currentFrequency;
uint16_t previousFrequency;
uint8_t bandwidthIdx = 0;
uint8_t currentStep = 1;
uint8_t currentBFOStep = 25;

uint8_t bwIdxSSB = 2;
const char *bandwitdthSSB[] = {"1.2", "2.2", "3.0", "4.0", "0.5", "1.0"};

uint8_t bwIdxAM = 1;
const char *bandwitdthAM[] = {"6", "4", "3", "2", "1", "1.8", "2.5"};

const char *bandModeDesc[] = {"FM ", "LSB", "USB", "AM "};
uint8_t currentMode = FM;

char buffer[64];

Rotary encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);
MCUFRIEND_kbv tft;
SI4735 si4735;

// ALL Touch panels and wiring is DIFFERENT
// copy-paste results from TouchScreen_Calibr_native.ino
const int XP = 6, XM = A2, YP = A1, YM = 7; //240x320 ID=0x9328
const int TS_LEFT = 294, TS_RT = 795, TS_TOP = 189, TS_BOT = 778;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

Adafruit_GFX_Button bFrequencyUp, bFrequencyDown, bSwitchBand;

int pixel_x, pixel_y; //Touch_getXY() updates global vars
bool Touch_getXY(void)
{
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT); //restore shared pins
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH); //because TFT control pins
  digitalWrite(XM, HIGH);
  bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
  if (pressed)
  {
    pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width()); //.kbv makes sense to me
    pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
  }
  return pressed;
}

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

void setup(void)
{

  // Encoder pins
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  uint16_t ID = tft.readID();

  if (ID == 0xD3D3)
    ID = 0x9486; // write-only shield
  tft.begin(ID);
  tft.setRotation(0); //PORTRAIT
  tft.fillScreen(BLACK);
  tft.setFont(&FreeSans9pt7b);
  bFrequencyUp.initButton(&tft, 60, 200, 115, 40, WHITE, CYAN, BLACK, (char *)"Seek Up", 1);
  bFrequencyDown.initButton(&tft, 180, 200, 115, 40, WHITE, CYAN, BLACK, (char *)"Seek Down", 1);
  bSwitchBand.initButton(&tft, 120, 250, 200, 40, WHITE, CYAN, BLACK, (char *)"AM/FM", 1);
  bFrequencyUp.drawButton(false);
  bFrequencyDown.drawButton(false);
  bSwitchBand.drawButton(false);
  tft.fillRect(40, 80, 160, 80, RED);

  showText(35, 15, 1, &FreeSans9pt7b, GREEN, "SI4735 Arduino Library");
  showText(35, 35, 1, &FreeSans9pt7b, YELLOW, "       By PU2CLR      ");

  // Atach Encoder pins interrupt
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);

  si4735.setup(RESET_PIN, 1);

  // Set up the radio for the current band (see index table variable bandIdx )
  // useBand();
  currentFrequency = previousFrequency = si4735.getFrequency();

  tft.setFont(&FreeSans9pt7b); // default font
}


/* 
 * Use Rotary.h and  Rotary.cpp implementation to process encoder via interrupt 
 * 
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


/*
 * Shows a text on a given position; with a given size and font, and with a given color
 * 
 * @param int x column
 * @param int y line
 * @param int sz font size
 * @param const GFXfont *f font type
 * @param uint16_t color
 * @param char * msg message 
 */
void showText(int x, int y, int sz, const GFXfont *f, uint16_t color, const char *msg)
{
  tft.setFont(f);
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(sz);
  tft.print(msg);
}

/*
 * 
 */
void showFrequency()
{
  float freq;
  int iFreq, dFreq;
  if (si4735.isCurrentTuneFM())
  {
    freq = currentFrequency / 100.0;
    iFreq = (int)freq;
    dFreq = (int)(freq - iFreq);
    sprintf(buffer, "%3d.%2d MHz", iFreq, dFreq);
  }
  else
  {
    freq = currentFrequency / 1000.0;
    iFreq = (int)freq;
    dFreq = (int)(freq - iFreq);
    sprintf(buffer, "%2d.%3d KHz", iFreq, dFreq);
  }
  showText(10, 40, 2, &FreeSans12pt7b, buffer, BLUE);

  tft.setFont(&FreeSans9pt7b); // default font
}

void showStatus()
{
  si4735.getStatus();
  si4735.getCurrentReceivedSignalQuality();
  // SRN
  tft.fillRect(20, 280, 160, 80, BLUE);

  si4735.getFrequency();
  showFrequency();
}

void showBFO()
{
}

void showVolume()
{
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
  si4735.reset();
  si4735.queryLibraryId(); // Is it really necessary here? I will check it.
  si4735.patchPowerUp();
  delay(50);
  // si4735.setI2CFastMode(); // Recommended
  si4735.setI2CFastModeCustom(500000); // It is a test and may crash.
  si4735.downloadPatch(ssb_patch_content, size_content);
  si4735.setI2CStandardMode(); // goes back to default (100KHz)

  // delay(50);
  // Parameters
  // AUDIOBW - SSB Audio bandwidth; 0 = 1.2KHz (default); 1=2.2KHz; 2=3KHz; 3=4KHz; 4=500Hz; 5=1KHz;
  // SBCUTFLT SSB - side band cutoff filter for band passand low pass filter ( 0 or 1)
  // AVC_DIVIDER  - set 0 for SSB mode; set 3 for SYNC mode.
  // AVCEN - SSB Automatic Volume Control (AVC) enable; 0=disable; 1=enable (default).
  // SMUTESEL - SSB Soft-mute Based on RSSI or SNR (0 or 1).
  // DSP_AFCDIS - DSP AFC Disable or enable; 0=SYNC MODE, AFC enable; 1=SSB MODE, AFC disable.
  si4735.setSSBConfig(bwIdxSSB, 1, 0, 0, 0, 1);
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
    bfoOn = ssbLoaded = false;
  }
  else
  {
    if (band[bandIdx].bandType == MW_BAND_TYPE || band[bandIdx].bandType == LW_BAND_TYPE)
      si4735.setTuneFrequencyAntennaCapacitor(0);
    else
      si4735.setTuneFrequencyAntennaCapacitor(1);

    if (ssbLoaded)
    {
      si4735.setSSB(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep, currentMode);
      si4735.setSSBAutomaticVolumeControl(1);
    }
    else
    {
      currentMode = AM;
      si4735.reset();
      si4735.setAM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
      si4735.setAutomaticGainControl(1, 0);
      bfoOn = false;
    }

  }

  currentFrequency = band[bandIdx].currentFreq;
  currentStep = band[bandIdx].currentStep;
  showStatus();
}



/* two buttons are quite simple
*/
void loop(void)
{
  bool down = Touch_getXY();
  bFrequencyUp.press(down && bFrequencyUp.contains(pixel_x, pixel_y));
  bFrequencyDown.press(down && bFrequencyDown.contains(pixel_x, pixel_y));
  bSwitchBand.press(down && bSwitchBand.contains(pixel_x, pixel_y));


  // Check if the encoder has moved.
  if (encoderCount != 0)
  {
    if (bfoOn)
    {
      currentBFO = (encoderCount == 1) ? (currentBFO + currentBFOStep) : (currentBFO - currentBFOStep);
    }
    else
    {
      if (encoderCount == 1)
        si4735.frequencyUp();
      else
        si4735.frequencyDown();

      // Show the current frequency only if it has changed
      delay(30);
      currentFrequency = si4735.getFrequency();
      showFrequency();
    }
    encoderCount = 0;
  }

  if (bFrequencyUp.justReleased())
    bFrequencyUp.drawButton(false);

  if (bFrequencyDown.justReleased())
    bFrequencyDown.drawButton(false);

  if (bFrequencyUp.justPressed())
  {
    bFrequencyUp.drawButton(false);
    bandUp();
  }

  if (bFrequencyDown.justPressed())
  {
    bFrequencyDown.drawButton(false);
    bandDown();
  }

  if (bSwitchBand.justReleased())
    bSwitchBand.drawButton(false);

  if (bSwitchBand.justPressed())
  {
    bSwitchBand.drawButton(false);

  }

  delay(15);

}
