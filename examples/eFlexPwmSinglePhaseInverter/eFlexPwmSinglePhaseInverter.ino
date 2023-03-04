/*
  eFlexPwm Single Phase Inverter Example

  This example generates the signals used to drive a single phase full-bridge inverter.
  These are 2 pairs of SPWM signals (Sinusoidal Pulse Width Modulation)
  Each signal pair corresponds to the PWMA output and the PWMB (A's complement)
  output of an eFlexPWM submodule.
  In this example, sub-modules 0 and 2 of PWM2 are used. On the Teensy 4.1 board,
  this corresponds to the following pins:
  | PWM | SubModule | Pol | Teensy | Native |
  |-----|-----------|-----|--------|--------|
  | 2   | 0         | A   | 4      | EMC_06 |
  | 2   | 0         | B   | 33     | EMC_07 |
  | 2   | 2         | A   | 6      | B0_10  |
  | 2   | 2         | B   | 9      | B0_11  |
  The PWM frequency is fixed pwmFreq, here at 20 kHz
  The SPWM frequency changes from 5 to 50 Hz in 1Hz steps (every 5 seconds).

  This example displays a message on the Serial link (USB CDC), and any error messages:

  eFlexPwm Single Phase Inverter Example
  Submodules successfuly started
*/
#include <Arduino.h>
#include <arm_math.h>
#include <util/atomic.h>
#include <eFlexPwm.h>

//   Definitions
// ----------------------------------------------------------------------------
// avoid systematically prefixing objects with the namespace
using namespace eFlex;

//   Variables
// ----------------------------------------------------------------------------
const float32_t MidDutyCycle = 32768;
const float32_t SineFreqMin = 5.0;
const float32_t SineFreqMax = 50.0;
const float32_t SineFreqStep = 1.0;
const float32_t DeadTimeNs = 500.0;
const uint32_t StepDelay = 5000; // Increments sineFreq by SineFreqStep every StepDelay ms.

// PWM frequency in hz
float32_t pwmFreq = 20000.0;
// Sine Frequency in Hz
float32_t sineFreq = SineFreqMax / 2;

// Interrupt Input Variables
volatile uint32_t vSample;
volatile float32_t vSpeed;

// My eFlexPWM submodules (Hardware > PWM2: SM[0], SM[2])
SubModule Sm20 (4, 33);
SubModule Sm22 (6, 9);

/*
  all the sub-modules are part of the same timer: PWM2,
  Tm2 simplifies access to the functions that concern all the sub-modules
*/
Timer &Tm2 = Sm20.timer();

//   Code
// ----------------------------------------------------------------------------
// Interrupt Service Routine
void IsrOverflowSm20() {
  float32_t s = MidDutyCycle * arm_sin_f32 (vSpeed * ++vSample);

  Sm20.updateDutyCycle (static_cast<uint16_t> (MidDutyCycle + s));
  Sm22.updateDutyCycle (static_cast<uint16_t> (MidDutyCycle - s));

  Sm20.clearStatusFlags (kPWM_CompareVal1Flag);
  Tm2.setPwmLdok();
}

void updateSpeed() {

  float32_t s = 2.0 * PI * (sineFreq / pwmFreq);
  ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
    vSpeed = s;
  }
}

void setup() {
  // initialize LED digital pin as an output.
  pinMode (LED_BUILTIN, OUTPUT);

  digitalWrite (LED_BUILTIN, HIGH);
  Serial.println ("eFlexPwm Single Phase Inverter Example");

  // start of PWM configuration -----------------------------------------------
  /* Submodule configuration, default values are:

    config->debugModeEnabled = false;
    config->enableWait = false;
    config->reloadSelect = kPWM_LocalReload;
    config->clockSource = kPWM_BusClock;
    config->prescale = kPWM_Prescale_Divide_1;
    config->initializationControl = kPWM_Initialize_LocalSync;
    config->forceTrigger = kPWM_Force_Local;
    config->reloadFrequency = kPWM_LoadEveryOportunity;
    config->reloadLogic = kPWM_ReloadImmediate;
    config->pairOperation = kPWM_Independent;
    config->pwmFreqHz = 5000;
    config->mode = kPWM_CenterAligned;
  */
  Config myConfig;

  /* Use full cycle reload */
  myConfig.setReloadLogic (kPWM_ReloadPwmFullCycle);
  /* PWM A & PWM B form a complementary PWM pair */
  myConfig.setPairOperation (kPWM_ComplementaryPwmA);
  myConfig.setPwmFreqHz (pwmFreq);

  /* Initialize submodule 0 */
  if (Sm20.configure (myConfig) != true) {
    Serial.println ("Submodule 0 initialization failed");
    exit (EXIT_FAILURE);
  }

  /* Initialize submodule 2, make it use same counter clock as submodule 0. */
  myConfig.setClockSource (kPWM_Submodule0Clock);
  myConfig.setPrescale (kPWM_Prescale_Divide_1);
  myConfig.setInitializationControl (kPWM_Initialize_MasterSync);

  if (Sm22.configure (myConfig) != true) {
    Serial.println ("Submodule 2 initialization failed");
    exit (EXIT_FAILURE);
  }

  /*
    Set deadtime count, we set this to about 1000ns for all submodules pins
    You can also, modify it for all the pins of a sub-module, eg. :

      Sm20.setupDeadtime (deadTimeVal);

    or even for a particular pin, eg. :

      Sm20.setupDeadtime (deadTimeVal, ChanA);
  */
  uint16_t deadTimeVal = ( (uint64_t) Tm2.srcClockHz() * DeadTimeNs) / 1000000000;
  Tm2.setupDeadtime (deadTimeVal);


  // Start all submodules
  if (Tm2.begin() != true) {
    Serial.println ("Failed to start submodules");
    exit (EXIT_FAILURE);
  }

  // synchronize registers and start all submodules
  if (Tm2.begin() != true) {
    Serial.println ("Failed to start submodules");
    exit (EXIT_FAILURE);
  }
  else {

    Serial.println ("Submodules successfuly started");
  }

  updateSpeed(); 

  // Enable counter overflow interrupt (VAL1)
  Sm20.enableInterrupts (kPWM_CompareVal1InterruptEnable);
  attachInterruptVector (IRQ_FLEXPWM2_0, &IsrOverflowSm20);
  NVIC_ENABLE_IRQ (IRQ_FLEXPWM2_0);

  // end of PWM setup
  digitalWrite (LED_BUILTIN, LOW);
}

void loop() {

  delay (StepDelay);
  sineFreq += SineFreqStep;

  if (sineFreq > SineFreqMax) {

    sineFreq = SineFreqMin;
  }

  updateSpeed();
  // Toggle LED state (HIGH is the voltage level)
  digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN));
}