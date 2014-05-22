//*************************************************************************************
/** \file avr_sht15.h
 *    This file contains a class which runs a Sensirion SHT15 or similar temperature 
 *    and humidity sensor. The sensor is attached to an AVR microcontroller with a 
 *    bit-banged interface similar to the TWI Two-Wire Interface (TWI is Atmel task 
 *    for the Philips I2C interface, but Atmel hasn't purchased a license to use the
 *    term I2C).  The term "bit-banged" means that instead of using the TWI hardware 
 *    in the microcontroller, this class uses any old I/O port pins (attached to the
 *    same 8-bit port) for the clock and data lines. This is necessary because the
 *    SHT15's interface isn't quite the same as standard I2C. 
 *
 *  Revisions:
 *    \li 07-21-2007 JRR Created this file
 *    \li 12-30-2007 JRR Corrected spelling of Fahrenheit
 *    \li 12-27-2012 JRR Updated for use with FreeRTOS based ME405 software
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
#ifndef _AVR_SHT15_H_
#define _AVR_SHT15_H_

#include "emstream.h"                       // Use serial device for debugging 


/** This is the data direction register for the I/O port to which the SHT15 is 
 *  connected. Both the data and clock pins must be connected to the same port.
 */
#define ATWI_DDR            DDRC

/** This is the output port used for the SHT15's connections. It must match the data
 *  direction register given in \c ATWI_DDR.
 */
#define ATWI_OUTPORT        PORTC

/** This is the input port used for the SHT15's connections. It must match the data
 *  direction register given in \c ATWI_DDR and the output port in \c ATWI_OUTPORT.
 *  Typical values might be, for example, \c DDRC, \c PORTC, and \c PINC. 
 */
#define ATWI_INPORT         PINC

/** This bitmask chooses the pin used for the data wire to the SHT15. A typical value
 *  might be given as (1 << 7) which specifies that pin 7 is used. This is equivalent
 *  to 0b10000000. 
 */
const uint8_t ATWI_DATA_MASK = (1 << 7);

/** This bitmask chooses the pin used for the clock wire to the SHT15.
 */
const uint8_t ATWI_CLOCK_MASK = (1 << 6);

/** This is the number of retries to wait for an acknowledgement from the SHT15.
 */
#define ATWI_RETRIES        10000

/** This retry counter is for a really long wait for the SHT15 to measure temperature
 *  or humidity and get the measurement ready to read. If FreeRTOS is in use, it's a
 *  good idea to use \c vTaskDelay() to give other tasks some extra time while waiting.
 */
#define ATWI_TEMP_RETRY     1000000L

/// This macro sets the data line low by setting data as an output, logic zero.
#define ATWI_DATA_LOW()     ATWI_OUTPORT &= ~ATWI_DATA_MASK; ATWI_DDR |= ATWI_DATA_MASK

/// This macro lets the data line go high by setting data as an input with a pullup.
#define ATWI_DATA_HIGH()    ATWI_DDR &= ~ATWI_DATA_MASK; ATWI_OUTPORT |= ATWI_DATA_MASK

/// This macro sets the clock line low by putting a zero there.
#define ATWI_CLOCK_LOW()    ATWI_OUTPORT &= ~ATWI_CLOCK_MASK

/// This macro sets the clock line high by putting a one there.
#define ATWI_CLOCK_HIGH()   ATWI_OUTPORT |= ATWI_CLOCK_MASK

/// This macro implements a rather dumb delay loop.
#define ATWI_DELAY(x)       for (volatile uint16_t atwid = 0; atwid < (x); atwid++) { }

/// This is the default length of a dumb delay loop.
//  Values which have worked:  25 at 4 MHz
#define ATWI_DEF_DEL        ((uint16_t)(F_CPU / 160000L))


//--------------------------------------------------------------------------------------
/** \brief This class implements a driver for a Sensirion SHT-15 temperature and 
 *  humidity sensor attached to an AVR microcontroller. 
 *  \details The driver uses a bit-banged TWI-like interface. The sensor doesn't need a
 *  full I2C/TWI interface (and in fact the documentation says that it isn't compatible
 *  wiht a proper I2C/TWI interface), so we use two generic I/O pins to create the 
 *  TWI-like interface which will be used to communicate with the sensor. 
 * 
 *  \section sec_use Usage
 *  In order to use the SHT15 driver, one must first define the constants that specify
 *  the microcontroller I/O pins used to connect to the sensor. There are just two 
 *  pins, \c data and \c clock. They are ordinary I/O pins. One must specify the input
 *  port, output port, and data direction register for these pins in \c avr_sht15.h.
 *  For example, to use I/O port C on an AVR, one could have:
 *  \code
 *  #define ATWI_DDR            DDRC
 *  #define ATWI_OUTPORT        PORTC
 *  #define ATWI_INPORT         PINC
 *  \endcode
 *  One must also specify the pins to which the \c data and \c clock lines connect. 
 *  This is done by creating bitmasks. For example, to connect to pin 7 of the given
 *  I/O port, one would specify as the bitmask \c (1 \c << \c 7) or \c 0b10000000. 
 *  The two forms are equivalent, though the author prefers \c (1 \c << \c 7) because
 *  it clearly shows the pin number. An example specification of the bitmasks would be:
 *  \code
 *  const uint8_t ATWI_DATA_MASK = (1 << 7);
 *  const uint8_t ATWI_CLOCK_MASK = (1 << 6);
 *  \endcode
 *  The system of specifying constants for I/O pins in \c avr_sht15.h means that only
 *  one SHT15 can be connected to each microcontroller at a time, but it uses less
 *  memory and processor time than a mechanism which allows the use of many SHT15's 
 *  with one microcontroller. Changing the way the code is written to enable the use 
 *  of many SHT15's at once would not be particularly difficult.
 * 
 *  To create a SHT15 driver in your code and periodically ask it for the temperature
 *  and humidity, one can write code such as this:
 *  \code
 *  avr_sht15* p_my_sht15 = new avr_sht15;
 *  ...
 *  for (;;)             // This is a task's loop
 *  {
 *      ...
 *      int16_t a_temperature = p_my_sht15->celsius ();
 *      uint8_t a_humidity = p_my_sht15->humidity ();
 *      ...
 *      delay (1000);    // No need to measure temperature or humidity too often
 *  }
 *  \endcode
 *  The \c celsius () and \c fahrenheit() methods return temperature in units of tenths
 *  of a degree, so a temperature of 23.4 C would be returned as the number 234. One
 *  can print such a temperature by using division and modulo by 10; another way is to
 *  use an overloaded \c << operator to have the SHT15 "print itself." 
 *  \code
 *  *p_serial << *p_my_sht15 << endl;
 *  \endcode
 *  The line above will cause the SHT15 to "print" its temperature and humidity both to
 *  a serial device; the resulting "printout" will look like:
 *  \code
 *  Temp: 66.9  Humid: 53
 *  \endcode
 */

class avr_sht15
{
	protected:
		/** This pointer points to a serial port to be used for debugging. If left out
		 *  of the constructor parameters, it defaults to \c NULL for no debugging 
		 *  messages.
		 */
		emstream* p_serial;

	public:
		avr_sht15 (emstream* = NULL);       // Constructor to set up a SHT15 driver

		void start (void);                  // Start a transmission
		void reset (void);                  // Communication channel reset

		bool write (uint8_t);               // Write a byte to the TWI bus
		bool wait_for_ack (uint32_t);       // Wait for acknowledgement bit
		uint8_t read (bool);                // Read a byte from the TWI bus
		void sleep (void);                  // Go to the most power-saving state
		void wake_up (void);                // Awaken from sleep mode
		uint16_t temperature (void);        // Get a 14-bit temperature reading
		int16_t fahrenheit (void);          // Returns temperature in 0.1 deg F
		int16_t celsius (void);             // Returns temperature in 0.1 deg C
		uint8_t humidity (void);            // Returns relative humidity in percent
};

// This operator "prints" a SHT15 sensor by showing its current measured outputs
emstream& operator << (emstream&, avr_sht15&);

#endif // _AVR_SHT15_H_
