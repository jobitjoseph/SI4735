/*
  This sketch uses an Arduino Pro Mini, 3.3V (8MZ) with a TM1638 7 segments display and controls

  Features:   AM; SSB; LW/MW/SW; two super band (from 150Khz to 30 MHz); external mute circuit control; Seek (Automatic tuning)
              AGC; Attenuation gain control; SSB filter; CW; AM filter; 1, 5, 10, 50 and 500KHz step on AM and 10Hhz sep on SSB

  Wire up on Arduino UNO, Pro mini and SI4735-D60

  | Device name               | Device Pin / Description      |  Arduino Pin  |
  | ----------------          | ----------------------------- | ------------  |
  |    TM1638                 |                               |               |
  |                           | STB                           |    4          |
  |                           | CLK                           |    7          |
  |                           | DIO                           |    8          |
  |                           | VCC                           |    3.3V       |
  |                           | GND                           |    GND        |
  |     Si4735                |                               |               |
  |                           | RESET (pin 15)                |     12        |
  |                           | SDIO (pin 18)                 |     A4        |
  |                           | SCLK (pin 17)                 |     A5        |
  |                           | SEN (pin 16)                  |    GND        |
  |    Encoder                |                               |               |
  |                           | A                             |       2       |
  |                           | B                             |       3       |
  |                           | Encoder button                |      A0       |


  Prototype documentation: https://pu2clr.github.io/SI4735/
  PU2CLR Si47XX API documentation: https://pu2clr.github.io/SI4735/extras/apidoc/html/
  Jim Reagan's schematic: https://github.com/JimReagans/Si4735-radio-PCB-s-and-bandpass-filter

  By PU2CLR, Ricardo; and W09CHL, Jim Reagan;  Sep  2020.

*/

#include <TM1638lite.h>
#include <SI4735.h>

#include "Rotary.h"

// Test it with patch_init.h or patch_full.h. Do not try load both.
#include "patch_init.h" // SSB patch for whole SSBRX initialization string

const uint16_t size_content = sizeof ssb_patch_content; // see ssb_patch_content in patch_full.h or patch_init.h

#define TM1638_STB   4
#define TM1638_CLK   7
#define TM1638_DIO   8

#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3

#define RESET_PIN 12

// Enconder PINs
#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3
#define ENCODER_PUSH_BUTTON 14 // Used to select the enconder control (BFO or VFO)

// TM1638 - Buttons controllers
#define BAND_BUTTON 1       // S1 Band switch button
#define MODE_SWITCH 2       // S2 FM/AM/SSB
#define BANDWIDTH_BUTTON 4  // S3 Used to select the banddwith. Values: 1.2, 2.2, 3.0, 4.0, 0.5, 1.0 KHz
#define SEEK_BUTTON 8       // S4 Seek
#define AGC_SWITCH  16      // S5 Switch AGC ON/OF
#define STEP_SWITCH 32      // S6 Increment or decrement frequency step (1, 5 or 10 KHz)
#define AUDIO_VOLUME 64     // S7 Volume Control
#define AUDIO_MUTRE 128     // S8 External AUDIO MUTE circuit control

#define MIN_ELAPSED_TIME 300
#define MIN_ELAPSED_RSSI_TIME 150
#define ELAPSED_COMMAND 2500 // time to turn off the last command controlled by encoder
#define DEFAULT_VOLUME 50    // change it for your favorite sound volume

#define FM 0
#define LSB 1
#define USB 2
#define AM 3
#define LW 4

#define SSB 1
#define CLEAR_BUFFER(x) (x[0] = '\0');

bool bfoOn = false;
bool ssbLoaded = false;

// AGC and attenuation control
int8_t agcIdx = 0;
uint8_t disableAgc = 0;
int8_t agcNdx = 0;

bool cmdBand = false;
bool cmdVolume = false;
bool cmdAgc = false;
bool cmdBandwidth = false;
bool cmdStep = false;
bool cmdMode = false;

int currentBFO = 0;
uint8_t seekDirection = 1; // Tells the SEEK direction (botton or upper limit)

long elapsedRSSI = millis();
long elapsedButton = millis();
long elapsedCommand = millis();

// Encoder control variables
volatile int encoderCount = 0;

// Some variables to check the SI4735 status
uint16_t currentFrequency;

uint8_t currentBFOStep = 10;

typedef struct
{
  uint8_t idx;      // SI473X device bandwitdth index
  const char *desc; // bandwitdth description
} Bandwitdth;

int8_t bwIdxSSB = 4;
Bandwitdth bandwitdthSSB[] = {{4, "0.5"},  // 0
  {5, "1.0"},  // 1
  {0, "1.2"},  // 2
  {1, "2.2"},  // 3
  {2, "3.0"},  // 4
  {3, "4.0"}
}; // 5

int8_t bwIdxAM = 4;
Bandwitdth bandwitdthAM[] = {{4, "1.0"},
  {5, "1.8"},
  {3, "2.0"},
  {6, "2.5"},
  {2, "3.0"},
  {1, "4.0"},
  {0, "6.0"}
};

const char *bandModeDesc[] = {"   ", "LSB", "USB", "AM "};
uint8_t currentMode = FM;

uint16_t currentStep = 1;

/*
   Band data structure
*/
typedef struct
{
  const char *bandName; // Band description
  uint8_t bandType;     // Band type (FM, MW or SW)
  uint16_t minimumFreq; // Minimum frequency of the band
  uint16_t maximumFreq; // maximum frequency of the band
  uint16_t currentFreq; // Default frequency or current frequency
  uint16_t currentStep; // Defeult step (increment and decrement)
} Band;

/*
   Band table
   Actually, except FM (VHF), the other bands cover the entire LW / MW and SW spectrum.
   Only the default frequency and step is changed. You can change this setup.
*/
Band band[] = {
  {"F ", FM_BAND_TYPE, 6400, 10800, 10390, 10},
  {"A ", MW_BAND_TYPE, 150, 1720, 810, 10},
  {"S1", SW_BAND_TYPE, 150, 30000, 7100, 1}, // Here and below: 150KHz to 30MHz
  {"S2", SW_BAND_TYPE, 150, 30000, 9600, 5},
  {"S3", SW_BAND_TYPE, 150, 30000, 11940, 5},
  {"S4", SW_BAND_TYPE, 150, 30000, 13600, 5},
  {"S5", SW_BAND_TYPE, 150, 30000, 14200, 1},
  {"S6", SW_BAND_TYPE, 150, 30000, 15300, 5},
  {"S7", SW_BAND_TYPE, 150, 30000, 17600, 5},
  {"S8", SW_BAND_TYPE, 150, 30000, 21100, 1},
  {"S9", SW_BAND_TYPE, 150, 30000, 28400, 1}
};

const int lastBand = (sizeof band / sizeof(Band)) - 1;
int bandIdx = 0;

int tabStep[] = {1, 5, 10, 50, 100, 500, 1000};
const int lastStep = (sizeof tabStep / sizeof(int)) - 1;
int idxStep = 0;

uint8_t rssi = 0;
uint8_t snr = 0;
uint8_t stereo = 1;
uint8_t volume = DEFAULT_VOLUME;

// Devices class declarations
Rotary encoder = Rotary(ENCODER_PIN_A, ENCODER_PIN_B);

TM1638lite tm(TM1638_STB, TM1638_CLK, TM1638_DIO);

SI4735 rx;

void setup()
{

  pinMode(ENCODER_PUSH_BUTTON, INPUT_PULLUP);

  tm.reset();
  showSplash();

  // uncomment the line below if you have external audio mute circuit
  // rx.setAudioMuteMcuPin(AUDIO_VOLUME);

  // Encoder interrupt
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);
  
  rx.setI2CFastMode(); // Set I2C bus speed.
  rx.getDeviceI2CAddress(RESET_PIN); // Looks for the I2C bus address and set it.  Returns 0 if error

  rx.setup(RESET_PIN, -1, 1, SI473X_ANALOG_AUDIO); // Starts FM mode and ANALOG audio mode.

  delay(400);
  useBand();
  rx.setVolume(volume);
  showStatus();
}

/**
    Set all command flags to false
    When all flags are disabled (false), the encoder controls the frequency
*/
void disableCommands()
{
  cmdBand = false;
  bfoOn = false;
  cmdVolume = false;
  cmdAgc = false;
  cmdBandwidth = false;
  cmdStep = false;
  cmdMode = false;

  tm.setLED(1, 0);  // Turn off the command LED indicator
  
}

/**
    Shows the static content on  display
*/
void showSplash()
{
  const char *s7= "-SI4735-";
  for (int i = 0; i < 8; i++) {
    tm.setLED(i, 1);
    delay(200);
    tm.displayASCII(i, s7[i]);
    tm.setLED(i, 0);
    delay(200);
  }

}

/**
     Reads encoder via interrupt
     Use Rotary.h and  Rotary.cpp implementation to process encoder via interrupt
*/
void rotaryEncoder()
{ // rotary encoder events
  uint8_t encoderStatus = encoder.process();

  if (encoderStatus)
    encoderCount = (encoderStatus == DIR_CW) ? 1 : -1;
}

/**
    Shows frequency information on Display
*/
void showFrequency()
{
  uint16_t color;
  char bufferDisplay[15];
  char tmp[15];

  // It is better than use dtostrf or String to save space.

  sprintf(tmp, "%5.5u", currentFrequency);

  bufferDisplay[0] = (tmp[0] == '0') ? ' ' : tmp[0];
  bufferDisplay[1] = tmp[1];
  if (rx.isCurrentTuneFM())
  {
    bufferDisplay[2] = tmp[2];
    bufferDisplay[3] = '.';
    bufferDisplay[4] = tmp[3];
  }
  else
  {
    if (currentFrequency < 1000)
    {
      bufferDisplay[1] = ' ';
      bufferDisplay[2] = tmp[2];
      bufferDisplay[3] = tmp[3];
      bufferDisplay[4] = tmp[4];
    }
    else
    {
      bufferDisplay[2] = tmp[2];
      bufferDisplay[3] = tmp[3];
      bufferDisplay[4] = tmp[4];
    }
  }
  bufferDisplay[5] = '\0';

  sprintf(tmp, "%s %s", band[bandIdx].bandName,bufferDisplay);
  
  tm.displayText(tmp);

}

/**
    This function is called by the seek function process.
*/
void showFrequencySeek(uint16_t freq)
{
  currentFrequency = freq;
  showFrequency();
}

/**
     Show some basic information on display
*/
void showStatus()
{

  showFrequency();
  if (rx.isCurrentTuneFM())
  {

  }
  else
  {

  }

  // showBandwitdth();
}

/**
   Shows the current Bandwitdth status
*/
void showBandwitdth()
{
  char bufferDisplay[15];
  // Bandwidth
  if (currentMode == LSB || currentMode == USB || currentMode == AM)
  {
    char *bw;

    if (currentMode == AM)
      bw = (char *)bandwitdthAM[bwIdxAM].desc;
    else
      bw = (char *)bandwitdthSSB[bwIdxSSB].desc;
    sprintf(bufferDisplay, "BW %s", bw);
  }
  else
  {
    bufferDisplay[0] = '\0';
  }
  tm.displayText(bufferDisplay);
}

/**
    Shows the current RSSI and SNR status
*/
void showRSSI()
{
  int rssiLevel;
  uint8_t rssiAux;
  int snrLevel;
  char sSt[10];
  char sRssi[7];

  if (currentMode == FM)
  {
    tm.setLED (0, rx.getCurrentPilot()); // Indicates Stereo or Mono 
  }

  // It needs to be calibrated. You can do it better.
  // RSSI: 0 to 127 dBuV

  if (rssi < 2)
    rssiAux = 4;
  else if (rssi < 4)
    rssiAux = 5;
  else if (rssi < 12)
    rssiAux = 6;
  else if (rssi < 25)
    rssiAux = 7;
  else if (rssi < 50)
    rssiAux = 8;
  else if (rssi >= 50)
    rssiAux = 9;

  for ( int i = 2; i < 8; i++ ) {
     tm.setLED(i, (rssiAux >= (i + 2)) );  
  }

}

/**
    Shows the current AGC and Attenuation status
*/
void showAgcAtt()
{
  char sAgc[10];
  rx.getAutomaticGainControl();
  if (agcNdx == 0 && agcIdx == 0)
    strcpy(sAgc, "AGC");
  else
    sprintf(sAgc, "ATT %2d", agcNdx);

  tm.displayText(sAgc);

}

/**
    Shows the current step
*/
void showStep()
{
  char sStep[15];
  sprintf(sStep, "STEP %3d", currentStep);
  tm.displayText(sStep);
}


/*
 *  Shows the volume level on LCD
 */
void showVolume()
{
  char volAux[12];
  sprintf(volAux, "Vol %2u", rx.getVolume());
  tm.displayText(volAux);
}

/**
   Shows the current BFO value
*/
void showBFO()
{
  // sprintf(bufferDisplay, "%+4d", currentBFO);
  // printValue(128, 30, bufferBFO, bufferDisplay, 7, ST77XX_CYAN, 1);
  // showFrequency();
  elapsedCommand = millis();
}


/**
 * Show cmd on display. It means you are setting up something.  
 */
inline void showCommandStatus(uint8_t v)
{
  tm.setLED(1,v);
}


/**
    Sets Band up (1) or down (!1)
*/
void setBand(int8_t up_down)
{
  band[bandIdx].currentFreq = currentFrequency;
  band[bandIdx].currentStep = currentStep;
  if (up_down == 1)
    bandIdx = (bandIdx < lastBand) ? (bandIdx + 1) : 0;
  else
    bandIdx = (bandIdx > 0) ? (bandIdx - 1) : lastBand;
  useBand();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}

/**
    This function loads the contents of the ssb_patch_content array into the CI (Si4735) and starts the radio on
    SSB mode.
    See also loadPatch implementation in the SI4735 Arduino Library (SI4735.h/SI4735.cpp)
*/
void loadSSB()
{
  rx.reset();
  rx.queryLibraryId(); // Is it really necessary here? I will check it.
  rx.patchPowerUp();
  delay(50);
  rx.setI2CFastMode(); // Recommended
  // rx.setI2CFastModeCustom(500000); // It is a test and may crash.
  rx.downloadPatch(ssb_patch_content, size_content);
  rx.setI2CStandardMode(); // goes back to default (100KHz)

  // Parameters
  // AUDIOBW - SSB Audio bandwidth; 0 = 1.2KHz (default); 1=2.2KHz; 2=3KHz; 3=4KHz; 4=500Hz; 5=1KHz;
  // SBCUTFLT SSB - side band cutoff filter for band passand low pass filter ( 0 or 1)
  // AVC_DIVIDER  - set 0 for SSB mode; set 3 for SYNC mode.
  // AVCEN - SSB Automatic Volume Control (AVC) enable; 0=disable; 1=enable (default).
  // SMUTESEL - SSB Soft-mute Based on RSSI or SNR (0 or 1).
  // DSP_AFCDIS - DSP AFC Disable or enable; 0=SYNC MODE, AFC enable; 1=SSB MODE, AFC disable.
  rx.setSSBConfig(bandwitdthSSB[bwIdxSSB].idx, 1, 0, 0, 0, 1);
  delay(25);
  ssbLoaded = true;
}

/**
    Switch the radio to current band
*/
void useBand()
{
  if (band[bandIdx].bandType == FM_BAND_TYPE)
  {
    currentMode = FM;
    rx.setTuneFrequencyAntennaCapacitor(0);
    rx.setFM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
    rx.setSeekFmLimits(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq);
    bfoOn = ssbLoaded = false;
  }
  else
  {
    // set the tuning capacitor for SW or MW/LW
    rx.setTuneFrequencyAntennaCapacitor((band[bandIdx].bandType == MW_BAND_TYPE || band[bandIdx].bandType == LW_BAND_TYPE) ? 0 : 1);

    if (ssbLoaded)
    {
      rx.setSSB(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep, currentMode);
      rx.setSSBAutomaticVolumeControl(1);
    }
    else
    {
      currentMode = AM;
      rx.setAM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
      bfoOn = false;
    }
    rx.setAmSoftMuteMaxAttenuation(0); // Disable Soft Mute for AM or SSB
    rx.setAutomaticGainControl(disableAgc, agcNdx);
    rx.setSeekAmLimits(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq);               // Consider the range all defined current band
    rx.setSeekAmSpacing((band[bandIdx].currentStep > 10) ? 10 : band[bandIdx].currentStep); // Max 10KHz for spacing
  }
  delay(100);
  currentFrequency = band[bandIdx].currentFreq;
  currentStep = band[bandIdx].currentStep;
  idxStep = getStepIndex(currentStep);
  rssi = 0;
  showStatus();
  showCommandStatus(1);
}

/**
    Switches the Bandwidth
*/
void doBandwidth(int8_t v)
{
  if (currentMode == LSB || currentMode == USB)
  {
    bwIdxSSB = (v == 1) ? bwIdxSSB + 1 : bwIdxSSB - 1;

    if (bwIdxSSB > 5)
      bwIdxSSB = 0;
    else if (bwIdxSSB < 0)
      bwIdxSSB = 5;

    rx.setSSBAudioBandwidth(bandwitdthSSB[bwIdxSSB].idx);
    // If audio bandwidth selected is about 2 kHz or below, it is recommended to set Sideband Cutoff Filter to 0.
    if (bandwitdthSSB[bwIdxSSB].idx == 0 || bandwitdthSSB[bwIdxSSB].idx == 4 || bandwitdthSSB[bwIdxSSB].idx == 5)
      rx.setSBBSidebandCutoffFilter(0);
    else
      rx.setSBBSidebandCutoffFilter(1);
  }
  else if (currentMode == AM)
  {
    bwIdxAM = (v == 1) ? bwIdxAM + 1 : bwIdxAM - 1;

    if (bwIdxAM > 6)
      bwIdxAM = 0;
    else if (bwIdxAM < 0)
      bwIdxAM = 6;

    rx.setBandwidth(bandwitdthAM[bwIdxAM].idx, 1);
  }
  showBandwitdth();
  elapsedCommand = millis();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
}

/**
    Deal with AGC and attenuattion
*/
void doAgc(int8_t v)
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

  rx.setAutomaticGainControl(disableAgc, agcNdx); // if agcNdx = 0, no attenuation
  showAgcAtt();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}

/**
    Gets the current step index.
*/
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
    Switches the current step
*/
void doStep(int8_t v)
{
  idxStep = (v == 1) ? idxStep + 1 : idxStep - 1;
  if (idxStep > lastStep)
    idxStep = 0;
  else if (idxStep < 0)
    idxStep = lastStep;

  currentStep = tabStep[idxStep];

  rx.setFrequencyStep(currentStep);
  band[bandIdx].currentStep = currentStep;
  rx.setSeekAmSpacing((currentStep > 10) ? 10 : currentStep); // Max 10KHz for spacing
  showStep();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}

/**
    Switches to the AM, LSB or USB modes
*/
void doMode(int8_t v)
{

  if (currentMode != FM)
  {
    if (currentMode == AM)
    {
      // If you were in AM mode, it is necessary to load SSB patch (avery time)
      loadSSB();
      currentMode = LSB;
    }
    else if (currentMode == LSB)
    {
      currentMode = USB;
    }
    else if (currentMode == USB)
    {
      currentMode = AM;
      bfoOn = ssbLoaded = false;
    }
    // Nothing to do if you are in FM mode
    band[bandIdx].currentFreq = currentFrequency;
    band[bandIdx].currentStep = currentStep;
    useBand();
  }

  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}

/**
    Find a station. The direction is based on the last encoder move clockwise or counterclockwise
*/
void doSeek()
{
  rx.seekStationProgress(showFrequencySeek, seekDirection);
  currentFrequency = rx.getFrequency();
}



/**
 * Sets the audio volume
 */
void doVolume( int8_t v ) {
  if ( v == 1)
    rx.volumeUp();
  else
    rx.volumeDown();

  showVolume();
  delay(MIN_ELAPSED_TIME); // waits a little more for releasing the button.
  elapsedCommand = millis();
}


void loop()
{

  // Check if the encoder has moved.
  if (encoderCount != 0)
  {
    if (bfoOn & (currentMode == LSB || currentMode == USB))
    {
      currentBFO = (encoderCount == 1) ? (currentBFO + currentBFOStep) : (currentBFO - currentBFOStep);
      rx.setSSBBfo(currentBFO);
      showBFO();
    }
    else if (cmdMode)
      doMode(encoderCount);
    else if (cmdStep)
      doStep(encoderCount);
    else if (cmdAgc)
      doAgc(encoderCount);
    else if (cmdBandwidth)
      doBandwidth(encoderCount);
    else if (cmdVolume)
      doVolume(encoderCount);      
    else if (cmdBand)
      setBand(encoderCount);
    else
    {
      if (encoderCount == 1)
      {
        rx.frequencyUp();
        seekDirection = 1;
      }
      else
      {
        rx.frequencyDown();
        seekDirection = 0;
      }
      // Show the current frequency only if it has changed
      currentFrequency = rx.getFrequency();
      showFrequency();
    }
    encoderCount = 0;
  }
  else
  {
    uint8_t tm_button = tm.readButtons();

    delay(50);
    // tm.displayHex(7, tm_button);

    if (tm_button == BANDWIDTH_BUTTON)
    {
      cmdBandwidth = !cmdBandwidth;
      showCommandStatus(cmdBandwidth);
      showBandwitdth();
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    } else if (tm_button == AUDIO_VOLUME) {
      cmdVolume = !cmdVolume;
      showCommandStatus(cmdVolume);
      showVolume();
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (tm_button == BAND_BUTTON)
    {
      cmdBand = !cmdBand;
      showCommandStatus(cmdBand);
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (tm_button == SEEK_BUTTON)
    {
      doSeek();
    }
    else if (digitalRead(ENCODER_PUSH_BUTTON) == LOW)
    {
      bfoOn = !bfoOn;
      if ((currentMode == LSB || currentMode == USB))
        showBFO();

      showFrequency();
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (tm_button == AGC_SWITCH )
    {
      cmdAgc = !cmdAgc;
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (tm_button ==  STEP_SWITCH)
    {
      cmdStep = !cmdStep;
      showCommandStatus(cmdStep);
      showStep();
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
    else if (tm_button == MODE_SWITCH)
    {
      cmdMode = !cmdMode;
      showCommandStatus(cmdMode);
      delay(MIN_ELAPSED_TIME);
      elapsedCommand = millis();
    }
  }

  // Show RSSI status only if this condition has changed
  if ((millis() - elapsedRSSI) > MIN_ELAPSED_RSSI_TIME * 6)
  {
    rx.getCurrentReceivedSignalQuality();
    int aux = rx.getCurrentRSSI();
    if (rssi != aux)
    {
      rssi = aux;
      snr = rx.getCurrentSNR();
      showRSSI();
    }
    elapsedRSSI = millis();
  }

  // Disable commands control
  if ((millis() - elapsedCommand) > ELAPSED_COMMAND)
  {
    if ((currentMode == LSB || currentMode == USB))
    {
      bfoOn = false;
      showBFO();
    }
    showFrequency();
    disableCommands();
    elapsedCommand = millis();
  }
  delay(1);
}
