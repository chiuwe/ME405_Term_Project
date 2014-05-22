//*************************************************************************************
/** \file avr_1wire.h
 *    This file contains a class which interfaces an AVR processor with devices on a 
 *    bit-banged One-Wire Interface. The one-wire interface is used by a number of
 *    chips from Dallas Semiconductor (which has merged with Maxim IC).  The term
 *    "bit-banged" means that this class uses any old I/O port pins for the data line.
 *    There can be several 1-wire interfaces in a program, in case the user wants to 
 *    talk to several devices on different buses instead of connecting all the one-wire 
 *    devices on the same bus. This can be useful to save power, as the buses can be 
 *    powered individually; also, some 1-wire devices like different timing than 
 *    others.
 *
 *  Revisions:
 *    \li 07-21-2007 JRR Created this file for Two-Wire Interfaces
 *    \li 12-17-2007 JRR Modified to work with the One-Wire Interface
 *    \li 07-11-2008 JRR Changed to use port pointers so several I/O ports can be used
 *    \li 07-12-2008 JRR Added debugging port which works with STL_DEBUG in stl_task.h
 *    \li 03-21-2009 JRR Changed debugging to the global debugging system
 *    \li 11-11-2011 JRR Cleaned up formatting, changed to .cpp
 *    \li 12-15-2011 JRR Changed read delay from AOWI_45us_D to AOWI_15us_D and fixed
 *                       a timing bug in avr_1wire::read_bit()
 *    \li 12-01-2012 JRR Changed to work in new FreeRTOS based ME405 environment,
 *                       made timing fully F_CPU dependent
 *
 *  License:
 *    This file copyright 2007-2012 by JR Ridgely. It is released under the Lesser GNU
 *    public license, version 2. It is intended for educational use only, but it may
 *    be used for any purpose permitted under the LGPL. The author has no control 
 *    over the uses of this software and cannot be responsible for any consequences 
 *    of such use. */
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

/// This define prevents this file from being included more than once in a *.cc file
#ifndef _AVR_1WIRE_H_
#define _AVR_1WIRE_H_

#include "emstream.h"                       // We can use a serial device for debugging


/// This is the number of retries we'll wait for an acknowledgement from a sensor.
const uint16_t AOWI_RETRIES = 10000;

/// This is the size of a table which can hold 64-bit device identifiers
const uint8_t AOWI_NUM_IDS = 3;

/// This macro implements a rather dumb delay loop
#define AOWI_DELAY(x) for (volatile uint16_t _awi_dl = 0; _awi_dl < (x); _awi_dl++)

// These values set the length of various dumb delays. We don't really want to use up a
// timer to make the delay durations precise, so we use imprecise delay loops instead.
// Values which have worked, with AVR-GCC optimization at level 2:
//   CPU,Freq     Device   RESET_D  PRES_D  PRES_END  1us   15us   45us  90us
//   M32,4MHz    DS1820,22   120      12      1000    1-2   4-8     8    40-80
//   M32,4MHz    DS2450+23   254-140  25-16   1000     4    8-9           50
//   M324P,10MHz DS1822      254      30      2000     5    15      20   125
// Notes: For M32,4MHz: AOWI_DELAY (45) makes a correct reset pulse ~450us(?)
//        The DS1820/22 seem to demand different timing than DS2423/50, even when on
//        the same bus. There's probably a good reason but I haven't found it yet

/** This is the delay counter for creating a reset pulse. It needs to generate a pulse
 *  about 500 microseconds in duration.  F_CPU / 30000L has been seen to work.  
 */
const uint16_t AOWI_RESET_D = (uint16_t)(F_CPU / 30000L);

/** This is the length of time after the end of a reset pulse to wait for a presence
 *  pulse.  It should usually be around 70 microseconds.  (F_CPU / 250000L) has 
 *  been seen to work.  
 */
const uint16_t AOWI_PRES_D = (uint16_t)(F_CPU / 250000L);

/** This is the number of retries to wait for the presence pulse to end. 
 *  F_CPU / 100L has been known to work.
 */
const uint16_t AOWI_PRES_END = (uint16_t)(F_CPU / 100L);

/** This is a delay counter for producing an approximately 1 microsecond delay.
 *  \c (F_CPU / 6000000) has worked in the past, but (F_CPU / 4000000L) is used 
 *  because it doesn't cause a zero when \c F_CPU is 4 MHz. 
 */
const uint16_t AOWI_1us_D = 
						(uint16_t)((F_CPU / 6000000L) > 0 ? (F_CPU / 6000000L) : 1);

/** This is a delay counter for producing an approximately 15 microsecond delay.
 *  \c (F_CPU / 1800000) has been known to work.
 */
const uint16_t AOWI_15us_D = (uint16_t)(F_CPU / 1800000L);

/** This is a delay counter for producing an approximately 45 microsecond delay.
 *  \c (F_CPU / 500000) has worked in the past.
 */
const uint16_t AOWI_45us_D = (AOWI_15us_D * 3);  // (uint16_t)(F_CPU / 600000L);

/** This is a delay counter for producing an approximately 90 microsecond delay.
 *  \c (F_CPU / 250000) has been known to work.
 */
const uint16_t AOWI_90us_D = (AOWI_15us_D * 6);  // (uint16_t)(F_CPU / 300000L);


//-------------------------------------------------------------------------------------
/** \cond DO_NOT_DOXY - Users won't need to use this type directly
 * 
 *  This type holds a 64-bit identifier number for a One-Wire device. The number can be
 *  accessed as a single 64-bit number or as an array of eight bytes.
 */

typedef union
{
	unsigned long long num64;			///< This is the whole number as a long long
	unsigned char bytes[8];				///< This is the number as a set of 8 bytes
}
AOWI_device_ID;

/// \endcond DO_NOT_DOXY


//-------------------------------------------------------------------------------------
/** \brief This class implements a bit-banged One-Wire Interface (OWI) port. 
 *  \details This port uses one generic I/O port pin, manipulating it directly, thus 
 *  the name "bit-banged." One uses this class to set up a one-wire port, then creates 
 *  one or more driver objects which use this port to communicate with one-wire devices 
 *  such as DS1820 or DS1822 temperature sensors. See the documentation for specific
 *  driver classes for examples of how to write code using this one-wire driver class. 
 */

class avr_1wire
	{
	protected:
		/// Pointer to the input port to which the one-wire data bit is connected
		volatile uint8_t& data_inport;

		/// Pointer to the output port to which the one-wire data bit is connected
		volatile uint8_t& data_outport;

		/// Pointer to the data direction register for the data port
		volatile uint8_t& data_ddr;

		/// This is a bitmask for the pin used for the 1-wire data line
		uint8_t data_mask; 

		/// This is a counter for 1-wire bus errors; it's used for debugging
		uint8_t errors;

		/// This array holds a set of 64-bit device identification numbers
		AOWI_device_ID identifiers[AOWI_NUM_IDS];

		/** This is a counter to control the duration of the reset pulse; it can be
		 *  tuned by the auto_timing() method to compensate for CPU clock speed and
		 *  variations in 1-wire devices' timing. */
		uint16_t reset_pulse_dur;

		/// This pointer allows the use of a serial device for debugging messages.
		emstream* p_serial;

		void write_0 (void);                // Write logic 0 to the bus
		void write_1 (void);                // Write logic 1 to the bus

	public:
		// The constructor configures the I/O port and DDR
		avr_1wire (volatile uint8_t&, volatile uint8_t&, volatile uint8_t&, uint8_t,
				   emstream* = NULL);

		bool reset (void);                  // Communication channel reset
		bool auto_timing (void);            // Try to find a timing which works
		void search (void);                 // Search the One-Wire bus for devices
		void read_ID (void);                // Read ID from device alone on bus
		bool read_bit (void);               // Read a bit from the bus
		bool read_byte (uint8_t*);          // Read 8 bits from the bus
		bool write_byte (uint8_t);          // Write a byte to the bus, LSB first
		bool write_byte_rev (uint8_t);      // Write byte MSB first
		void match_ROM (uint8_t);           // Match device in the ID number array

		// Get the identifying number for the given entry in the array of ID numbers
		uint64_t get_ID (uint8_t);

		// Find the index of this device identifier number in the array
		uint8_t find_by_ID (uint64_t);

		// Find the index of a device with this type ID number
		uint8_t find_by_type (uint8_t);

		// Get one bit from the 64-bit device identifier
		bool get_ID_bit (uint8_t, uint8_t);

		// Set one bit in the given 64-bit device identifier
		void set_ID_bit (uint8_t, uint8_t, bool);

		// Show a list of devices on the one-wire bus
		void show_devices (emstream*, uint8_t = AOWI_NUM_IDS);
	} /*__attribute__ ((unpacked))*/;

#endif // _AVR_1WIRE_H_
