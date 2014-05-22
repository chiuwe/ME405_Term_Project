//*************************************************************************************
/** \file avr_sht15.cpp
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

#include <avr/io.h>                         // AVR I/O port definitions
#include "FreeRTOS.h"                       // Include FreeRTOS header if it exists
#include "task.h"                           // along with the FreeRTOS task functions
#include "avr_sht15.h"                      // Include header for this file


//-------------------------------------------------------------------------------------
/** This constructor sets up a bit-banged TWI port for the SHT15 driver to use.
 *  @param p_ser_dev A pointer to a serial device to use for debugging (default: NULL)
 */

avr_sht15::avr_sht15 (emstream* p_ser_dev)
{
	p_serial = p_ser_dev;                   // Save debugging serial port pointer

	ATWI_DDR |= ATWI_CLOCK_MASK;            // Set clock pin as an output
	ATWI_DDR &= ~ATWI_DATA_MASK;            // Set data pin as an input

	ATWI_OUTPORT |= ATWI_DATA_MASK;         // Data is high by default (pullup on)
	ATWI_OUTPORT &= ~ATWI_CLOCK_MASK;       // Clock is low when inactive
}


//-------------------------------------------------------------------------------------
/** This method starts a transmission on the TWI port. Before this method is called,
 *  the data line should be high and the clock line low. The start sequence goes like
 *  this:
 *  \li To begin, data is high and clock is low (this is assumed; make sure it's so)
 *  \li Clock is raised
 *  \li Data is dropped
 *  \li Clock is dropped, then raised again 
 *  \li Data is raised
 *  \li Clock is dropped and we're back to the starting condition
 */

void avr_sht15::start (void)
{
	// Assume that data is high and clock low before this method is called

	ATWI_CLOCK_HIGH ();
	ATWI_DELAY (ATWI_DEF_DEL);
	ATWI_DATA_LOW ();
	ATWI_DELAY (ATWI_DEF_DEL);
	ATWI_CLOCK_LOW ();
	ATWI_DELAY (ATWI_DEF_DEL);
	ATWI_CLOCK_HIGH ();
	ATWI_DELAY (ATWI_DEF_DEL);
	ATWI_DATA_HIGH ();
	ATWI_DELAY (ATWI_DEF_DEL);
	ATWI_CLOCK_LOW ();
	ATWI_DELAY (ATWI_DEF_DEL);
}


//-------------------------------------------------------------------------------------
/** This method sends a reset sequence to the TWI interface. The next action should be
 *  the sending of a transmission start sequence with start().
 */

void avr_sht15::reset (void)
{
	ATWI_DATA_HIGH ();                      // Make sure data is floating high

	for (uint8_t count = 0; count < 9; count++)
	{
		ATWI_CLOCK_HIGH ();                 // Send 9 clock pulses
		ATWI_DELAY (ATWI_DEF_DEL);
		ATWI_CLOCK_LOW ();
		ATWI_DELAY (ATWI_DEF_DEL);
	}
}


//-------------------------------------------------------------------------------------
/** This method writes a byte of data onto the SHT-15 TWI bus. Each bit in the byte is 
 *  written to the bus by putting it onto the data line and pulsing clock high then
 *  low; then a ninth pulse of the clock is used to check for an acknowledgement 
 *  from the device on the bus.
 *  @param charout The byte of data to be written
 *  @return True if the acknowledgement bit was seen, false if it was not
 */

bool avr_sht15::write (uint8_t charout)
{
	uint16_t count;                         // Used in waiting for the acknowledgement

	// For each bit in the byte to be sent, put it on the data line, then pulse the
	// clock signal
	for (uint8_t bmask = 0x80; bmask > 0; bmask >>= 1)
	{
		if (charout & bmask)
		{
			ATWI_DATA_HIGH ();              // change the data line to logic 1
		}
		else
		{
			ATWI_DATA_LOW ();               // This sends out a logic 0
		}
		ATWI_DELAY (ATWI_DEF_DEL);
		ATWI_CLOCK_HIGH ();
		ATWI_DELAY (ATWI_DEF_DEL);
		ATWI_CLOCK_LOW ();
		ATWI_DELAY (ATWI_DEF_DEL);
	}

	// Now release the data line, with its pullup on
	ATWI_DATA_HIGH ();
// 	ATWI_DELAY (ATWI_DEF_DEL);

	// Look for an acknowledgement bit with the clock high, then wait for the
	// acknowledgement to end after the clock is dropped
	ATWI_CLOCK_HIGH ();
// 	ATWI_DELAY (ATWI_DEF_DEL);
	for (count = 0; (ATWI_INPORT & ATWI_DATA_MASK) != 0; count++)
	{
		if (count > ATWI_RETRIES)
		{
			return (false);
		}
	}
	ATWI_CLOCK_LOW ();
	ATWI_DELAY (ATWI_DEF_DEL);

	return (true);
}


//--------------------------------------------------------------------------------------
/** This method reads a byte of data from the TWI bus. Some sort of command should 
 *  have already been sent to the TWI device to cause it to be sending us data. Once
 *  the data has been successfully read, this function sends an acknowledgement bit
 *  by taking control of the data line, setting it to 0, and pulsing the clock.
 *  @param  do_ack  True if we are to send acknowledge bit, false if not
 *  @return The byte which was read from the TWI port
 */

uint8_t avr_sht15::read (bool do_ack)
{
	uint8_t byte_in = 0x00;                 // The byte read from the TWI port

	ATWI_DATA_HIGH ();                      // Release control of data line
	ATWI_DELAY (ATWI_DEF_DEL);

	// For each bit in the byte to be read, set the clock high, read the data, then
	// drop the clock back to zero
	for (uint8_t bmask = 0x80; bmask > 0; bmask >>= 1)
	{
		ATWI_CLOCK_HIGH ();                 // Set the clock line high
		ATWI_DELAY (ATWI_DEF_DEL);
		if (ATWI_INPORT & ATWI_DATA_MASK)
			byte_in |= bmask;               // set the correct bit in byte_in...
		ATWI_CLOCK_LOW ();                  // Lower the clock line
		ATWI_DELAY (ATWI_DEF_DEL);
	}

	if (do_ack)                             // If sending an acknowledgement, set
	{                                       // the data line low before sending the
		ATWI_DATA_LOW ();                   // last clock pulse
		ATWI_DELAY (ATWI_DEF_DEL);
	}

	ATWI_CLOCK_HIGH ();                     // Regardless of ACK or no ACK, send a
	ATWI_DELAY (ATWI_DEF_DEL);              // single clock pulse at the end of the
	ATWI_CLOCK_LOW ();                      // byte read
	ATWI_DELAY (ATWI_DEF_DEL);

	if (do_ack)                             // If an acknowledgement was sent,
	{                                       // it's time to release the data line
		ATWI_DATA_HIGH ();
		ATWI_DELAY (ATWI_DEF_DEL);
	}

	return (byte_in);
}


//-------------------------------------------------------------------------------------
/** This method waits for an acknowledgement bit saying that the device on the TWI
 *  interface has dealt with a command which has been written to it. An acknowledgement
 *  bit consists of the device on the TWI bus pulling the data line low. If the
 *  acknowledgement has not been recieved after the number of tries indicated in the
 *  timeout parameter, this method gives up and returns false.
 *  @param retries The number of times to check before giving up
 *  @return True if the acknowledgement bit occurred, false if we timed out instead
 */

bool avr_sht15::wait_for_ack (uint32_t retries)
{
	// Watch the data line until it goes low (if it does)
	for (uint32_t count = 0; count < retries; count++)
	{
		if ((ATWI_INPORT & ATWI_DATA_MASK) == 0)
		{
			return (true);
		}
	}
	return (false);
}


//-------------------------------------------------------------------------------------
/** This method causes the TWI port to go to a low-power state by tri-stating its 
 *  pins. 
 */

void avr_sht15::sleep (void)
{
	ATWI_DDR &= ~(ATWI_CLOCK_MASK & ATWI_DATA_MASK);     // Set both pins as inputs
	ATWI_OUTPORT &= ~(ATWI_CLOCK_MASK & ATWI_DATA_MASK); // Drop pins to 0 (no pullups)
}


//-------------------------------------------------------------------------------------
/** This method wakes up the TWI port by setting the clock line as an output and 
 *  placing a 1 on the data line, thereby enabling its internal pullup resistor.
 */

void avr_sht15::wake_up (void)
{
	ATWI_OUTPORT |= ATWI_DATA_MASK;                    // Write 1 to data line
	ATWI_OUTPORT &= ~ATWI_CLOCK_MASK;                  // Write 0 to the clock line
	ATWI_DDR |= ATWI_CLOCK_MASK;                       // 
	ATWI_DDR &= ~(ATWI_CLOCK_MASK & ATWI_DATA_MASK);   // Set both pins as inputs
}


//-------------------------------------------------------------------------------------
/** This method asks the SHT15 sensor for the current temperature reading and returns
 *  the raw reading in an unsigned integer.
 *  @return A uint16_t containing the raw temperature reading
 */

uint16_t avr_sht15::temperature (void)
{
	union                                   // This union holds the raw data of a
	{                                       // humidity reading from the sensor.
		uint8_t byte[2];                    // We read 8 bits at a time to get a
		uint32_t word;                      // 16-bit number
	}
	raw_data;

// 	uint8_t bytes[2];                       // Bytes read in from the sensor

	start ();                               // Send transmission start code
	write (0x03);                           // Send the "measure temperature" code

	// We have to wait for the sensor to finish sensing; it signals completion by
	// pulling the data line low
	for (uint32_t count = 0; (ATWI_INPORT & ATWI_DATA_MASK) != 0; count++)
	{
		#ifdef configTICK_RATE_HZ           // If FreeRTOS is being used, call its
			vTaskDelay (2);                 // delay function to give control of
		#endif                              // the processor to other tasks for a time
		if (count > ATWI_TEMP_RETRY)
		{
			return (-100);
		}
	}

	raw_data.byte[1] = read (true);
	raw_data.byte[0] = read (false);

	return (raw_data.word);
}


//-------------------------------------------------------------------------------------
/** This method gets a raw temperature reading from the SHT15, then converts it to a
 *  reading in degrees Fahrenheit. The returned number is 10 times the temperature so
 *  that the resolution is 0.1 degrees F.
 *  @return The temperature in tenths of a degree Fahrenheit (so 723 is 72.3 degrees)
 */

int16_t avr_sht15::fahrenheit (void)
{
	return ((int16_t)((int32_t)(temperature ()) * 18 / 100) - 400);
}


//-------------------------------------------------------------------------------------
/** This method gets a raw temperature reading from the SHT15, then converts it to a
 *  reading in degrees Celsius. The returned number is 10 times the temperature so
 *  that the resolution is 0.1 degrees C.
 *  @return The temperature in tenths of a degree Celsius (so 253 is 25.3 degrees)
 */

int16_t avr_sht15::celsius (void)
{
	return ((int16_t)(temperature () / 10) - 400);
}


//-------------------------------------------------------------------------------------
/** This method asks the SHT15 sensor for the current humidity reading and returns
 *  the corrected relative humidity in an 8-bit integer.
 *  @return The measured relative humidity
 */

uint8_t avr_sht15::humidity (void)
{
	union                                   // This union holds the raw data of a
	{                                       // humidity reading from the sensor.
		uint8_t byte[4];                    // We only get 16 bits from the sensor,
		uint32_t all;                       // but we need 32 bits to do the math
	}                                       // without risk of overflow
	raw_data;

// 	uint8_t bytes[2];                       // Bytes read in from the sensor
// 	int32_t raw;                            // Raw reading from RH sensor
	uint32_t rel_humid;                     // Humidity converted from raw data

	start ();                               // Send transmission start code
	write (0x05);                           // Send the "measure humidity" code

	// We have to wait for the sensor to finish sensing; it signals completion by
	// pulling the data line low
	for (uint32_t count = 0; (ATWI_INPORT & ATWI_DATA_MASK) != 0; count++)
	{
		#ifdef configTICK_RATE_HZ           // If FreeRTOS is being used, call its
			vTaskDelay (2);                 // delay function to give control of
		#endif                              // the processor to other tasks for a time
		if (count > ATWI_TEMP_RETRY)
		{
			return (-1);
		}
	}

	raw_data.byte[1] = read (true);         // Get raw data bytes from the sensor
	raw_data.byte[0] = read (false);

// 	raw = (int32_t)(((uint16_t)(bytes[0]) << 8) | bytes[1]);

	// Formula: RH = 0.0405 * raw - 2.8E-6 * raw^2 - 4
	rel_humid = raw_data.all * 405 / 10000;
	rel_humid -= raw_data.all * raw_data.all * 28 / 10000000L;
	rel_humid -= 4;

	if (rel_humid > 99)
	{
		rel_humid = 100;
	}

	return ((uint8_t)rel_humid);
}


//-------------------------------------------------------------------------------------
/** \cond NO_DOXY This simple function finds a number's absolute value. It's here as
 *  a quick, easy solution for the "<<" operator below.
 */

inline int16_t _iab (int16_t num)
{
	return ((num < 0) ? -num : num);
}

/// \endcond


//-------------------------------------------------------------------------------------
/** This overloaded operator writes information about the status of a SHT15 sensor to
 *  the serial port. The printout shows the current temperature and humidity. This
 *  operator is quite slow to run, because it gets a reading from the sensor. 
 *  @param ser_dev A reference to the serial device on which we're writing information
 *  @param sensor A reference to the sensor object whose status is being written
 *  @return A reference to the same serial device on which we write information.
 *          This is used to string together things to write with "<<" operators
 */

emstream& operator << (emstream& ser_dev, avr_sht15& sensor)
{
	int16_t temptemp = sensor.fahrenheit ();

	ser_dev << PMS ("Temp: ") << (temptemp / 10) << '.' << (_iab (temptemp) % 10)
			<< PMS ("  Humid: ") << sensor.humidity ();

	return (ser_dev);
}

