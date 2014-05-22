//*************************************************************************************
/** \file hmc6352.h
 *    This file contains a driver class for a Honeywell HMC6352 compass chip. 
 *
 *  Revised:
 *    \li 12-24-2012 JRR Original file
 *
 *  License:
 *    This file is copyright 2012 by JR Ridgely and released under the Lesser GNU 
 *    Public License, version 2. It is intended for educational use only, but its use
 *    is not limited thereto. */
/*    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 *    TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 *    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 *    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 *    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
//*************************************************************************************

// This define prevents this file from being included more than once in a *.cpp file
#ifndef _HMC6352_H_
#define _HMC6352_H_

#include <stdlib.h>                         // Standard C/C++ library stuff
#include "i2c_master.h"                     // Header for I2C (AKA TWI) bus driver
#include "emstream.h"                       // Header for base serial devices


//-------------------------------------------------------------------------------------
/** This class is a driver for a Honeywell HMC6352 compass. It's very basic and does
 *  not use all the HMC6352's functions; it is set up only to put the compass in 
 *  standby mode and take a heading measurement when asked for, or optionally to put
 *  the compass into continuous measurement mode (in which the compass measures its
 *  heading once every second) and then read out the previous heading measurement
 *  quickly on demand. 
 * 
 *  \section use_hmc6352 Usage
 *  To use this driver, one need only create the driver object and then ask it for a
 *  heading now and then.
 *  \code
 *  hmc6352* p_honey = new hmc6352 (p_serial);
 *  ...
 *  *p_serial << PMS ("HMC6352: ") << p_honey->get_heading () << endl;
 *  \endcode
 */

class hmc6352 : public i2c_master
{
protected:

public:
	// This constructor sets up the driver
	hmc6352 (emstream* = NULL);

	// This destructor doesn't exist...psych

	// This method reads the current heading
	uint16_t heading (void);

	/** This method puts the HMC6352 in continuous mode, which means that once per
	 *  second, the compass takes a reading and updates its output register. The
	 *  heading can then be read quickly, as it will be 16 bits just waiting to be
	 *  read. The mode is not stored in EEPROM, so unless a command to store the
	 *  mode is called elsewhere, the mode will be reset to standby mode when the 
	 *  power is next turned off and on. 
	 */
	void continuous_mode (void)
	{
		set_mode (0x12);
	}

	/** This method puts the HMC6352 in standby mode, where it's pretty much asleep.
	 *  If one wants to find the heading, one must send an 'A' command to force a
	 *  measurement. Standby mode is the factory default mode at startup.
	 */
	void standby_mode (void)
	{
		set_mode (0x50);
	}

	/*  This method sets the mode byte of the HMC6352. The mode is only set in RAM
	 *  unless saved in EEPROM elsewhere.
	 */
	void set_mode (uint8_t);
};

// This operator "prints" a SHT15 sensor by showing its current measured outputs
emstream& operator << (emstream&, hmc6352&);

#endif // _HMC6352_H_

