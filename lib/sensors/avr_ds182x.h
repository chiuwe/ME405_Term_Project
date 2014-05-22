//*************************************************************************************
/** \file avr_ds182x.h
 *    This file contains a class which interfaces an AVR processor to each of several
 *    types of Dallas Semiconductor One-Wire temperature sensors. It has been written
 *    to work on the DS18B20 and DS182X. The sensors are connected to a single digital
 *    I/O pin on the microcontroller. Many sensors can be connected to the same pin. 
 *    Each sensor should be powered with Vcc and ground, not bus parasite powered. 
 *
 *  Revisions:
 *    \li 12-18-2007 JRR Created file
 *    \li 12-30-2007 JRR Corrected spelling of Fahrenheit
 *    \li 03-20-2009 JRR Updated for newer boards
 *    \li 12-01-2012 JRR Changed to work in new FreeRTOS based ME405 environment,
 *                       made "<<" operator work properly, made generic _ds182x class
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

// This define prevents this file from being included more than once in a *.cpp file
#ifndef _AVR_DS182X_H_
#define _AVR_DS182X_H_

#include "emstream.h"                   // Include serial device header for debugging
#include "avr_1wire.h"                  // This class needs the 1-wire driver class

/// This is the number of retries we will wait for a response from the 1-wire chip.
const uint16_t DS182X_RETRIES = 40000;

/// This is the duration of the start of a delay loop to wait for a conversion.
const uint16_t DS182X_ST_DEL = 1000;

/// This is the chip type ID (lowest byte of ID number) for the DS1820
const uint8_t DS1820_TYPE_ID = 0x10;

/// This is the chip type ID (lowest byte of ID number) for the DS1822
const uint8_t DS1822_TYPE_ID = 0x22;

/// This is the maximum number of retries if the temperature reading isn't reasonable.
const uint8_t DS182X_MAX_TOUT = 3;


//--------------------------------------------------------------------------------------
/** \brief This class implements a driver for an AVR processor to a DS182X type
 *  "one-wire" temperature sensor. 
 *  \details 
 *  The sensor should actually be connected with three wires (power, ground, and data).
 *  This driver is not intended for two-wire connection (power/data and ground), which 
 *  is the way one-wire devices are connected using the minimum number of wires. There
 *  is no way to actually connect a one-wire device with just one wire. 
 * 
 *  \section Usage
 *  One or more one-wire devices are connected with their data pins all attached to 
 *  one digital I/O pin on the microcontroller. It is usually necessary to attach an
 *  external pullup resistor of about 4.7K to the pin; the internal pullup resistors
 *  in an AVR are generally not strong enough to work properly as 1-wire pullups. 
 * 
 *  This class uses the \c avr_1wire class to operate the one-wire interface. To use 
 *  this driver, one must first set up a one-wire interface. The \c avr_1wire
 *  constructor takes as parameters the following:
 *    \li The parallel input port to which the one wire that goes to the DS182X's data
 *        pin is connected
 *    \li The parallel output port to which that same wire is connected
 *    \li The data direction register which controls the I/O pin for that wire
 *    \li The number (from 0 to 7) of the pin
 *    \li (Optional) A serial device which will be used to show debugging messages
 * 
 *  \code
 *  my_1wire = new avr_1wire (PIND, PORTD, DDRD, 7, p_serial);
 *  my_1wire->search ();
 *  my_1wire->show_devices (p_serial);
 *  \endcode
 *  Calling the \c search() method causes the one-wire driver to scan the wire and 
 *  make a list of all the one-wire devices attached to it. The \c avr_1wire driver
 *  must be configured to hold at least as many device ID's as there are devices on 
 *  the wire; see the define \c AOWI_NUM_IDS in file \c avr_1wire.h. 
 *  To display the list of the ID's of devices found, call \c show_devices(). 
 *  After the one-wire driver has been created, the \c avr_ds182x driver is created:
 *  \code
 *  my_ds182x = new avr_ds182x (my_1wire);
 *  my_ds182x->find_by_type (0x22);
 *  \endcode
 *  The \c 0x22 is the type ID of a DS1822; the type ID of a DS1820 is \c 0x10. 
 *  One can only use \c find_by_type() when there is only a single device of a given
 *  type attached to the wire; this is usually the case. After the device has been
 *  set up, one can read the temperature in desired units with a call to \c celsius()
 *  or \c fahrenheit(), which return temperature as signed 16-bit integers in
 *  units of tenths of degrees, such as \c 234 for 23.4 degrees. When using a serial 
 *  device such as a USB serial port, an SD card, or even a serial RTOS queue, one 
 *  can also use the overloaded \c << operator to display the measured temperature: 
 *  \code
 *  *p_serial << "The temperature is: " << *my_ds182x << endl;
 *  \endcode
 */

class avr_ds182x
{
	protected:
		avr_1wire* the_bus;                 ///< Pointer to the 1-wire bus object
		uint8_t ID_index;                   ///< Index of the sensor in bus's array
		uint8_t type_ID;                    ///< The sensor's type ID byte

	public:
		avr_ds182x (avr_1wire*, uint8_t);   // Constructor attaches to 1-wire bus

		// This method writes a configuration into the DS182X's EEPROM
		bool configure (uint8_t, uint8_t = 0x70, uint8_t = 0xE0);

		void find_by_ID (uint64_t);         // Find device in bus's device table
		void find_by_type (void);           // Find one on bus using its type ID
		int16_t temperature (void);         // Reads raw temperature data
		int16_t fahrenheit (void);          // Returns temperature in 0.1 deg F
		int16_t celsius (void);             // Returns temperature in 0.1 deg C
};

// This operator conveniently prints the temperature found by a DS182X sensor
emstream& operator<< (emstream&, avr_ds182x&);

#endif // _AVR_DS182X_H_
