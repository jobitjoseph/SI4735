/**
 * @mainpage SI47XX Arduino Library implementation 
 * 
 * This is a library for the SI4735, BROADCAST AM/FM/SW RADIO RECEIVER, IC from Silicon Labs for the 
 * Arduino development environment.  It works with I2C protocol and can provide an easier interface for controlling the SI47XX CI family.<br>
 * 
 * This library was built based on [Si47XX PROGRAMMING GUIDE-AN332](https://www.silabs.com/documents/public/application-notes/AN332.pdf) document from Silicon Labs. 
 * It also can be used on **all members of the SI473X family** respecting, of course, the features available for each IC version. 
 * These functionalities can be seen in the comparison matrix shown in table 1 (Product Family Function); pages 2 and 3 of the programming guide.
 * If you need to build a prototype based on SI47XX device, see <https://pu2clr.github.io/SI4735/><br>
 * 
 * This larary has more than 20 examples. See <https://github.com/pu2clr/SI4735/tree/master/examples><br>
 * This library can be freely distributed using the MIT Free Software model. [Copyright (c) 2019 Ricardo Lima Caratti](https://pu2clr.github.io/SI4735/#mit-license).  
 * Contact: pu2clr@gmail.com
 * 
 * @details This library uses the I²C communication protocol and implements most of the functions offered by Si47XX (BROADCAST AM / FM / SW / LW RADIO RECEIVER) IC family from Silicon Labs. 
 * @details The main features of this library are listed below. 
 * @details 1. Open Source. It is free. You can use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software. See [MIT License](https://pu2clr.github.io/SI4735/#mit-license) to know more.   
 * @details 2. Built Based on [Si47XX PROGRAMMING GUIDE](https://www.silabs.com/documents/public/application-notes/AN332.pdf).
 * @details 3. C++ Language and Object-oriented programming. You can easily extend the SI4735 class by adding more functionalities.
 * @details 4. Available on Arduino IDE (Manage Libraries).
 * @details 5. Cross-platform. You can compile and run this library on most of board available on Arduino IDE (Examples: ATtiny85, boards based on ATmega328 and ATmega-32u4, ATmega2560, 32 ARM Cortex, Arduino DUE, ESP32 and more). See [Boards where this library has been successfully tested](https://pu2clr.github.io/SI4735/#boards-where-this-library-has-been-successfully-tested).
 * @details 6. Simplifies projects based on SI4735.
 * @details 7. I²C communication and Automatic I²C bus address detection. 
 * @details 8. More than 120 functions implemented. You can customize almost every feature available on Si47XX family. 
 * @details 9. RDS support.
 * @details 10. SSB (Single Side Band) patch support. 
 * @details 11. Digital Audio.
 * 
 * @see https://pu2clr.github.io/SI4735/
 * @see Si47XX PROGRAMMING GUIDE AN332: https://www.silabs.com/documents/public/application-notes/AN332.pdf
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; AMENDMENT FOR SI4735-D60 SSB AND NBFM PATCHES
 * 
 * @author PU2CLR - Ricardo Lima Caratti 
 * @date  2019-2020
 * @copyright MIT Free Software model. See [Copyright (c) 2019 Ricardo Lima Caratti](https://pu2clr.github.io/SI4735/#mit-license). 
 */

#include <SI4735.h>

/**
 * @brief Construct a new SI4735::SI4735 
 * 
 * @details This class has a set of functions that can help you to build your receiver based on Si47XX IC family.
 * @details This library uses the I²C communication protocol and implements most of the functions offered by Si47XX (BROADCAST AM / FM / SW / LW RADIO RECEIVER) IC family from Silicon Labs. 
 * @details Currently you have more than 120 functions implemented to control the Si47XX devices. These functions are listed and documented here.
 * @details Some methods were implemented using inline resource. Inline methods are implemented in SI4735.h
 * 
 * IMPORTANT: According to Si47XX PROGRAMMING GUIDE; AN332; page 207, "For write operations, the system controller next 
 * sends a data byte on SDIO, which is captured by the device on rising edges of SCLK. The device acknowledges 
 * each data byte by driving SDIO low for one cycle on the next falling edge of SCLK. 
 * The system controller may write up to 8 data bytes in a single 2-wire transaction. 
 * The first byte is a command, and the next seven bytes are arguments. Writing more than 8 bytes results 
 * in unpredictable device behavior". So, If you are extending this library, consider that restriction presented earlier.
 */
SI4735::SI4735()
{
    // 1 = LSB and 2 = USB; 0 = AM, FM or WB
    currentSsbStatus = 0;
}


/** @defgroup group05 Deal with Interrupt and I2C bus */


/**
 * @ingroup group05 Interrupt
 * @brief Interrupt handle
 * @details If you setup interrupt, this function will be called whenever the Si4735 changes. 
 * 
 */
void SI4735::waitInterrupr(void)
{
    while (!data_from_si4735)
        ;
}

/** 
 * @ingroup group05 I2C bus address
 * 
 * @brief I2C bus address setup
 * 
 * @details Scans for two possible addresses for the Si47XX (0x11 or 0x63).
 * @details This function also sets the system to the found I2C bus address of Si47XX.
 * @details You do not need to use this function if the SEN PIN is configured to ground (GND). 
 * The default I2C address is 0x11.
 * @details Use this function if you do not know how the SEN pin is configured.
 * 
 * @param uint8_t  resetPin MCU Mater (Arduino) reset pin
 * 
 * @return int16_t 0x11   if the SEN pin of the Si47XX is low or 0x63 if the SEN pin of the Si47XX is HIGH or 0x0 if error.    
 */
int16_t SI4735::getDeviceI2CAddress(uint8_t resetPin) {
    int16_t error;

    pinMode(resetPin, OUTPUT);
    delay(50);
    digitalWrite(resetPin, LOW);
    delay(50);
    digitalWrite(resetPin, HIGH);

    Wire.begin();
    // check 0X11 I2C address
    Wire.beginTransmission(SI473X_ADDR_SEN_LOW);
    error = Wire.endTransmission(); 
    if ( error == 0 ) {
      setDeviceI2CAddress(0);  
      return SI473X_ADDR_SEN_LOW;
    }

    // check 0X63 I2C address
    Wire.beginTransmission(SI473X_ADDR_SEN_HIGH);
    error = Wire.endTransmission();  
    if ( error == 0 ) {
      setDeviceI2CAddress(1);  
      return SI473X_ADDR_SEN_HIGH;
    }  

    // Did find the device   
    return 0;
}

/** 
 * @ingroup group05 I2C bus address
 * 
 * @brief Sets the I2C Bus Address
 *
 * @details The parameter senPin is not the I2C bus address. It is the SEN pin setup of the schematic (eletronic circuit).
 * @details If it is connected to the ground, call this function with senPin = 0; else senPin = 1.
 * @details You do not need to use this function if the SEN PIN configured to ground (GND).
 * @details The default value is 0x11 (senPin = 0). In this case you have to ground the pin SEN of the SI473X. 
 * @details If you want to change this address, call this function with senPin = 1.
 *  
 * @param senPin 0 -  when the pin SEN (16 on SSOP version or pin 6 on QFN version) is set to low (GND - 0V);
 *               1 -  when the pin SEN (16 on SSOP version or pin 6 on QFN version) is set to high (+3.3V).
 */
void SI4735::setDeviceI2CAddress(uint8_t senPin) {
    deviceAddress = (senPin)? SI473X_ADDR_SEN_HIGH : SI473X_ADDR_SEN_LOW;
};

/**
 * @ingroup group05 I2C bus address
 * 
 * @brief Sets the onther I2C Bus Address (for Si470X) 
 * 
 * @details You can set another I2C address different of 0x11  and 0x63
 * 
 * @param uint8_t i2cAddr (example 0x10)
 */
void SI4735::setDeviceOtherI2CAddress(uint8_t i2cAddr) {
    deviceAddress = i2cAddr;
};

/** @defgroup group06 Host and slave MCU setup */

/**
 * @ingroup group06 RESET
 * 
 * @brief Reset the SI473X  
 *  
 * @see Si47XX PROGRAMMING GUIDE; AN332;
 */
void SI4735::reset()
{
    pinMode(resetPin, OUTPUT);
    delay(10);
    digitalWrite(resetPin, LOW);
    delay(10);
    digitalWrite(resetPin, HIGH);
    delay(10);
}

/**
 * @ingroup group06 Wait to send command 
 * 
 * @brief  Wait for the si473x is ready (Clear to Send (CTS) status bit have to be 1).  
 * 
 * @details This function should be used before sending any command to a SI47XX device.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 63, 128
 */
void SI4735::waitToSend()
{
    do
    {
        delayMicroseconds(MIN_DELAY_WAIT_SEND_LOOP); // Need check the minimum value.
        Wire.requestFrom(deviceAddress, 1);
    } while (!(Wire.read() & B10000000));
}

/**
 * @ingroup group06 Si47XX device Power Up parameters 
 *  
 * @brief Set the Power Up parameters for si473X. 
 * 
 * @details Use this method to chenge the defaul behavior of the Si473X. Use it before PowerUp()
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 65 and 129
 * 
 * @param uint8_t CTSIEN sets Interrupt anabled or disabled (1 = anabled and 0 = disabled )
 * @param uint8_t GPO2OEN sets GP02 Si473X pin enabled (1 = anabled and 0 = disabled )
 * @param uint8_t PATCH  Used for firmware patch updates. Use it always 0 here. 
 * @param uint8_t XOSCEN sets external Crystal enabled or disabled 
 * @param uint8_t FUNC sets the receiver function have to be used [0 = FM Receive; 1 = AM (LW/MW/SW) and SSB (if SSB patch apllied)]
 * @param uint8_t OPMODE set the kind of audio mode you want to use.
 */
void SI4735::setPowerUp(uint8_t CTSIEN, uint8_t GPO2OEN, uint8_t PATCH, uint8_t XOSCEN, uint8_t FUNC, uint8_t OPMODE)
{
    powerUp.arg.CTSIEN = CTSIEN;   // 1 -> Interrupt anabled;
    powerUp.arg.GPO2OEN = GPO2OEN; // 1 -> GPO2 Output Enable;
    powerUp.arg.PATCH = PATCH;     // 0 -> Boot normally;
    powerUp.arg.XOSCEN = XOSCEN;   // 1 -> Use external crystal oscillator;
    powerUp.arg.FUNC = FUNC;       // 0 = FM Receive; 1 = AM/SSB (LW/MW/SW) Receiver.
    powerUp.arg.OPMODE = OPMODE;   // 0x5 = 00000101 = Analog audio outputs (LOUT/ROUT).

    // Set the current tuning frequancy mode 0X20 = FM and 0x40 = AM (LW/MW/SW)
    // See See Si47XX PROGRAMMING GUIDE; AN332; pages 55 and 124

    if (FUNC == 0)
    {
        currentTune = FM_TUNE_FREQ;
        currentFrequencyParams.arg.FREEZE = 1;
    }
    else
    {
        currentTune = AM_TUNE_FREQ;
        currentFrequencyParams.arg.FREEZE = 0;
    }
    currentFrequencyParams.arg.FAST = 1;
    currentFrequencyParams.arg.DUMMY1 = 0;
    currentFrequencyParams.arg.ANTCAPH = 0;
    currentFrequencyParams.arg.ANTCAPL = 1;
}

/**
 * @ingroup group06 Si47XX device Power Up 
 * 
 * @brief Powerup the Si47XX
 * 
 * @details Before call this function call the setPowerUp to set up the parameters.
 * 
 * @details Parameters you have to set up with setPowerUp
 * | Parameter | Description |
 * | --------- | ----------- |
 * | CTSIEN    | Interrupt anabled or disabled |
 * | GPO2OEN   | GPO2 Output Enable or disabled |
 * | PATCH     | Boot normally or patch |
 * | XOSCEN    | Use external crystal oscillator |
 * | FUNC      | defaultFunction = 0 = FM Receive; 1 = AM (LW/MW/SW) Receiver |
 * | OPMODE    | SI473X_ANALOG_AUDIO (B00000101) or SI473X_DIGITAL_AUDIO (B00001011) |
 * 
 * @see  SI4735::setPowerUp()
 * @see  Si47XX PROGRAMMING GUIDE; AN332; pages 64, 129
 */
void SI4735::radioPowerUp(void) {
    // delayMicroseconds(1000);
    waitToSend();
    Wire.beginTransmission(deviceAddress);
    Wire.write(POWER_UP);
    Wire.write(powerUp.raw[0]); // Content of ARG1
    Wire.write(powerUp.raw[1]); // COntent of ARG2
    Wire.endTransmission();
    // Delay at least 500 ms between powerup command and first tune command to wait for
    // the oscillator to stabilize if XOSCEN is set and crystal is used as the RCLK.
    waitToSend();
    delay(10);
}

/**
 * @ingroup   group06 Si47XX device Power Up 
 * 
 * @brief You have to call setPowerUp method before. 
 * @details This function is still available only for legacy reasons. 
 *          If you are using this function, please, replace it by radioPowerup().
 * @deprecated Use radioPowerUp instead.
 * @see  SI4735::setPowerUp()
 * @see  Si47XX PROGRAMMING GUIDE; AN332; pages 64, 129
 */
void SI4735::analogPowerUp(void)
{
    radioPowerUp();
}

/** 
 * @ingroup   group06 Si47XX device Power Down 
 * 
 * @brief Moves the device from powerup to powerdown mode.
 * 
 * @details After Power Down command, only the Power Up command is accepted.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 67, 132
 * @see radioPowerUp()
 */
void SI4735::powerDown(void)
{
    waitToSend();
    Wire.beginTransmission(deviceAddress);
    Wire.write(POWER_DOWN);
    Wire.endTransmission();
    delayMicroseconds(2500);
}

/** @defgroup group07 Si47XX device information and start up */

/**
 * @ingroup   group07 Firmware Information 
 * 
 * @brief Gets firmware information 
 * @details The firmware information will be stored in firmwareInfo member variable 
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 66, 131
 * @see firmwareInfo
 */
void SI4735::getFirmware(void)
{
    waitToSend();

    Wire.beginTransmission(deviceAddress);
    Wire.write(GET_REV);
    Wire.endTransmission();

    do
    {
        waitToSend();
        // Request for 9 bytes response
        Wire.requestFrom(deviceAddress, 9);
        for (int i = 0; i < 9; i++)
            firmwareInfo.raw[i] = Wire.read();
    } while (firmwareInfo.resp.ERR);
}

/** 
 * @ingroup   group07 Device start up 
 *  
 * @brief Starts the Si473X device. 
 *
 * @details Use this function to start the device up with the parameters shown below.
 * @details If the audio mode parameter is not entered, analog mode will be considered.
 * @details You can use any Arduino digital pin. Be sure you are using less than 3.6V on Si47XX RST pin.   
 *  
 * @param uint8_t resetPin Digital Arduino Pin used to RESET de Si47XX device. 
 * @param uint8_t interruptPin interrupt Arduino Pin (see your Arduino pinout). If less than 0, iterrupt disabled.
 * @param uint8_t defaultFunction is the mode you want the receiver starts.
 * @param uint8_t audioMode default SI473X_ANALOG_AUDIO (Analog Audio). Use SI473X_ANALOG_AUDIO or SI473X_DIGITAL_AUDIO.
 */
void SI4735::setup(uint8_t resetPin, int interruptPin, uint8_t defaultFunction, uint8_t audioMode)
{
    uint8_t interruptEnable = 0;
    Wire.begin();

    this->resetPin = resetPin;
    this->interruptPin = interruptPin;

    // Arduino interrupt setup (you have to know which Arduino Pins can deal with interrupt).
    if (interruptPin >= 0)
    {
        pinMode(interruptPin, INPUT);
        attachInterrupt(digitalPinToInterrupt(interruptPin), interrupt_hundler, RISING);
        interruptEnable = 1;
    }

    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, HIGH);

    data_from_si4735 = false;

    // Set the initial SI473X behavior
    // CTSIEN   1 -> Interrupt anabled or disable;
    // GPO2OEN  1 -> GPO2 Output Enable;
    // PATCH    0 -> Boot normally;
    // XOSCEN   1 -> Use external crystal oscillator;
    // FUNC     defaultFunction = 0 = FM Receive; 1 = AM (LW/MW/SW) Receiver.
    // OPMODE   SI473X_ANALOG_AUDIO or SI473X_DIGITAL_AUDIO.
    setPowerUp(interruptEnable, 0, 0, 1, defaultFunction, audioMode);

    reset();
    radioPowerUp();
    setVolume(30); // Default volume level.
    getFirmware();
}

/** 
 * @ingroup   group07 Device start up 
 * 
 * @brief  Starts the Si473X device.  
 * 
 * @details Use this setup if you are not using interrupt resource.
 * @details If the audio mode parameter is not entered, analog mode will be considered.
 * @details You can use any Arduino digital pin. Be sure you are using less than 3.6V on Si47XX RST pin.   
 * 
 * @param uint8_t resetPin Digital Arduino Pin used to RESET command. 
 * @param uint8_t defaultFunction. 0 =  FM mode; 1 = AM 
 */
void SI4735::setup(uint8_t resetPin, uint8_t defaultFunction)
{
    setup(resetPin, -1, defaultFunction);
    delay(250);
}

/** @defgroup group08 Si47XX device Mode, Band and Frequency setup */

/**
 * @ingroup   group08 Internal Antenna Tuning capacitor
 * 
 * @brief Selects the tuning capacitor value.
 * 
 * @details For FM, Antenna Tuning Capacitor is valid only when using TXO/LPI pin as the antenna input.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 71 and 136
 * 
 * @param capacitor If zero, the tuning capacitor value is selected automatically. 
 *                  If the value is set to anything other than 0:
 *                  AM - the tuning capacitance is manually set as 95 fF x ANTCAP + 7 pF. 
 *                       ANTCAP manual range is 1–6143;
 *                  FM - the valid range is 0 to 191.    
 *                  According to Silicon Labs, automatic capacitor tuning is recommended (value 0). 
 */
void SI4735::setTuneFrequencyAntennaCapacitor(uint16_t capacitor)
{
    si47x_antenna_capacitor cap;

    cap.value = capacitor;

    currentFrequencyParams.arg.DUMMY1 = 0;

    if (currentTune == FM_TUNE_FREQ)
    {
        // For FM, the capacitor value has just one byte
        currentFrequencyParams.arg.ANTCAPH = (capacitor <= 191) ? cap.raw.ANTCAPL : 0;
    }
    else
    {
        if (capacitor <= 6143)
        {
            currentFrequencyParams.arg.FREEZE = 0; // This parameter is not used for AM
            currentFrequencyParams.arg.ANTCAPH = cap.raw.ANTCAPH;
            currentFrequencyParams.arg.ANTCAPL = cap.raw.ANTCAPL;
        }
    }
}

/**
 * @ingroup   group08 Tune Frequency 
 * 
 * @brief Set the frequency to the corrent function of the Si4735 (FM, AM or SSB)
 * 
 * @details You have to call setup or setPowerUp before call setFrequency.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 70, 135
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; page 13
 * 
 * @param uint16_t  freq Is the frequency to change. For example, FM => 10390 = 103.9 MHz; AM => 810 = 810 KHz.
 */
void SI4735::setFrequency(uint16_t freq)
{
    waitToSend(); // Wait for the si473x is ready.
    currentFrequency.value = freq;
    currentFrequencyParams.arg.FREQH = currentFrequency.raw.FREQH;
    currentFrequencyParams.arg.FREQL = currentFrequency.raw.FREQL;

    if (currentSsbStatus != 0)
    {
        currentFrequencyParams.arg.DUMMY1 = 0;
        currentFrequencyParams.arg.USBLSB = currentSsbStatus; // Set to LSB or USB
        currentFrequencyParams.arg.FAST = 1;                  // Used just on AM and FM
        currentFrequencyParams.arg.FREEZE = 0;                // Used just on FM
    }

    Wire.beginTransmission(deviceAddress);
    Wire.write(currentTune);
    Wire.write(currentFrequencyParams.raw[0]); // Send a byte with FAST and  FREEZE information; if not FM must be 0;
    Wire.write(currentFrequencyParams.arg.FREQH);
    Wire.write(currentFrequencyParams.arg.FREQL);
    Wire.write(currentFrequencyParams.arg.ANTCAPH);
    // If current tune is not FM sent one more byte
    if (currentTune != FM_TUNE_FREQ)
        Wire.write(currentFrequencyParams.arg.ANTCAPL);

    Wire.endTransmission();
    waitToSend();                // Wait for the si473x is ready.
    currentWorkFrequency = freq; // check it
    delay(MAX_DELAY_AFTER_SET_FREQUENCY); // For some reason I need to delay here. 
}

/** 
 * @ingroup group08 Tune Frequency step
 * 
 * @brief Sets the current step value. 
 * 
 * @details This function does not check the limits of the current band. Please, don't take a step bigger than your legs.
 * 
 * @param step if you are using FM, 10 means 100KHz. If you are using AM 10 means 10KHz
 *             For AM, 1 (1KHz) to 1000 (1MHz) are valid values.
 *             For FM 5 (50KHz) and 10 (100KHz) are valid values.  
 */
void SI4735::setFrequencyStep(uint16_t step)
{
    currentStep = step;
}

/**
 * @ingroup group08 Tune Frequency 
 *  
 * @brief Increments the current frequency on current band/function by using the current step.
 * 
 * @see setFrequencyStep()
 */
void SI4735::frequencyUp()
{
    if (currentWorkFrequency >= currentMaximumFrequency)
        currentWorkFrequency = currentMinimumFrequency;
    else
        currentWorkFrequency += currentStep;

    setFrequency(currentWorkFrequency);
}

/**
 * @ingroup group08 Tune Frequency 
 * 
 * @brief Decrements the current frequency on current band/function by using the current step.
 *  
 * @see setFrequencyStep()
 */
void SI4735::frequencyDown()
{

    if (currentWorkFrequency <= currentMinimumFrequency)
        currentWorkFrequency = currentMaximumFrequency;
    else
        currentWorkFrequency -= currentStep;

    setFrequency(currentWorkFrequency);
}

/**
 * @ingroup group08 Set mode and Band
 * 
 * @brief Sets the radio to AM function. It means: LW MW and SW.
 * 
 * @details Define the band range you want to use for the AM mode. 
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 129.
 */
void SI4735::setAM()
{
    // If you’re already using AM mode, it is not necessary to call powerDown and radioPowerUp. 
    // The other properties also should have the same value as the previous status.
    if ( lastMode != AM_CURRENT_MODE ) {
        powerDown();
        setPowerUp(1, 1, 0, 1, 1, SI473X_ANALOG_AUDIO);
        radioPowerUp();
        setAvcAmMaxGain(currentAvcAmMaxGain); // Set AM Automatic Volume Gain to 48
        setVolume(volume); // Set to previus configured volume
    }
    currentSsbStatus = 0;
    lastMode = AM_CURRENT_MODE;
}

/**
 * @ingroup group08 Set mode and Band
 * 
 * @brief Sets the radio to FM function
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 64. 
 */
void SI4735::setFM()
{
    powerDown();
    setPowerUp(1, 1, 0, 1, 0, SI473X_ANALOG_AUDIO);
    radioPowerUp();
    setVolume(volume); // Set to previus configured volume
    currentSsbStatus = 0;
    disableFmDebug();
    lastMode = FM_CURRENT_MODE;
}

/**
 * @ingroup group08 Set mode and Band
 * 
 * @brief Sets the radio to AM (LW/MW/SW) function. 
 * 
 * @details The example below sets the band from 550KHz to 1550KHz on AM mode. The band will start on 810KHz and step is 10KHz. 
 * 
 * @code
 * si4735.setAM(520, 1750, 810, 10);
 * @endcode
 *  
 * @see setAM()
 * 
 * 
 * 
 * @param fromFreq minimum frequency for the band
 * @param toFreq maximum frequency for the band
 * @param initialFreq initial frequency 
 * @param step step used to go to the next channel 
 */
void SI4735::setAM(uint16_t fromFreq, uint16_t toFreq, uint16_t initialFreq, uint16_t step)
{

    currentMinimumFrequency = fromFreq;
    currentMaximumFrequency = toFreq;
    currentStep = step;

    if (initialFreq < fromFreq || initialFreq > toFreq)
        initialFreq = fromFreq;

    setAM();
    currentWorkFrequency = initialFreq;
    setFrequency(currentWorkFrequency);
}

/**
 * @ingroup group08 Set mode and Band
 * 
 * @brief Sets the radio to FM function. 
 * 
 * @details Defines the band range you want to use for the FM mode. 
 * 
 * @details The example below sets the band from 64MHz to 108MHzKHz on FM mode. The band will start on 103.9MHz and step is 100KHz.
 * On FM mode, the step 10 means 100KHz.    
 * 
 * @code
 * si4735.setFM(6400, 10800, 10390, 10);
 * @endcode
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 70
 * @see setFM()
 * 
 * @param fromFreq minimum frequency for the band
 * @param toFreq maximum frequency for the band
 * @param initialFreq initial frequency (default frequency)
 * @param step step used to go to the next channel   
 */
void SI4735::setFM(uint16_t fromFreq, uint16_t toFreq, uint16_t initialFreq, uint16_t step)
{

    currentMinimumFrequency = fromFreq;
    currentMaximumFrequency = toFreq;
    currentStep = step;

    if (initialFreq < fromFreq || initialFreq > toFreq)
        initialFreq = fromFreq;

    setFM();

    currentWorkFrequency = initialFreq;
    setFrequency(currentWorkFrequency);
}

/**
 * @ingroup group08 Check FM mode status
 * 
 * @brief Returns true if the current function is FM (FM_TUNE_FREQ).
 * 
 * @return true if the current function is FM (FM_TUNE_FREQ).
 */
bool SI4735::isCurrentTuneFM()
{
    return (currentTune == FM_TUNE_FREQ);
}


/** @defgroup group09 Si47XX filter setup  */

/**
 * @ingroup group09 Set bandwidth
 * 
 * @brief Selects the bandwidth of the channel filter for AM reception. 
 * 
 * @details The choices are 6, 4, 3, 2, 2.5, 1.8, or 1 (kHz). The default bandwidth is 2 kHz. It works only in AM / SSB (LW/MW/SW) 
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 125, 151, 277, 181.
 * 
 * @param AMCHFLT the choices are:   0 = 6 kHz Bandwidth                    
 *                                   1 = 4 kHz Bandwidth
 *                                   2 = 3 kHz Bandwidth
 *                                   3 = 2 kHz Bandwidth
 *                                   4 = 1 kHz Bandwidth
 *                                   5 = 1.8 kHz Bandwidth
 *                                   6 = 2.5 kHz Bandwidth, gradual roll off
 *                                   7–15 = Reserved (Do not use).
 * @param AMPLFLT Enables the AM Power Line Noise Rejection Filter.
 */
void SI4735::setBandwidth(uint8_t AMCHFLT, uint8_t AMPLFLT)
{
    si47x_bandwidth_config filter;
    si47x_property property;

    if (currentTune == FM_TUNE_FREQ) // Only for AM/SSB mode
        return;

    if (AMCHFLT > 6)
        return;

    property.value = AM_CHANNEL_FILTER;

    filter.param.AMCHFLT = AMCHFLT;
    filter.param.AMPLFLT = AMPLFLT;

    waitToSend();
    this->volume = volume;
    Wire.beginTransmission(deviceAddress);
    Wire.write(SET_PROPERTY);
    Wire.write(0x00);                  // Always 0x00
    Wire.write(property.raw.byteHigh); // High byte first
    Wire.write(property.raw.byteLow);  // Low byte after
    Wire.write(filter.raw[1]);         // Raw data for AMCHFLT and
    Wire.write(filter.raw[0]);         // AMPLFLT
    Wire.endTransmission();
    waitToSend();
}

/** @defgroup group10 Tools method 
 * @details A set of functions used to support other functions
*/

/**
 * @ingroup group10 Generic send property
 * 
 * @brief Sends (sets) property to the SI47XX
 * 
 * @details This method is used for others to send generic properties and params to SI47XX
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 68, 124 and  133.
 */
void SI4735::sendProperty(uint16_t propertyValue, uint16_t parameter)
{
    si47x_property property;
    si47x_property param;

    property.value = propertyValue;
    param.value = parameter;
    waitToSend();
    Wire.beginTransmission(deviceAddress);
    Wire.write(SET_PROPERTY);
    Wire.write(0x00);
    Wire.write(property.raw.byteHigh); // Send property - High byte - most significant first
    Wire.write(property.raw.byteLow);  // Send property - Low byte - less significant after
    Wire.write(param.raw.byteHigh);    // Send the argments. High Byte - Most significant first
    Wire.write(param.raw.byteLow);     // Send the argments. Low Byte - Less significant after
    Wire.endTransmission();
    delayMicroseconds(550);
}

/** @defgroup group12 FM Mono Stereo audio setup */

/**
 * @ingroup group12 FM Mono Stereo audio setup
 *  
 * @brief Sets RSSI threshold for stereo blend (Full stereo above threshold, blend below threshold). 
 * 
 * @details To force stereo, set this to 0. To force mono, set this to 127.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 90. 
 * 
 * @param parameter  valid values: 0 to 127
 */
void SI4735::setFmBlendStereoThreshold(uint8_t parameter)
{
    sendProperty(FM_BLEND_STEREO_THRESHOLD, parameter);
}

/**
 * @ingroup group12 FM Mono Stereo audio setup
 * 
 * @brief Sets RSSI threshold for mono blend (Full mono below threshold, blend above threshold). 
 * 
 * @details To force stereo set this to 0. To force mono set this to 127. Default value is 30 dBμV.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 56.
 * 
 * @param parameter valid values: 0 to 127
 */
void SI4735::setFmBlendMonoThreshold(uint8_t parameter)
{
    sendProperty(FM_BLEND_MONO_THRESHOLD, parameter);
}

/** 
 * @ingroup group12 FM Mono Stereo audio setup
 * 
 * @brief Sets RSSI threshold for stereo blend. (Full stereo above threshold, blend below threshold.) 
 * 
 * @details To force stereo, set this to 0. To force mono, set this to 127. Default value is 49 dBμV.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 59. 
 * 
 * @param parameter valid values: 0 to 127
 */
void SI4735::setFmBlendRssiStereoThreshold(uint8_t parameter)
{
    sendProperty(FM_BLEND_RSSI_STEREO_THRESHOLD, parameter);
}

/** 
 * @ingroup group12 FM Mono Stereo audio setup
 * 
 * @brief Sets RSSI threshold for mono blend (Full mono below threshold, blend above threshold). 
 * 
 * @details To force stereo, set this to 0. To force mono, set this to 127. Default value is 30 dBμV.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 59.  
 * 
 * @param parameter valid values: 0 to 127
 */
void SI4735::setFmBLendRssiMonoThreshold(uint8_t parameter)
{
    sendProperty(FM_BLEND_RSSI_MONO_THRESHOLD, parameter);
}

/**
 * @ingroup group12 FM Mono Stereo audio setup
 * 
 * @brief Sets SNR threshold for stereo blend (Full stereo above threshold, blend below threshold). 
 * 
 * @details To force stereo, set this to 0. To force mono, set this to 127. Default value is 27 dB.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 59.  
 * 
 * @param parameter valid values: 0 to 127
 */
void SI4735::setFmBlendSnrStereoThreshold(uint8_t parameter)
{
    sendProperty(FM_BLEND_SNR_STEREO_THRESHOLD, parameter);
}

/**
 * @ingroup group12 FM Mono Stereo audio setup
 * 
 * @brief Sets SNR threshold for mono blend (Full mono below threshold, blend above threshold). 
 * 
 * @details To force stereo, set this to 0. To force mono, set this to 127. Default value is 14 dB.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 59. 
 * 
 * @param parameter valid values: 0 to 127 
 */
void SI4735::setFmBLendSnrMonoThreshold(uint8_t parameter)
{
    sendProperty(FM_BLEND_SNR_MONO_THRESHOLD, parameter);
}

/** 
 * @ingroup group12 FM Mono Stereo audio setup
 * 
 * @brief Sets multipath threshold for stereo blend (Full stereo below threshold, blend above threshold). 
 * 
 * @details To force stereo, set this to 100. To force mono, set this to 0. Default value is 20.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 60.
 * 
 * @param parameter valid values: 0 to 100 
 */
void SI4735::setFmBlendMultiPathStereoThreshold(uint8_t parameter)
{
    sendProperty(FM_BLEND_MULTIPATH_STEREO_THRESHOLD, parameter);
}

/**
 * @ingroup group12 FM Mono Stereo audio setup
 * 
 * @brief Sets Multipath threshold for mono blend (Full mono above threshold, blend below threshold). 
 * 
 * @details To force stereo, set to 100. To force mono, set to 0. The default is 60.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 60.
 * 
 * @param parameter valid values: 0 to 100 
 */
void SI4735::setFmBlendMultiPathMonoThreshold(uint8_t parameter)
{
    sendProperty(FM_BLEND_MULTIPATH_MONO_THRESHOLD, parameter);
}

/** 
 * @ingroup group12 FM Mono Stereo audio setup
 * @todo 
 * @brief Turn Off Stereo operation.
 */
void SI4735::setFmStereoOff()
{
    //! TO DO
}

/** 
 * @ingroup group12 FM Mono Stereo audio setup
 * @todo 
 * @brief Turn Off Stereo operation.
 */
void SI4735::setFmStereoOn()
{
    //! TO DO
}

/**
 * @ingroup group12 FM Mono Stereo audio setup
 * 
 * @brief There is a debug feature that remains active in Si4704/05/3x-D60 firmware which can create periodic noise in audio.
 * 
 * @details Silicon Labs recommends you disable this feature by sending the following bytes (shown here in hexadecimal form):
 * 0x12 0x00 0xFF 0x00 0x00 0x00.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 299. 
 */
void SI4735::disableFmDebug()
{
    Wire.beginTransmission(deviceAddress);
    Wire.write(0x12);
    Wire.write(0x00);
    Wire.write(0xFF);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.endTransmission();
    delayMicroseconds(2500);
}

/** @defgroup group13 Audio setup */

/**
 * @ingroup group13 Digital Audio setup
 * 
 * @brief Configures the digital audio output format. 
 * 
 * @details Options: DCLK edge, data format, force mono, and sample precision.
 *  
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 195. 

 * @param uint8_t OSIZE Digital Output Audio Sample Precision (0=16 bits, 1=20 bits, 2=24 bits, 3=8bits).
 * @param uint8_t OMONO Digital Output Mono Mode (0=Use mono/stereo blend ).
 * @param uint8_t OMODE Digital Output Mode (0=I2S, 6 = Left-justified, 8 = MSB at second DCLK after DFS pulse, 12 = MSB at first DCLK after DFS pulse).
 * @param uint8_t OFALL Digital Output DCLK Edge (0 = use DCLK rising edge, 1 = use DCLK falling edge)
 */
void SI4735::digitalOutputFormat(uint8_t OSIZE, uint8_t OMONO, uint8_t OMODE, uint8_t OFALL)
{
    si4735_digital_output_format df;
    df.refined.OSIZE = OSIZE;
    df.refined.OMONO = OMONO;
    df.refined.OMODE = OMODE;
    df.refined.OFALL = OFALL;
    sendProperty(DIGITAL_OUTPUT_FORMAT, df.raw);
}

/**
 * @ingroup group13 Digital Audio setup
 * 
 * @brief Enables digital audio output and configures digital audio output sample rate in samples per second (sps).
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 196. 
 * 
 * @param uint16_t DOSR Digital Output Sample Rate(32–48 ksps .0 to disable digital audio output).
 */
void SI4735::digitalOutputSampleRate(uint16_t DOSR)
{
    sendProperty(DIGITAL_OUTPUT_SAMPLE_RATE, DOSR);
}

/** 
 * @ingroup group13 Audio volume
 * 
 * @brief Sets volume level (0  to 63)
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 62, 123, 170, 173 and 204
 * 
 * @param uint8_t volume (domain: 0 - 63) 
 */
void SI4735::setVolume(uint8_t volume)
{
    sendProperty(RX_VOLUME, volume);
    this->volume = volume;
}

/**
 * @ingroup group13 Audio volume
 * 
 * @brief Sets the audio on or off
 * 
 * @see See Si47XX PROGRAMMING GUIDE; AN332; pages 62, 123, 171 
 * 
 * @param value if true, mute the audio; if false unmute the audio.
 */
void SI4735::setAudioMute(bool off)
{
    uint16_t value = (off) ? 3 : 0; // 3 means mute; 0 means unmute
    sendProperty(RX_HARD_MUTE, value);
}

/**
 * @ingroup group13 Audio volume
 * 
 * @brief Gets the current volume level.
 * 
 * @see setVolume()
 * 
 * @return volume (domain: 0 - 63) 
 */
uint8_t SI4735::getVolume()
{
    return this->volume;
}

/**
 * @ingroup group13 Audio volume
 *  
 * @brief Set sound volume level Up   
 *  
 * @see setVolume()
 */
void SI4735::volumeUp()
{
    if (volume < 63)
        volume++;
    setVolume(volume);
}

/**
 * @ingroup group13 Audio volume
 *  
 * @brief Set sound volume level Down   
 * 
 * @see setVolume() 
 */
void SI4735::volumeDown()
{
    if (volume > 0)
        volume--;
    setVolume(volume);
}

/** @defgroup group14 Frequency and Si47XX device status */

/*******************************************************************************
 * Device Status Information
 ******************************************************************************/

/**
 * @ingroup group14 Frequency 
 * 
 * @brief Gets the current frequency of the Si4735 (AM or FM)
 * 
 * @details The method status do it an more. See getStatus below. 
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 73 (FM) and 139 (AM)
 */
uint16_t SI4735::getFrequency()
{
    si47x_frequency freq;
    getStatus(0, 1);

    freq.raw.FREQL = currentStatus.resp.READFREQL;
    freq.raw.FREQH = currentStatus.resp.READFREQH;

    currentWorkFrequency = freq.value;
    return freq.value;
}

/**
 * @ingroup group14 Frequency 
 * 
 * @brief Gets the current frequency saved in memory. 
 * 
 * @details Unlike getFrequency, this method gets the current frequency recorded after the last setFrequency command. 
 * @details This method avoids bus traffic and CI processing.
 * @details However, you can not get others status information like RSSI.
 * 
 * @see getFrequency()
 */
uint16_t SI4735::getCurrentFrequency()
{
    return currentWorkFrequency;
}

/**
 * @ingroup group14 Frequency 
 * 
 * @brief Gets the current status  of the Si4735 (AM or FM)
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 73 (FM) and 139 (AM)
 * 
 * @param uint8_t INTACK Seek/Tune Interrupt Clear. If set, clears the seek/tune complete interrupt status indicator;
 * @param uint8_t CANCEL Cancel seek. If set, aborts a seek currently in progress;
 */
void SI4735::getStatus(uint8_t INTACK, uint8_t CANCEL)
{
    si47x_tune_status status;
    uint8_t cmd = (currentTune == FM_TUNE_FREQ) ? FM_TUNE_STATUS : AM_TUNE_STATUS;

    waitToSend();

    status.arg.INTACK = INTACK;
    status.arg.CANCEL = CANCEL;

    Wire.beginTransmission(deviceAddress);
    Wire.write(cmd);
    Wire.write(status.raw);
    Wire.endTransmission();
    // Reads the current status (including current frequency).
    do
    {
        waitToSend();
        Wire.requestFrom(deviceAddress, 8); // Check it
        // Gets response information
        for (uint8_t i = 0; i < 8; i++)
            currentStatus.raw[i] = Wire.read();
    } while (currentStatus.resp.ERR); // If error, try it again
    waitToSend();
}

/**
 * @ingroup group14 Si47XX device Status 
 * 
 * @brief Gets the current status  of the Si4735 (AM or FM)
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 73 (FM) and 139 (AM)
 */
void SI4735::getStatus()
{
    getStatus(0, 1);
}

/**
 * @ingroup group14 Si47XX AGC 
 * 
 * @brief Queries Automatic Gain Control STATUS
 * 
 * @details After call this method, you can call isAgcEnabled to know the AGC status and getAgcGainIndex to know the gain index value.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; For FM page 80; for AM page 142.
 * @see AN332 REV 0.8 Universal Programming Guide Amendment for SI4735-D60 SSB and NBFM patches; page 18. 
 * 
 */
void SI4735::getAutomaticGainControl()
{
    uint8_t cmd;

    if (currentTune == FM_TUNE_FREQ)
    { // FM TUNE
        cmd = FM_AGC_STATUS;
    }
    else
    { // AM TUNE - SAME COMMAND used on SSB mode
        cmd = AM_AGC_STATUS;
    }

    waitToSend();

    Wire.beginTransmission(deviceAddress);
    Wire.write(cmd);
    Wire.endTransmission();

    do
    {
        waitToSend();
        Wire.requestFrom(deviceAddress, 3);
        currentAgcStatus.raw[0] = Wire.read(); // STATUS response
        currentAgcStatus.raw[1] = Wire.read(); // RESP 1
        currentAgcStatus.raw[2] = Wire.read(); // RESP 2
    } while (currentAgcStatus.refined.ERR);    // If error, try get AGC status again.
}

/** 
 * @ingroup group14 Si47XX AGC 
 * 
 * @brief Automatic Gain Control setup 
 * 
 * @details If FM, overrides AGC setting by disabling the AGC and forcing the LNA to have a certain gain that ranges between 0 
 * (minimum attenuation) and 26 (maximum attenuation).
 * @details If AM/SSB, Overrides the AM AGC setting by disabling the AGC and forcing the gain index that ranges between 0 
 * (minimum attenuation) and 37+ATTN_BACKUP (maximum attenuation).
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; For FM page 81; for AM page 143 
 * 
 * @param uint8_t AGCDIS This param selects whether the AGC is enabled or disabled (0 = AGC enabled; 1 = AGC disabled);
 * @param uint8_t AGCIDX AGC Index (0 = Minimum attenuation (max gain); 1 – 36 = Intermediate attenuation); 
 *                if >greater than 36 - Maximum attenuation (min gain) ).
 */
void SI4735::setAutomaticGainControl(uint8_t AGCDIS, uint8_t AGCIDX)
{
    si47x_agc_overrride agc;

    uint8_t cmd;

    cmd = (currentTune == FM_TUNE_FREQ) ? FM_AGC_OVERRIDE : AM_AGC_OVERRIDE;

    agc.arg.AGCDIS = AGCDIS;
    agc.arg.AGCIDX = AGCIDX;

    waitToSend();

    Wire.beginTransmission(deviceAddress);
    Wire.write(cmd);
    Wire.write(agc.raw[0]);
    Wire.write(agc.raw[1]);
    Wire.endTransmission();

    waitToSend();
}

/**
 * @ingroup group14 Si47XX Automatic Volume Control
 * 
 * @brief Sets the maximum gain for automatic volume control.
 *  
 * @details If no parameter is sent, it will be consider 48dB.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 152
 * @see setAvcAmMaxGain()
 * 
 * @param uint8_t gain  Select a value between 12 and 192.  Defaul value 48dB.
 */
void SI4735::setAvcAmMaxGain( uint8_t gain) {
    uint16_t aux;
    aux = ( gain > 12 && gain < 193 )? (gain * 340) : (48 * 340);
    currentAvcAmMaxGain =  gain;
    sendProperty(AM_AUTOMATIC_VOLUME_CONTROL_MAX_GAIN, aux);
}

/**
 * @ingroup group14 Si47XX Received Signal Quality
 * 
 * @brief Queries the status of the Received Signal Quality (RSQ) of the current channel.
 * 
 * @details This method sould be called berore call getCurrentRSSI(), getCurrentSNR() etc.
 * Command FM_RSQ_STATUS
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 75 and 141
 * 
 * @param INTACK Interrupt Acknowledge. 
 *        0 = Interrupt status preserved; 
 *        1 = Clears RSQINT, BLENDINT, SNRHINT, SNRLINT, RSSIHINT, RSSILINT, MULTHINT, MULTLINT.
 */
void  SI4735::getCurrentReceivedSignalQuality(uint8_t INTACK) 
{
        uint8_t arg;
        uint8_t cmd;
        int sizeResponse;

        if (currentTune == FM_TUNE_FREQ)
        { // FM TUNE
            cmd = FM_RSQ_STATUS;
            sizeResponse = 8; // Check it
        }
        else
        { // AM TUNE
            cmd = AM_RSQ_STATUS;
            sizeResponse = 6; // Check it
        }

        waitToSend();

        arg = INTACK;
        Wire.beginTransmission(deviceAddress);
        Wire.write(cmd);
        Wire.write(arg); // send B00000001
        Wire.endTransmission();

        // Check it
        // do
        //{
            waitToSend();
            Wire.requestFrom(deviceAddress, sizeResponse);
            // Gets response information
            for (uint8_t i = 0; i < sizeResponse; i++)
                currentRqsStatus.raw[i] = Wire.read();
        //} while (currentRqsStatus.resp.ERR); // Try again if error found
}

/**
 * @ingroup group14 Si47XX Received Signal Quality
 * 
 * @brief Queries the status of the Received Signal Quality (RSQ) of the current channel (FM_RSQ_STATUS)
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 75 and 141
 * 
 * @param INTACK Interrupt Acknowledge. 
 *        0 = Interrupt status preserved; 
 *        1 = Clears RSQINT, BLENDINT, SNRHINT, SNRLINT, RSSIHINT, RSSILINT, MULTHINT, MULTLINT.
 */
void SI4735::getCurrentReceivedSignalQuality(void)
{
    getCurrentReceivedSignalQuality(0);
}

/** @defgroup group15 Tune */

/**
 * @ingroup group15 Seek 
 * 
 * @brief Look for a station (Automatic tune)
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 55, 72, 125 and 137
 * 
 * @param SEEKUP Seek Up/Down. Determines the direction of the search, either UP = 1, or DOWN = 0. 
 * @param Wrap/Halt. Determines whether the seek should Wrap = 1, or Halt = 0 when it hits the band limit.
 */
void SI4735::seekStation(uint8_t SEEKUP, uint8_t WRAP)
{
    si47x_seek seek;

    // Check which FUNCTION (AM or FM) is working now
    uint8_t seek_start = (currentTune == FM_TUNE_FREQ) ? FM_SEEK_START : AM_SEEK_START;

    waitToSend();

    seek.arg.SEEKUP = SEEKUP;
    seek.arg.WRAP = WRAP;

    Wire.beginTransmission(deviceAddress);
    Wire.write(seek_start);
    Wire.write(seek.raw);

    if (seek_start == AM_SEEK_START)
    {
        Wire.write(0x00); // Always 0
        Wire.write(0x00); // Always 0
        Wire.write(0x00); // Tuning Capacitor: The tuning capacitor value
        Wire.write(0x00); //                   will be selected automatically.
    }

    Wire.endTransmission();
    delay(100);
}

/**
 * @ingroup group15 Seek 
 * 
 * @brief Search for the next station 
 * 
 * @see seekStation(uint8_t SEEKUP, uint8_t WRAP)
 */
void SI4735::seekStationUp()
{
    seekStation(1, 1);
    delay(50);
    getFrequency();
}

/**
 * @ingroup group15 Seek 
 * 
 * @brief Search the previous station
 * 
 * @see seekStation(uint8_t SEEKUP, uint8_t WRAP)
 */
void SI4735::seekStationDown()
{
    seekStation(0, 1);
    delay(50);
    getFrequency();
}

/**
 * @ingroup group15 Seek 
 * 
 * @brief Sets the bottom frequency and top frequency of the AM band for seek. Default is 520 to 1710.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 127, 161, and 162
 * 
 * @param uint16_t bottom - the bottom of the AM band for seek
 * @param uint16_t    top - the top of the AM band for seek
 */
void SI4735::setSeekAmLimits(uint16_t bottom, uint16_t top)
{
    sendProperty(AM_SEEK_BAND_BOTTOM, bottom);
    sendProperty(AM_SEEK_BAND_TOP, top);
}

/**
 * @ingroup group15 Seek 
 * 
 * @brief Selects frequency spacingfor AM seek. Default is 10 kHz spacing.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 163, 229 and 283
 * 
 * @param uint16_t spacing - step in KHz
 */
void SI4735::setSeekAmSpacing(uint16_t spacing)
{
    sendProperty(AM_SEEK_FREQ_SPACING, spacing);
}

/**
 * @ingroup group15 Seek 
 * 
 * @brief Sets the SNR threshold for a valid AM Seek/Tune. 
 * 
 * @details If the value is zero then SNR threshold is not considered when doing a seek. Default value is 5 dB.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 127
 */
void SI4735::setSeekSrnThreshold(uint16_t value)
{
    sendProperty(AM_SEEK_SNR_THRESHOLD, value);
}

/**
 * @ingroup group15 Seek 
 * 
 * @brief Sets the RSSI threshold for a valid AM Seek/Tune. 
 * 
 * @details If the value is zero then RSSI threshold is not considered when doing a seek. Default value is 25 dBμV.
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 127
 */
void SI4735::setSeekRssiThreshold(uint16_t value)
{
    sendProperty(AM_SEEK_RSSI_THRESHOLD, value);
}

/** @defgroup group16 FM RDS/DBDS */

/*******************************************************************************
 * RDS implementation 
 ******************************************************************************/

/**
 * @ingroup group16 RDS setup 
 *  
 * @brief  Starts the control member variables for RDS.
 * 
 * @details This method is called by setRdsConfig()
 * 
 * @see setRdsConfig()
 */
void SI4735::RdsInit()
{
    clearRdsBuffer2A();
    clearRdsBuffer2B();
    clearRdsBuffer0A();
    rdsTextAdress2A = rdsTextAdress2B = lastTextFlagAB = rdsTextAdress0A = 0;
}

/**
 * @ingroup group16 RDS setup 
 *  
 * @brief Clear RDS buffer 2A (text) 
 * 
 */
void SI4735::clearRdsBuffer2A()
{
    for (int i = 0; i < 65; i++)
        rds_buffer2A[i] = ' '; // Radio Text buffer - Program Information
}

/**
 * @ingroup group16 RDS setup 
 * 
 * @brief Clear RDS buffer 2B (text)
 * 
 */
void SI4735::clearRdsBuffer2B()
{
    for (int i = 0; i < 33; i++)
        rds_buffer2B[i] = ' '; // Radio Text buffer - Station Informaation
}
/**
 * @ingroup group16 RDS setup 
 * 
 * @brief Clear RDS buffer 0A (text)
 * 
 */
void SI4735::clearRdsBuffer0A()
{
    for (int i = 0; i < 9; i++)
        rds_buffer0A[i] = ' '; // Station Name buffer
}

/**
 * @ingroup group16 RDS setup 
 * 
 * @brief Sets RDS property
 * 
 * @details Configures RDS settings to enable RDS processing (RDSEN) and set RDS block error thresholds. 
 * @details When a RDS Group is received, all block errors must be less than or equal the associated block 
 * error threshold for the group to be stored in the RDS FIFO. 
 * @details IMPORTANT: 
 * All block errors must be less than or equal the associated block error threshold 
 * for the group to be stored in the RDS FIFO. 
 * |Value | Description |
 * |------| ----------- | 
 * | 0    | No errors |
 * | 1    | 1–2 bit errors detected and corrected |
 * | 2    | 3–5 bit errors detected and corrected |
 * | 3    | Uncorrectable |
 * 
 * @details Recommended Block Error Threshold options:
 * | Exemples | Description |
 * | -------- | ----------- |
 * | 2,2,2,2  | No group stored if any errors are uncorrected |
 * | 3,3,3,3  | Group stored regardless of errors |
 * | 0,0,0,0  | No group stored containing corrected or uncorrected errors |
 * | 3,2,3,3  | Group stored with corrected errors on B, regardless of errors on A, C, or D | 
 *  
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 104
 * 
 * @param uint8_t RDSEN RDS Processing Enable; 1 = RDS processing enabled.
 * @param uint8_t BLETHA Block Error Threshold BLOCKA.   
 * @param uint8_t BLETHB Block Error Threshold BLOCKB.  
 * @param uint8_t BLETHC Block Error Threshold BLOCKC.  
 * @param uint8_t BLETHD Block Error Threshold BLOCKD. 
 */
void SI4735::setRdsConfig(uint8_t RDSEN, uint8_t BLETHA, uint8_t BLETHB, uint8_t BLETHC, uint8_t BLETHD)
{
    si47x_property property;
    si47x_rds_config config;

    waitToSend();

    // Set property value
    property.value = FM_RDS_CONFIG;

    // Arguments
    config.arg.RDSEN = RDSEN;
    config.arg.BLETHA = BLETHA;
    config.arg.BLETHB = BLETHB;
    config.arg.BLETHC = BLETHC;
    config.arg.BLETHD = BLETHD;
    config.arg.DUMMY1 = 0;

    Wire.beginTransmission(deviceAddress);
    Wire.write(SET_PROPERTY);
    Wire.write(0x00);                  // Always 0x00 (I need to check it)
    Wire.write(property.raw.byteHigh); // Send property - High byte - most significant first
    Wire.write(property.raw.byteLow);  // Low byte
    Wire.write(config.raw[1]);         // Send the argments. Most significant first
    Wire.write(config.raw[0]);
    Wire.endTransmission();
    delayMicroseconds(550);

    RdsInit(); 
}

/** 
 * @ingroup group16 RDS setup 
 * 
 * @brief Configures interrupt related to RDS
 * 
 * @details Use this method if want to use interrupt
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; page 103
 * 
 * @param RDSRECV If set, generate RDSINT when RDS FIFO has at least FM_RDS_INT_FIFO_COUNT entries.
 * @param RDSSYNCLOST If set, generate RDSINT when RDS loses synchronization.
 * @param RDSSYNCFOUND set, generate RDSINT when RDS gains synchronization.
 * @param RDSNEWBLOCKA If set, generate an interrupt when Block A data is found or subsequently changed
 * @param RDSNEWBLOCKB If set, generate an interrupt when Block B data is found or subsequently changed
 */
void SI4735::setRdsIntSource(uint8_t RDSNEWBLOCKB, uint8_t RDSNEWBLOCKA, uint8_t RDSSYNCFOUND, uint8_t RDSSYNCLOST, uint8_t RDSRECV)
{
    si47x_property property;
    si47x_rds_int_source rds_int_source;

    if (currentTune != FM_TUNE_FREQ)
        return;

    rds_int_source.refined.RDSNEWBLOCKB = RDSNEWBLOCKB;
    rds_int_source.refined.RDSNEWBLOCKA = RDSNEWBLOCKA;
    rds_int_source.refined.RDSSYNCFOUND = RDSSYNCFOUND;
    rds_int_source.refined.RDSSYNCLOST = RDSSYNCLOST;
    rds_int_source.refined.RDSRECV = RDSRECV;
    rds_int_source.refined.DUMMY1 = 0;
    rds_int_source.refined.DUMMY2 = 0;

    property.value = FM_RDS_INT_SOURCE;

    waitToSend();

    Wire.beginTransmission(deviceAddress);
    Wire.write(SET_PROPERTY);
    Wire.write(0x00);                  // Always 0x00 (I need to check it)
    Wire.write(property.raw.byteHigh); // Send property - High byte - most significant first
    Wire.write(property.raw.byteLow);  // Low byte
    Wire.write(rds_int_source.raw[1]); // Send the argments. Most significant first
    Wire.write(rds_int_source.raw[0]);
    Wire.endTransmission();
    waitToSend();
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Gets the RDS status. Store the status in currentRdsStatus member. RDS COMMAND FM_RDS_STATUS
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 55 and 77
 * 
 * @param INTACK Interrupt Acknowledge; 0 = RDSINT status preserved. 1 = Clears RDSINT.
 * @param MTFIFO 0 = If FIFO not empty, read and remove oldest FIFO entry; 1 = Clear RDS Receive FIFO.
 * @param STATUSONLY Determines if data should be removed from the RDS FIFO.
 */
void SI4735::getRdsStatus(uint8_t INTACK, uint8_t MTFIFO, uint8_t STATUSONLY)
{
    si47x_rds_command rds_cmd;
    static uint16_t lastFreq;
    // checking current FUNC (Am or FM)
    if (currentTune != FM_TUNE_FREQ)
        return;

    if (lastFreq != currentWorkFrequency)
    {
        lastFreq = currentWorkFrequency;
        clearRdsBuffer2A();
        clearRdsBuffer2B();
        clearRdsBuffer0A();
    }

    waitToSend();

    rds_cmd.arg.INTACK = INTACK;
    rds_cmd.arg.MTFIFO = MTFIFO;
    rds_cmd.arg.STATUSONLY = STATUSONLY;

    Wire.beginTransmission(deviceAddress);
    Wire.write(FM_RDS_STATUS);
    Wire.write(rds_cmd.raw);
    Wire.endTransmission();

    do
    {
        waitToSend();
        // Gets response information
        Wire.requestFrom(deviceAddress, 13);
        for (uint8_t i = 0; i < 13; i++)
            currentRdsStatus.raw[i] = Wire.read();
    } while (currentRdsStatus.resp.ERR);
    delayMicroseconds(550);
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Gets RDS Status.
 * 
 * @details Same result of calling getRdsStatus(0,0,0).
 * @details Please, call getRdsStatus(uint8_t INTACK, uint8_t MTFIFO, uint8_t STATUSONLY) instead getRdsStatus() 
 * if you want other behaviour. 
 * 
 * @see SI4735::getRdsStatus(uint8_t INTACK, uint8_t MTFIFO, uint8_t STATUSONLY)
 */
void SI4735::getRdsStatus()
{
    getRdsStatus(0, 0, 0);
}

// See inlines methods / functions on SI4735.h

/**  
 * @ingroup group16 RDS status 
 * 
 * @brief Returns the programa type. 
 * 
 * @details Read the Block A content
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 77 and 78
 * 
 * @return BLOCKAL
 */
uint16_t SI4735::getRdsPI(void)
{
    if (getRdsReceived() && getRdsNewBlockA())
    {
        return currentRdsStatus.resp.BLOCKAL;
    }
    return 0;
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Returns the Group Type (extracted from the Block B)
 * 
 * @return BLOCKBL 
 */
uint8_t SI4735::getRdsGroupType(void)
{
    si47x_rds_blockb blkb;

    blkb.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
    blkb.raw.highValue = currentRdsStatus.resp.BLOCKBH;

    return blkb.refined.groupType;
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Returns the current Text Flag A/B  
 * 
 * @return uint8_t current Text Flag A/B  
 */
uint8_t SI4735::getRdsFlagAB(void)
{
    si47x_rds_blockb blkb;

    blkb.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
    blkb.raw.highValue = currentRdsStatus.resp.BLOCKBH;

    return blkb.refined.textABFlag;
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Returns the address of the text segment.
 * 
 * @details 2A - Each text segment in version 2A groups consists of four characters. A messages of this group can be 
 * have up to 64 characters. 
 * @details 2B - In version 2B groups, each text segment consists of only two characters. When the current RDS status is
 *      using this version, the maximum message length will be 32 characters.
 * 
 * @return uint8_t the address of the text segment.
 */
uint8_t SI4735::getRdsTextSegmentAddress(void)
{
    si47x_rds_blockb blkb;
    blkb.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
    blkb.raw.highValue = currentRdsStatus.resp.BLOCKBH;

    return blkb.refined.content;
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Gets the version code (extracted from the Block B)
 * 
 * @returns  0=A or 1=B
 */
uint8_t SI4735::getRdsVersionCode(void)
{
    si47x_rds_blockb blkb;

    blkb.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
    blkb.raw.highValue = currentRdsStatus.resp.BLOCKBH;

    return blkb.refined.versionCode;
}

/**  
 * @ingroup group16 RDS status 
 * 
 * @brief Returns the Program Type (extracted from the Block B)
 * 
 * @see https://en.wikipedia.org/wiki/Radio_Data_System
 * 
 * @return program type (an integer betwenn 0 and 31)
 */
uint8_t SI4735::getRdsProgramType(void)
{
    si47x_rds_blockb blkb;

    blkb.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
    blkb.raw.highValue = currentRdsStatus.resp.BLOCKBH;

    return blkb.refined.programType;
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Process data received from group 2B
 * 
 * @param c  char array reference to the "group 2B" text 
 */
void SI4735::getNext2Block(char *c)
{
    char raw[2];
    int i, j;

    raw[1] = currentRdsStatus.resp.BLOCKDL;
    raw[0] = currentRdsStatus.resp.BLOCKDH;

    for (i = j = 0; i < 2; i++)
    {
        if (raw[i] == 0xD || raw[i] == 0xA)
        {
            c[j] = '\0';
            return;
        }
        if (raw[i] >= 32)
        {
            c[j] = raw[i];
            j++;
        }
        else
        {
            c[i] = ' ';
        }
    }
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Process data received from group 2A
 * 
 * @param c  char array reference to the "group  2A" text 
 */
void SI4735::getNext4Block(char *c)
{
    char raw[4];
    int i, j;

    raw[0] = currentRdsStatus.resp.BLOCKCH;
    raw[1] = currentRdsStatus.resp.BLOCKCL;
    raw[2] = currentRdsStatus.resp.BLOCKDH;
    raw[3] = currentRdsStatus.resp.BLOCKDL;
    for (i = j = 0; i < 4; i++)
    {
        if (raw[i] == 0xD || raw[i] == 0xA)
        {
            c[j] = '\0';
            return;
        }
        if (raw[i] >= 32)
        {
            c[j] = raw[i];
            j++;
        }
        else
        {
            c[i] = ' ';
        }
    }
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Gets the RDS Text when the message is of the Group Type 2 version A
 * 
 * @return char*  The string (char array) with the content (Text) received from group 2A 
 */
char *SI4735::getRdsText(void)
{

    // Needs to get the "Text segment address code".
    // Each message should be ended by the code 0D (Hex)

    if (rdsTextAdress2A >= 16)
        rdsTextAdress2A = 0;

    getNext4Block(&rds_buffer2A[rdsTextAdress2A * 4]);

    rdsTextAdress2A += 4;

    return rds_buffer2A;
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Gets the station name and other messages. 
 * 
 * @return char* should return a string with the station name. 
 *         However, some stations send other kind of messages
 */
char *SI4735::getRdsText0A(void)
{
    si47x_rds_blockb blkB;

    // getRdsStatus();

    if (getRdsReceived())
    {
        if (getRdsGroupType() == 0)
        {
            // Process group type 0
            blkB.raw.highValue = currentRdsStatus.resp.BLOCKBH;
            blkB.raw.lowValue = currentRdsStatus.resp.BLOCKBL;

            rdsTextAdress0A = blkB.group0.address;
            if (rdsTextAdress0A >= 0 && rdsTextAdress0A < 4)
            {
                getNext2Block(&rds_buffer0A[rdsTextAdress0A * 2]);
                rds_buffer0A[8] = '\0';
                return rds_buffer0A;
            }
        }
    }
    return NULL;
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Gets the Text processed for the 2A group
 * 
 * @return char* string with the Text of the group A2  
 */
char *SI4735::getRdsText2A(void)
{
    si47x_rds_blockb blkB;

    // getRdsStatus();
    if (getRdsReceived())
    {
        if (getRdsGroupType() == 2 /* && getRdsVersionCode() == 0 */)
        {
            // Process group 2A
            // Decode B block information
            blkB.raw.highValue = currentRdsStatus.resp.BLOCKBH;
            blkB.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
            rdsTextAdress2A = blkB.group2.address;

            if (rdsTextAdress2A >= 0 && rdsTextAdress2A < 16)
            {
                getNext4Block(&rds_buffer2A[rdsTextAdress2A * 4]);
                rds_buffer2A[63] = '\0';
                return rds_buffer2A;
            }
        }
    }
    return NULL;
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Gets the Text processed for the 2B group
 * 
 * @return char* string with the Text of the group AB  
 */
char *SI4735::getRdsText2B(void)
{
    si47x_rds_blockb blkB;

    // getRdsStatus();
    // if (getRdsReceived())
    // {
    // if (getRdsNewBlockB())
    // {
    if (getRdsGroupType() == 2 /* && getRdsVersionCode() == 1 */)
    {
        // Process group 2B
        blkB.raw.highValue = currentRdsStatus.resp.BLOCKBH;
        blkB.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
        rdsTextAdress2B = blkB.group2.address;
        if (rdsTextAdress2B >= 0 && rdsTextAdress2B < 16)
        {
            getNext2Block(&rds_buffer2B[rdsTextAdress2B * 2]);
            return rds_buffer2B;
        }
    }
    //  }
    // }
    return NULL;
}

/**
 * @ingroup group16 RDS status 
 * 
 * @brief Gets the RDS time and date when the Group type is 4 
 * 
 * @return char* a string with hh:mm +/- offset
 */
char *SI4735::getRdsTime()
{
    // Under Test and construction
    // Need to check the Group Type before.
    si47x_rds_date_time dt;

    uint16_t minute;
    uint16_t hour;

    if (getRdsGroupType() == 4)
    {
        char offset_sign;
        int offset_h;
        int offset_m;

        // uint16_t y, m, d;

        dt.raw[4] = currentRdsStatus.resp.BLOCKBL;
        dt.raw[5] = currentRdsStatus.resp.BLOCKBH;
        dt.raw[2] = currentRdsStatus.resp.BLOCKCL;
        dt.raw[3] = currentRdsStatus.resp.BLOCKCH;
        dt.raw[0] = currentRdsStatus.resp.BLOCKDL;
        dt.raw[1] = currentRdsStatus.resp.BLOCKDH;

        // Unfortunately it was necessary to wotk well on the GCC compiler on 32-bit
        // platforms. See si47x_rds_date_time (typedef union) and CGG “Crosses boundary” issue/features.
        // Now it is working on Atmega328, STM32, Arduino DUE, ESP32 and more.
        minute = (dt.refined.minute2 << 2) | dt.refined.minute1;
        hour = (dt.refined.hour2 << 4) | dt.refined.hour1;

        offset_sign = (dt.refined.offset_sense == 1) ? '+' : '-';
        offset_h = (dt.refined.offset * 30) / 60;
        offset_m = (dt.refined.offset * 30) - (offset_h * 60);
        // sprintf(rds_time, "%02u:%02u %c%02u:%02u", dt.refined.hour, dt.refined.minute, offset_sign, offset_h, offset_m);
        sprintf(rds_time, "%02u:%02u %c%02u:%02u", hour, minute, offset_sign, offset_h, offset_m);

        return rds_time;
    }

    return NULL;
}

/**
 * @defgroup group17 Si4735-D60 Single Side Band (SSB) support
 * 
 * @brief Single Side Band (SSB) implementation.<br>  
 * First of all, it is important to say that the SSB patch content **is not part of this library**. 
 * The paches used here were made available by Mr. Vadim Afonkin on his [Dropbox repository](https://www.dropbox.com/sh/xzofrl8rfaaqh59/AAA5au2_CVdi50NBtt0IivyIa?dl=0). 
 * It is important to note that the author of this library does not encourage anyone to use the SSB patches content for commercial purposes.
 * In other words, this library only supports SSB patches, the patches themselves are not part of this library.  
 * 
 * @details This implementation was tested only on Si4735-D60 device. 
 * @details SSB modulation is a refinement of amplitude modulation that one of the side band and the carrier are suppressed.
 * 
 * @details What does SSB patch means?
 * In this context, a patch is a piece of software used to change the behavior of the SI4735 device.
 * There is little information available about patching the SI4735.
 *  
 * The following information is the understanding of the author of this project and 
 * it is not necessarily correct. 
 * 
 * A patch is executed internally (run by internal MCU) of the device. Usually, 
 * patches are used to fixes bugs or add improvements and new features of the firmware
 * installed in the internal ROM of the device. Patches to the SI4735 are distributed
 * in binary form and have to be transferred to the internal RAM of the device by the 
 * host MCU (in this case Arduino boards). 
 * Since the RAM is volatile memory, the patch stored into the device gets lost when 
 * you turn off the system. Consequently, the content of the patch has to be transferred
 * again to the device each time after turn on the system or reset the device.
 * 
 * I would like to thank Mr Vadim Afonkin for making available the SSBRX patches for 
 * SI4735-D60 on his Dropbox repository. On this repository you have two files, 
 * amrx_6_0_1_ssbrx_patch_full_0x9D29.csg and amrx_6_0_1_ssbrx_patch_init_0xA902.csg. 
 * It is important to know that the patch content of the original files is constant 
 * hexadecimal representation used by the language C/C++. Actally, the original files 
 * are in ASCII format (not in binary format). 
 * If you are not using C/C++ or if you want to load the files directly to the SI4735, 
 * you must convert the values to numeric value of the hexadecimal constants. 
 * For example: 0x15 = 21 (00010101); 0x16 = 22 (00010110); 0x01 = 1 (00000001); 
 * 0xFF = 255 (11111111);
 * 
 * @details ATTENTION: The author of this project does not guarantee that procedures shown 
 * here will work in your development environment. Given this, it is at your own risk 
 * to continue with the procedures suggested here. This library works with the I²C 
 * communication protocol and it is designed to apply a SSB extension PATCH to CI 
 * SI4735-D60. Once again, the author disclaims any liability for any damage this 
 * procedure may cause to your SI4735 or other devices that you are using.
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; pages 3 and 5 
 */

/**
 * @ingroup group17 Patch and SSB support 
 *  
 * @brief Sets the SSB Beat Frequency Offset (BFO). 
 * 
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; pages 5 and 23
 * 
 * @param offset 16-bit signed value (unit in Hz). The valid range is -16383 to +16383 Hz. 
 */
void SI4735::setSSBBfo(int offset)
{

    si47x_property property;
    si47x_frequency bfo_offset;

    if (currentTune == FM_TUNE_FREQ) // Only for AM/SSB mode
        return;

    waitToSend();

    property.value = SSB_BFO;
    bfo_offset.value = offset;

    Wire.beginTransmission(deviceAddress);
    Wire.write(SET_PROPERTY);
    Wire.write(0x00);                  // Always 0x00
    Wire.write(property.raw.byteHigh); // High byte first
    Wire.write(property.raw.byteLow);  // Low byte after
    Wire.write(bfo_offset.raw.FREQH);  // Offset freq. high byte first
    Wire.write(bfo_offset.raw.FREQL);  // Offset freq. low byte first

    Wire.endTransmission();
    delayMicroseconds(550);
}

/**
 * @ingroup group17 Patch and SSB support
 * 
 * @brief Sets the SSB receiver mode.
 * 
 * @details You can use this method for:  
 * @details 1) Enable or disable AFC track to carrier function for receiving normal AM signals;
 * @details 2) Set the audio bandwidth;
 * @details 3) Set the side band cutoff filter;
 * @details 4) Set soft-mute based on RSSI or SNR;
 * @details 5) Enable or disbable automatic volume control (AVC) function. 
 * 
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; page 24 
 * 
 * @param AUDIOBW SSB Audio bandwidth; 0 = 1.2KHz (default); 1=2.2KHz; 2=3KHz; 3=4KHz; 4=500Hz; 5=1KHz.
 * @param SBCUTFLT SSB side band cutoff filter for band passand low pass filter
 *                 if 0, the band pass filter to cutoff both the unwanted side band and high frequency 
 *                  component > 2KHz of the wanted side band (default).
 * @param AVC_DIVIDER set 0 for SSB mode; set 3 for SYNC mode.
 * @param AVCEN SSB Automatic Volume Control (AVC) enable; 0=disable; 1=enable (default).
 * @param SMUTESEL SSB Soft-mute Based on RSSI or SNR.
 * @param DSP_AFCDIS DSP AFC Disable or enable; 0=SYNC MODE, AFC enable; 1=SSB MODE, AFC disable. 
 */
void SI4735::setSSBConfig(uint8_t AUDIOBW, uint8_t SBCUTFLT, uint8_t AVC_DIVIDER, uint8_t AVCEN, uint8_t SMUTESEL, uint8_t DSP_AFCDIS)
{
    if (currentTune == FM_TUNE_FREQ) // Only AM/SSB mode
        return;

    currentSSBMode.param.AUDIOBW = AUDIOBW;
    currentSSBMode.param.SBCUTFLT = SBCUTFLT;
    currentSSBMode.param.AVC_DIVIDER = AVC_DIVIDER;
    currentSSBMode.param.AVCEN = AVCEN;
    currentSSBMode.param.SMUTESEL = SMUTESEL;
    currentSSBMode.param.DUMMY1 = 0;
    currentSSBMode.param.DSP_AFCDIS = DSP_AFCDIS;

    sendSSBModeProperty();
}

/**
 * @ingroup group17 Patch and SSB support
 * 
 * @brief Sets DSP AFC disable or enable
 * 
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; page 24 
 * 
 * @param DSP_AFCDIS 0 = SYNC mode, AFC enable; 1 = SSB mode, AFC disable
 */
void SI4735::setSSBDspAfc(uint8_t DSP_AFCDIS)
{
    currentSSBMode.param.DSP_AFCDIS = DSP_AFCDIS;
    sendSSBModeProperty();
}

/**
 * @ingroup group17 Patch and SSB support 
 * 
 * @brief Sets SSB Soft-mute Based on RSSI or SNR Selection: 
 * 
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; page 24 
 * 
 * @param SMUTESEL  0 = Soft-mute based on RSSI (default); 1 = Soft-mute based on SNR.
 */
void SI4735::setSSBSoftMute(uint8_t SMUTESEL)
{
    currentSSBMode.param.SMUTESEL = SMUTESEL;
    sendSSBModeProperty();
}

/**
 * @ingroup group17 Patch and SSB support
 * 
 * @brief Sets SSB Automatic Volume Control (AVC) for SSB mode
 * 
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; page 24 
 * 
 * @param AVCEN 0 = Disable AVC; 1 = Enable AVC (default).
 */
void SI4735::setSSBAutomaticVolumeControl(uint8_t AVCEN)
{
    currentSSBMode.param.AVCEN = AVCEN;
    sendSSBModeProperty();
}

/**
 * @ingroup group17 Patch and SSB support
 * 
 * @brief Sets AVC Divider
 * 
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; page 24  
 * 
 * @param AVC_DIVIDER  SSB mode, set divider = 0; SYNC mode, set divider = 3; Other values = not allowed.
 */
void SI4735::setSSBAvcDivider(uint8_t AVC_DIVIDER)
{
    currentSSBMode.param.AVC_DIVIDER = AVC_DIVIDER;
    sendSSBModeProperty();
}

/**  
 * @ingroup group17 Patch and SSB support
 * 
 * @brief Sets SBB Sideband Cutoff Filter for band pass and low pass filters.
 * 
 * @details 0 = Band pass filter to cutoff both the unwanted side band and high frequency components > 2.0 kHz of the wanted side band. (default)
 * @details 1 = Low pass filter to cutoff the unwanted side band. 
 * Other values = not allowed.
 *
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; page 24 
 *  
 * @param SBCUTFLT 0 or 1; see above
 */
void SI4735::setSBBSidebandCutoffFilter(uint8_t SBCUTFLT)
{
    currentSSBMode.param.SBCUTFLT = SBCUTFLT;
    sendSSBModeProperty();
}

/**
 * @ingroup group17 Patch and SSB support
 * 
 * @brief SSB Audio Bandwidth for SSB mode
 * 
 * @details 0 = 1.2 kHz low-pass filter  (default).
 * @details 1 = 2.2 kHz low-pass filter.
 * @details 2 = 3.0 kHz low-pass filter.
 * @details 3 = 4.0 kHz low-pass filter.
 * @details 4 = 500 Hz band-pass filter for receiving CW signal, i.e. [250 Hz, 750 Hz] with center 
 * frequency at 500 Hz when USB is selected or [-250 Hz, -750 1Hz] with center frequency at -500Hz 
 * when LSB is selected* .
 * @details 5 = 1 kHz band-pass filter for receiving CW signal, i.e. [500 Hz, 1500 Hz] with center 
 * frequency at 1 kHz when USB is selected or [-500 Hz, -1500 1 Hz] with center frequency 
 *     at -1kHz when LSB is selected.
 * @details Other values = reserved.
 * 
 * @details If audio bandwidth selected is about 2 kHz or below, it is recommended to set SBCUTFLT[3:0] to 0 
 * to enable the band pass filter for better high- cut performance on the wanted side band. Otherwise, set it to 1.
 * 
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; page 24 
 * 
 * @param AUDIOBW the valid values are 0, 1, 2, 3, 4 or 5; see description above
 */
void SI4735::setSSBAudioBandwidth(uint8_t AUDIOBW)
{
    // Sets the audio filter property parameter
    currentSSBMode.param.AUDIOBW = AUDIOBW;
    sendSSBModeProperty();
}

/**
 * @ingroup group17 Patch and SSB support
 * 
 * @brief Set the radio to AM function. 
 * 
 * @details It means: LW MW and SW.
 * 
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; pages 13 and 14 
 * @see setAM()
 * @see void SI4735::setFrequency(uint16_t freq)
 * 
 * @param usblsb upper or lower side band;  1 = LSB; 2 = USB
 */
void SI4735::setSSB(uint8_t usblsb)
{
    // Is it needed to load patch when switch to SSB?
    // powerDown();
    // It starts with the same AM parameters.
    setPowerUp(1, 1, 0, 1, 1, SI473X_ANALOG_AUDIO);
    radioPowerUp();
    // ssbPowerUp(); // Not used for regular operation
    setVolume(volume); // Set to previus configured volume
    currentSsbStatus = usblsb;
    lastMode = SSB_CURRENT_MODE;
}

/**
 * @ingroup group17 Patch and SSB support
 *  
 * @details Set the radio to SSB (LW/MW/SW) function. 
 * 
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE; pages 13 and 14
 * 
 * @param fromFreq minimum frequency for the band
 * @param toFreq maximum frequency for the band
 * @param initialFreq initial frequency 
 * @param step step used to go to the next channel  
 * @param usblsb SSB Upper Side Band (USB) and Lower Side Band (LSB) Selection; 
 *               value 2 (banary 10) = USB; 
 *               value 1 (banary 01) = LSB.   
 */
void SI4735::setSSB(uint16_t fromFreq, uint16_t toFreq, uint16_t initialFreq, uint16_t step, uint8_t usblsb)
{
    currentMinimumFrequency = fromFreq;
    currentMaximumFrequency = toFreq;
    currentStep = step;

    if (initialFreq < fromFreq || initialFreq > toFreq)
        initialFreq = fromFreq;

    setSSB(usblsb);

    currentWorkFrequency = initialFreq;
    setFrequency(currentWorkFrequency);
    delayMicroseconds(550);
}

/**  
 * @ingroup group17 Patch and SSB support
 * 
 * @brief Just send the property SSB_MOD to the device.  Internal use (privete method). 
 */
void SI4735::sendSSBModeProperty()
{
    si47x_property property;
    property.value = SSB_MODE;
    waitToSend();
    Wire.beginTransmission(deviceAddress);
    Wire.write(SET_PROPERTY);
    Wire.write(0x00);                  // Always 0x00
    Wire.write(property.raw.byteHigh); // High byte first
    Wire.write(property.raw.byteLow);  // Low byte after
    Wire.write(currentSSBMode.raw[1]); // SSB MODE params; freq. high byte first
    Wire.write(currentSSBMode.raw[0]); // SSB MODE params; freq. low byte after

    Wire.endTransmission();
    delayMicroseconds(550);
}

/***************************************************************************************
 * SI47XX PATCH RESOURCES
 **************************************************************************************/

/** 
 * @ingroup group17 Patch and SSB support
 * 
 * @brief Query the library information of the Si47XX device 
 * 
 * @details Used to confirm if the patch is compatible with the internal device library revision.
 * 
 * @details You have to call this function if you are applying a patch on SI47XX (SI4735-D60).
 * @details The first command that is sent to the device is the POWER_UP command to confirm 
 * that the patch is compatible with the internal device library revision. 
 * @details The device moves into the powerup mode, returns the reply, and moves into the 
 * powerdown mode. 
 * @details The POWER_UP command is sent to the device again to configure 
 * the mode of the device and additionally is used to start the patching process.
 * @details When applying the patch, the PATCH bit in ARG1 of the POWER_UP command must be 
 * set to 1 to begin the patching process. [AN332 page 219].
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 64 and 215-220.
 * @see struct si47x_firmware_query_library
 *
 * @return a struct si47x_firmware_query_library (see it in SI4735.h)
*/
si47x_firmware_query_library SI4735::queryLibraryId()
{
    si47x_firmware_query_library libraryID;

    powerDown(); // Is it necessary

    // delay(500);

    waitToSend();
    Wire.beginTransmission(deviceAddress);
    Wire.write(POWER_UP);
    Wire.write(0b00011111);          // Set to Read Library ID, disable interrupt; disable GPO2OEN; boot normaly; enable External Crystal Oscillator  .
    Wire.write(SI473X_ANALOG_AUDIO); // Set to Analog Line Input.
    Wire.endTransmission();

    do
    {
        waitToSend();
        Wire.requestFrom(deviceAddress, 8);
        for (int i = 0; i < 8; i++)
            libraryID.raw[i] = Wire.read();
    } while (libraryID.resp.ERR); // If error found, try it again.

    delayMicroseconds(2500);

    return libraryID;
}

/**  
 * @ingroup group17 Patch and SSB support
 *  
 * @brief This method can be used to prepare the device to apply SSBRX patch
 *  
 * @details Call queryLibraryId before call this method. Powerup the device by issuing the POWER_UP 
 * command with FUNC = 1 (AM/SW/LW Receive).
 *  
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 64 and 215-220 and
 * @see AN332 REV 0.8 UNIVERSAL PROGRAMMING GUIDE AMENDMENT FOR SI4735-D60 SSB AND NBFM PATCHES; page 7.
 */
void SI4735::patchPowerUp()
{
    waitToSend();
    Wire.beginTransmission(deviceAddress);
    Wire.write(POWER_UP);
    Wire.write(0b00110001);          // Set to AM, Enable External Crystal Oscillator; Set patch enable; GPO2 output disabled; CTS interrupt disabled.
    Wire.write(SI473X_ANALOG_AUDIO); // Set to Analog Output
    Wire.endTransmission();
    delayMicroseconds(2500);
}

/** 
 * @ingroup group17 Patch and SSB support
 * 
 * @brief Starts the Si473X device on SSB (same AM Mode). 
 * 
 * @details Same SI4735::setup optimized to improve loading patch performance 
 */
void SI4735::ssbSetup()
{
    // setPowerUp(powerUp.arg.CTSIEN, 0, 0, 1, 1, SI473X_ANALOG_AUDIO);
    reset();
    // radioPowerUp();
}

/**
 * @ingroup group17 Patch and SSB support
 * 
 * @brief This function can be useful for debug and test. 
 */
void SI4735::ssbPowerUp()
{
    waitToSend();
    Wire.beginTransmission(deviceAddress);
    Wire.write(POWER_UP);
    Wire.write(0b00010001); // Set to AM/SSB, disable interrupt; disable GPO2OEN; boot normaly; enable External Crystal Oscillator  .
    Wire.write(0b00000101); // Set to Analog Line Input.
    Wire.endTransmission();
    delayMicroseconds(2500);

    powerUp.arg.CTSIEN = 0;          // 1 -> Interrupt anabled;
    powerUp.arg.GPO2OEN = 0;         // 1 -> GPO2 Output Enable;
    powerUp.arg.PATCH = 0;           // 0 -> Boot normally;
    powerUp.arg.XOSCEN = 1;          // 1 -> Use external crystal oscillator;
    powerUp.arg.FUNC = 1;            // 0 = FM Receive; 1 = AM/SSB (LW/MW/SW) Receiver.
    powerUp.arg.OPMODE = 0b00000101; // 0x5 = 00000101 = Analog audio outputs (LOUT/ROUT).
}

/**
 * @ingroup group17 Patch and SSB support
 *  
 * @brief Transfers the content of a patch stored in a array of bytes to the SI4735 device. 
 *  
 * @details You must mount an array as shown below and know the size of that array as well.
 * 
 *  @details It is importante to say  that patches to the SI4735 are distributed in binary form and 
 *  have to be transferred to the internal RAM of the device by the host MCU (in this case Arduino).
 *  Since the RAM is volatile memory, the patch stored into the device gets lost when you turn off 
 *  the system. Consequently, the content of the patch has to be transferred again to the device 
 *  each time after turn on the system or reset the device.
 * 
 *  @details The disadvantage of this approach is the amount of memory used by the patch content. 
 *  This may limit the use of other radio functions you want implemented in Arduino.
 * 
 *  @details Example of content:
 *  @code
 *  const PROGMEM uint8_t ssb_patch_content_full[] =
 *   { // SSB patch for whole SSBRX full download
 *       0x15, 0x00, 0x0F, 0xE0, 0xF2, 0x73, 0x76, 0x2F,
 *       0x16, 0x6F, 0x26, 0x1E, 0x00, 0x4B, 0x2C, 0x58,
 *       0x16, 0xA3, 0x74, 0x0F, 0xE0, 0x4C, 0x36, 0xE4,
 *          .
 *          .
 *          .
 *       0x16, 0x3B, 0x1D, 0x4A, 0xEC, 0x36, 0x28, 0xB7,
 *       0x16, 0x00, 0x3A, 0x47, 0x37, 0x00, 0x00, 0x00,
 *       0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9D, 0x29};   
 * 
 *  const int size_content_full = sizeof ssb_patch_content_full;
 *  @endcode
 * 
 * @see Si47XX PROGRAMMING GUIDE; AN332; pages 64 and 215-220.  
 * 
 *  @param ssb_patch_content point to array of bytes content patch.
 *  @param ssb_patch_content_size array size (number of bytes). The maximum size allowed for a patch is 15856 bytes
 * 
 *  @return false if an error is found.
 */
bool SI4735::downloadPatch(const uint8_t *ssb_patch_content, const uint16_t ssb_patch_content_size)
{
    uint8_t content;
    register int i, offset;
    // Send patch to the SI4735 device
    for (offset = 0; offset < (int) ssb_patch_content_size; offset += 8)
    {
        Wire.beginTransmission(deviceAddress);
        for (i = 0; i < 8; i++)
        {
            content = pgm_read_byte_near(ssb_patch_content + (i + offset));
            Wire.write(content);
        }
        Wire.endTransmission();

        // Testing download performance
        // approach 1 - Faster - less secure (it might crash in some architectures)
        delayMicroseconds(MIN_DELAY_WAIT_SEND_LOOP); // Need check the minimum value

        // approach 2 - More control. A little more secure than approach 1
        /*
        do
        {
            delayMicroseconds(150); // Minimum delay founded (Need check the minimum value)
            Wire.requestFrom(deviceAddress, 1);
        } while (!(Wire.read() & B10000000));
        */

        // approach 3 - same approach 2
        // waitToSend();

        // approach 4 - safer
        /*
        waitToSend();
        uint8_t cmd_status;
        Uncomment the lines below if you want to check erro.
        Wire.requestFrom(deviceAddress, 1);
        cmd_status = Wire.read();
        The SI4735 issues a status after each 8 byte transfered.
        Just the bit 7 (CTS) should be seted. if bit 6 (ERR) is seted, the system halts.
        if (cmd_status != 0x80)
           return false;
        */
    }
    delayMicroseconds(250);
    return true;
}

/**
 * @ingroup group17 Patch and SSB support
 *  
 * @brief Transfers the content of a patch stored in a eeprom to the SI4735 device.
 * 
 * @details TO USE THIS METHOD YOU HAVE TO HAVE A EEPROM WRITEN WITH THE PATCH CONTENT
 * 
 * ATTENTION: Under construction...
 * 
 * @see the sketch write_ssb_patch_eeprom.ino (TO DO)
 * 
 * @param eeprom_i2c_address 
 * @return false if an error is found.
 */
bool SI4735::downloadPatch(int eeprom_i2c_address)
{
    int ssb_patch_content_size;
    uint8_t cmd_status;
    int i, offset;
    uint8_t eepromPage[8];

    union {
        struct
        {
            uint8_t lowByte;
            uint8_t highByte;
        } raw;
        uint16_t value;
    } eeprom;

    // The first two bytes are the size of the patches
    // Set the position in the eeprom to read the size of the patch content
    Wire.beginTransmission(eeprom_i2c_address);
    Wire.write(0); // writes the most significant byte
    Wire.write(0); // writes the less significant byte
    Wire.endTransmission();
    Wire.requestFrom(eeprom_i2c_address, 2);
    eeprom.raw.highByte = Wire.read();
    eeprom.raw.lowByte = Wire.read();

    ssb_patch_content_size = eeprom.value;

    // the patch content starts on position 2 (the first two bytes are the size of the patch)
    for (offset = 2; offset < ssb_patch_content_size; offset += 8)
    {
        // Set the position in the eeprom to read next 8 bytes
        eeprom.value = offset;
        Wire.beginTransmission(eeprom_i2c_address);
        Wire.write(eeprom.raw.highByte); // writes the most significant byte
        Wire.write(eeprom.raw.lowByte);  // writes the less significant byte
        Wire.endTransmission();

        // Reads the next 8 bytes from eeprom
        Wire.requestFrom(eeprom_i2c_address, 8);
        for (i = 0; i < 8; i++)
            eepromPage[i] = Wire.read();

        // sends the page (8 bytes) to the SI4735
        Wire.beginTransmission(deviceAddress);
        for (i = 0; i < 8; i++)
            Wire.write(eepromPage[i]);
        Wire.endTransmission();

        waitToSend();

        Wire.requestFrom(deviceAddress, 1);
        cmd_status = Wire.read();
        // The SI4735 issues a status after each 8 byte transfered.
        // Just the bit 7 (CTS) should be seted. if bit 6 (ERR) is seted, the system halts.
        if (cmd_status != 0x80)
            return false;
    }
    delayMicroseconds(250);
    return true;
}
