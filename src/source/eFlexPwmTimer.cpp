/*
   Copyright (c) 2023, epsilonrt
   All rights reserved.

   SPDX-License-Identifier: BSD-3-Clause
*/
#include "component/eFlexPwmTimer.h"

extern "C" void xbar_connect (unsigned int input, unsigned int output);

namespace eFlex {

  //-----------------------------------------------------------------------------
  //                                Timer class
  //-----------------------------------------------------------------------------

  //-----------------------------------------------------------------------------
  Timer::Timer (uint8_t index) :
    m_tmidx (index), m_ptr (PWM[index]), m_isenabled (true) {
  }

  //-----------------------------------------------------------------------------
  bool Timer::begin (bool doStart, bool doSync) {
    bool success = true;

    if (doStart) {

      stop();
    }

    if (doSync) {

      setPwmLdok (false);
    }

    for (uint8_t i = 0; (i < NofSubmodules) && success; i++) {

      SubModule *s = SmList[m_tmidx][i];
      if (s) {

        success &= s ->begin (false, false);
      }
    }

    if (doSync) {

      setPwmLdok (true);
    }
    if (doStart && success) {

      start();
    }
    return success;
  }

  //-----------------------------------------------------------------------------
  void Timer::enable (bool value) {

    for (uint8_t i = 0; i < NofSubmodules; i++) {

      SubModule *s = SmList[m_tmidx][i];
      if (s) {
        s->enable (value);
      }
    }

    m_isenabled = value;
  }


  // ----------------------------------------------------------------------------
  void Timer::setupLevel (pwm_level_select_t level) {

    for (uint8_t i = 0; i < NofSubmodules; i++) {

      SubModule *s = SmList[m_tmidx][i];
      if (s) {

        s ->setupLevel (level);
      }
    }
  }

  // ----------------------------------------------------------------------------
  void Timer::setupDeadtime (uint16_t deadtimeValue) {

    for (uint8_t i = 0; i < NofSubmodules; i++) {

      SubModule *s = SmList[m_tmidx][i];
      if (s) {

        s ->setupDeadtime (deadtimeValue);
      }
    }
  }

  // ----------------------------------------------------------------------------
  void Timer::setupOutputEnable (bool activate) {

    for (uint8_t i = 0; i < NofSubmodules; i++) {

      SubModule *s = SmList[m_tmidx][i];
      if (s) {

        s ->setupOutputEnable (activate);
      }
    }
  }

  // ----------------------------------------------------------------------------
  void Timer::setupDutyCyclePercent (uint8_t dutyCyclePercent) {

    for (uint8_t i = 0; i < NofSubmodules; i++) {

      SubModule *s = SmList[m_tmidx][i];
      if (s) {

        s ->setupDutyCyclePercent (dutyCyclePercent);
      }
    }
  }

  // ----------------------------------------------------------------------------
  void Timer::setupFaultState (pwm_fault_state_t faultState) {

    for (uint8_t i = 0; i < NofSubmodules; i++) {

      SubModule *s = SmList[m_tmidx][i];
      if (s) {

        s ->setupFaultState (faultState);
      }
    }
  }

  //-----------------------------------------------------------------------------
  bool Timer::updateSetting (bool doSync) {
    bool success = true;

    for (uint8_t i = 0; i < NofSubmodules; i++) {

      SubModule *s = SmList[m_tmidx][i];
      if (s) {

        success &= s ->updateSetting (doSync);
      }
    }
    return success;
  }

  const uint8_t XBarFaultOutput [NofTimers][4] = {
    {
      XBARA1_OUT_FLEXPWM1_FAULT0,
      XBARA1_OUT_FLEXPWM1_FAULT1,
      XBARA1_OUT_FLEXPWM1_FAULT2,
      XBARA1_OUT_FLEXPWM1_FAULT3
    },
    {
      XBARA1_OUT_FLEXPWM2_FAULT0,
      XBARA1_OUT_FLEXPWM2_FAULT1,
      XBARA1_OUT_FLEXPWM2_FAULT2,
      XBARA1_OUT_FLEXPWM2_FAULT3
    },
    {
      XBARA1_OUT_FLEXPWM3_FAULT0,
      XBARA1_OUT_FLEXPWM3_FAULT1,
      XBARA1_OUT_FLEXPWM3_FAULT2,
      XBARA1_OUT_FLEXPWM3_FAULT3
    },
    {

      XBARA1_OUT_FLEXPWM4_FAULT0,
      XBARA1_OUT_FLEXPWM4_FAULT1,
      XBARA1_OUT_FLEXPWM4_FAULT2,
      XBARA1_OUT_FLEXPWM4_FAULT3
    }
  };

  //-----------------------------------------------------------------------------
  void Timer::setupFaults (uint8_t faultNum, const FaultConfig &faultConfig, int faultPin) {

    if ( (faultNum & 0x03) == 0) {
      if (faultPin > 0) {

        xbar_connect (faultPin, XBarFaultOutput[m_tmidx][faultNum]);
      }
      PWM_SetupFaults (ptr(), faultNum, faultConfig.kPwmConfig());
    }
  }

  //-----------------------------------------------------------------------------
  void Timer::printRegs (Stream &out) const {
    #ifdef EFLEXPWM_DUMPREG_ENABLED
    const char *names[] = {
      "OUTEN",                             /**< Output Enable Register, offset: 0x180 */
      "MASK",                              /**< Mask Register, offset: 0x182 */
      "SWCOUT",                            /**< Software Controlled Output Register, offset: 0x184 */
      "DTSRCSEL",                          /**< PWM Source Select Register, offset: 0x186 */
      "MCTRL",                             /**< Master Control Register, offset: 0x188 */
      "MCTRL2",                            /**< Master Control 2 Register, offset: 0x18A */
      "FCTRL",                             /**< Fault Control Register, offset: 0x18C */
      "FSTS",                              /**< Fault Status Register, offset: 0x18E */
      "FFILT",                             /**< Fault Filter Register, offset: 0x190 */
      "FTST",                              /**< Fault Test Register, offset: 0x192 */
      "FCTRL2"                             /**< Fault Control 2 Register, offset: 0x194 */
    };
    __IM uint16_t *p = & (ptr()->OUTEN);
    const char **n = names;

    while (p <= & (ptr()->FCTRL2)) {

      out.printf ("%s\t: 0x%04X\n", *n++, *p++);
    }
    out.println ("");
    #endif
  }

  //-----------------------------------------------------------------------------
  void Timer::printAllRegs (Stream &out) const {
    #ifdef EFLEXPWM_DUMPREG_ENABLED

    delay (10);
    out.println ("Timer >>>>>");
    printRegs (out);
    out.println ("");

    for (uint8_t i = 0; i < NofSubmodules; i++) {

      SubModule *s = SmList[m_tmidx][i];
      if (s) {

        out.printf ("SubM %u +++++\n", i);
        s->printRegs (out);
        out.println ("");
      }
    }
    #endif
  }

  class TimerFactory : public Timer {
    public:
      /**
        @brief Construct a new Timer Module

        @param index the timer module index (0 for PWM1...)
      */
      TimerFactory (uint8_t index) : Timer (index) {}
  };

  // All timers are instantiated globally
  TimerFactory TMF[NofTimers] = {
    TimerFactory (Pwm1),
    TimerFactory (Pwm2),
    TimerFactory (Pwm3),
    TimerFactory (Pwm4)
  };

  Timer *TM[NofTimers] = {
    & TMF[Pwm1],
    & TMF[Pwm2],
    & TMF[Pwm3],
    & TMF[Pwm4]
  };

}
