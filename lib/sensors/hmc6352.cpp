//*************************************************************************************
/** \file hmc6352.cpp
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

#include "FreeRTOS.h"                       // Main header for FreeRTOS
#include "task.h"                           // Needed for the vTaskDelay() function
#include "hmc6352.h"                        // Header for this compass chip


/** This is the I2C bus address of the sensor as used for writing commands and data to
 *  the sensor. Sensors are shipped with a default write address of 0x42. This address 
 *  includes the read/write bit, so in the terminology used in the AVR data sheets, 
 *  it's a \c SLA+W number as given here.
 */
const uint8_t HMC6352_WRITE_ADDRESS = 0x42;

/** This is the I2C bus address of the sensor as used for reading commands and data 
 *  from the sensor. Sensors are shipped with a default read address of 0x43. This 
 *  address includes the read/write bit, so in the terminology used in the AVR data 
 *  sheets, this is a \c SLA+R number given here.
 */
const uint8_t HMC6352_READ_ADDRESS = 0x43;


//-------------------------------------------------------------------------------------
/** This constructor creates an HMC6352 driver object.
 *  @param p_debug_port A serial port, often RS-232, for debugging text (default: NULL)
 */

hmc6352::hmc6352 (emstream* p_debug_port)
	: i2c_master (p_debug_port)
{
	DBG (p_serial, PMS ("HMC6352 constructor") << endl);
}


//-------------------------------------------------------------------------------------
/** This method sets the mode byte of the HMC6352. The mode is only set in RAM unless 
 *  saved in EEPROM elsewhere.
 *  @param mode_byte A byte specifying the mode into which the sensor will be set
 */

void hmc6352::set_mode (uint8_t mode_byte)
{
	// Make sure the mode byte is legal; if it's not, complain and exit
	if ((mode_byte & 0b10001100) || ((mode_byte & 0b00000011) == 0b00000011))
	{
		DBG (p_serial, PMS ("Illegal HMC6352 mode byte ") << bin << mode_byte << dec
			 << endl);
		return;
	}

	// Send an I2C start condition
	start ();

	// Now send the address thingy
	send (HMC6352_WRITE_ADDRESS, 0x18);

	// Send a 'G' command, meaning write into RAM
	send ('G', 0x28);

	// Send the address for the mode control byte, which is at address 0x74
	send (0x74, 0x28);

	// Send the mode byte which is to be set
	send (mode_byte, 0x28);

	// Send a stop condition
	stop ();
}


//-------------------------------------------------------------------------------------
/** This method gets a heading from the HMC6352 compass. It assumes that the sensor is
 *  in standby mode; this means that it is necessary to send an "A" command, then wait
 *  until the sensor has measured and computed its heading, then read the heading. All
 *  this means that the \c heading() method takes quite some time to run. 
 *  @return The measured heading, in tenths of a degree of angle...eventually
 */

uint16_t hmc6352::heading (void)
{
	union                                   // This union holds the raw data of a
	{                                       // heading from the sensor. It gives us
		uint8_t byte[2];                    // two bytes which we put together to make
		uint32_t word;                      // a 16-bit word that will be returned
	}
	raw_data;

	start ();                               // Send an I2C start condition
	send (HMC6352_WRITE_ADDRESS, 0x18);     // Now send the write address thingy
	send ('A', 0x28);                       // Send an 'A' (read a heading) command
	stop ();                                // Cause an I2C stop condition

	// Wait about 6 ms for the heading to be calculatificationized
	vTaskDelay (configMS_TO_TICKS (6));

	start ();                               // Send an I2C start condition
	send (HMC6352_READ_ADDRESS, 0x40);      // Now send the read address
	raw_data.byte[1] = receive (true);      // Read the first byte (true = send ACK)
	raw_data.byte[0] = receive (false);     // Read the second byte (false = send NACK)
	stop ();                                // Cause an I2C stop condition

	return (raw_data.word);                 // Return the bytes we read, as one word
}


//-------------------------------------------------------------------------------------
/** This overloaded operator writes information about the status of an HMC6352 sensor 
 *  to the serial port. The printout shows the current heading as text, with the 
 *  integer part of the heading, a decimal point, and the fractional part. It is 
 *  assumed that the sensor is in standby mode, so the "A" command is sent to the 
 *  sensor, then the driver waits for the sensor to compute and make available the 
 *  heading. This process means that this \c << operator is quite slow to run. 
 *  @param ser_dev A reference to the serial device on which we're writing information
 *  @param sensor A reference to the sensor object whose status is being written
 *  @return A reference to the same serial device on which we write information.
 *          This is used to string together things to write with "<<" operators
 */

emstream& operator << (emstream& ser_dev, hmc6352& sensor)
{
	int16_t the_heading = sensor.heading ();
	div_t parts_is_parts = div (the_heading, 10);

	ser_dev << parts_is_parts.quot << '.' << parts_is_parts.rem;

	return (ser_dev);
}

