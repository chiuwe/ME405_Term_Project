//*************************************************************************************
/** \file avr_ds182x.cpp
 *    This file contains a class which interfaces an AVR processor to each of several
 *    types of Dallas Semiconductor One-Wire temperature sensors. It has been written
 *    to work on the DS18B20 and DS1822. The sensors are connected to a single digital
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

#include "FreeRTOS.h"                       // Include FreeRTOS functions
#include "task.h"                           // FreeRTOS timing is in this file

#include "avr_ds182x.h"                     // Header for this file


//--------------------------------------------------------------------------------------
/** This constructor creates a DS1822 interface object. A One-Wire interface to which 
 *  the sensor is attached must already have been created.
 *  @param a_bus A pointer to a one-wire bus object which connects to this sensor
 *  @param type_ID_byte The type identifier for a given model of sensor, for example
 *                      \c 0x10 for a DS1820 or \c 0x22 for a DS1822
 */

avr_ds182x::avr_ds182x (avr_1wire* a_bus, uint8_t type_ID_byte)
{
	the_bus = a_bus;                     // Save a pointer to the 1-wire bus
	type_ID = type_ID_byte;              // Save the type ID byte

	ID_index = 0xFF;                     // Not identified in bus's table yet
}


//--------------------------------------------------------------------------------------
/** This method writes a byte to the configuration register of the DS182X. While doing
 *  so, it also writes to the alarm configuration registers, which also include the
 *  alarm registers. Writing 0x70 to the upper temperature alarm and 0xE0 to the
 *  lower temperature alarm should keep them from going off too often, although since
 *  one doesn't usually check the alarms, it doesn't matter very much. The resolution
 *  byte can be \c 0x1F for 9 bits, \c 0x3F for 10 bits, \c 0x5F for 11 bits and
 *  \c 0x7F for 12 bits resolution. Higher resolution makes the DS182X work more 
 *  slowly -- about 3/4 of a second at 12 bits. The data sent here is stored in the 
 *  DS182X's EEPROM, so this command doesn't need to be run every time a sensor is 
 *  powered up. 
 *  @param resolution A byte which sets the A/D resolution for the sensor
 *  @param high_alarm Value for the high alarm register (default: 0x70)
 *  @param low_alarm Value for the low alarm register (default: 0xE0)
 */

bool avr_ds182x::configure (uint8_t resolution, uint8_t high_alarm, uint8_t low_alarm)
{
	if (!(the_bus->reset ()))               // Begin with a reset pulse, making sure
	{                                       // a presence pulse comes back from device
		return (false);
	}

	if (ID_index != 0xFF)                   // If the device ID number is valid,
	{                                       // this device must be selected
		the_bus->match_ROM (ID_index);
	}
	else                                    // If this is the only device on the bus,
	{                                       // skip ROM selection command
		the_bus->write_byte (0xCC);
	}

	the_bus->write_byte (0x4E);             // Send the write scratchpad code, 0x4E

	the_bus->write_byte (high_alarm);       // Now send the high and low alarm values,
	the_bus->write_byte (low_alarm);        // then the configuration byte; each byte
	the_bus->write_byte (resolution);       // is sent LSB first

	if (!(the_bus->reset ()))               // Now send another reset pulse to try to
	{                                       // ensure that the sensor paid attention
		return (false);
	}

	return (true);                          // If we got here, things are probably OK
}


//--------------------------------------------------------------------------------------
/** This method finds the index of the given sensor in the One-Wire bus's device ID 
 *  table by looking for a matching 64-bit device identifier. The result is saved in
 *  ID_index. If no match is found, the ID is set to 0xFF. 
 *  @param ID_to_match The 64-bit ID number for the device we seek
 */

void avr_ds182x::find_by_ID (uint64_t ID_to_match)
{
	ID_index = the_bus->find_by_ID (ID_to_match);
}


//--------------------------------------------------------------------------------------
/** This method finds the index of the given sensor in the One-Wire bus's device ID 
 *  table by looking for a device whose device type matches that of this sensor. If
 *  there are more than one such sensors on the bus, this method just finds the first
 *  one and returns it. The result is saved in ID_index. If no match is found, the ID 
 *  is set to 0xFF. 
 *  @return The index into the device table of this device's 64-bit ID number
 */

void avr_ds182x::find_by_type (void)
{
	ID_index = the_bus->find_by_type (type_ID);
}


//--------------------------------------------------------------------------------------
/** This method initiates a temperature conversion and reads the converted temperature
 *  as a raw 16-bit binary number from the "scratchpad" memory in the DS182X. 
 *  @return A 16-bit number corresponding to the temperature, or 0 if something's amiss
 */

int16_t avr_ds182x::temperature (void)
{
	volatile uint16_t time_cnt;             // Timeout counter
	uint8_t tm_out = 0;                     // Outer loop timeout counter

	union                                   // Union holds integer to be returned
	{
		int16_t word;                       // The whole 16 bits
		uint8_t bytes[2];                   // Array of two eight-bit parts
	} data;

	// If the data comes out screwy, we should retry a couple of times to get good data
	do
	{
		if (the_bus->reset ())              // Begin with a reset pulse, making sure
		{                                   // a presence pulse comes back from device

			if (ID_index != 0xFF)           // If the device ID number is valid,
			{                               // this device must be selected
				the_bus->match_ROM (ID_index);
			}
			else                            // If this is the only device on the bus,
			{                               // skip ROM selection command
				the_bus->write_byte (0xCC);
			}

			// Send the convert temperature code
			the_bus->write_byte (0x44);

			// Delay to let the sensor get started up. The DS182X sensors are slow,
			// taking ~100 ms minimum and ~750 ms maximum to do a conversion
			#ifdef FREERTOS_CONFIG_H
				vTaskDelay (configMS_TO_TICKS (80));
			#else
				for (volatile uint16_t delc = 0; delc < DS1822_ST_DEL; delc++) { }
			#endif

			// Wait until the data line goes high, indicating conversion complete
			for (time_cnt = 0; !(the_bus->read_bit ()); time_cnt++)
			{
				#ifdef FREERTOS_CONFIG_H
					vTaskDelay (configMS_TO_TICKS (10));
				#endif
				if (time_cnt > DS182X_RETRIES)
				{
					data.word = -100;       // This error value will cause a retry
				}
			}

			// The sensor should be done now, so read the data from the scratchpad
			if (!(the_bus->reset ()))
			{
				data.word = -120;           // This error value will cause a retry
			}

			if (ID_index != 0xFF)           // Do another match ROM command
			{
				the_bus->match_ROM (ID_index);
			}
			else
			{
				the_bus->write_byte (0xCC);
			}

			the_bus->write_byte (0xBE);             // Read scratchpad command
			the_bus->read_byte (&data.bytes[0]);    // Get the first byte out
			the_bus->read_byte (&data.bytes[1]);    // Get the second byte

			#ifdef FREERTOS_CONFIG_H                // If running the RTOS, give
				taskYIELD ();                       // other tasks a chance to run
			#endif

			if (!(the_bus->reset ()))               // And another reset pulse
			{
				data.word = -130;                   // This error value causes a retry
			}
		}
	}
	while ((tm_out++ < DS182X_MAX_TOUT) && ((data.word < -80) || (data.word > 260)));

	return (data.word);                     // Return the 16-bit result
}


//--------------------------------------------------------------------------------------
/** This method reads the temperature sensor, then converts the results into an integer
 *  which contains the temperature in degrees Celsius times ten.
 *  @return The temperature in tenths of a degree Celsius
 */

int16_t avr_ds182x::celsius (void)
{
	// Get a binary temperature reading
	int16_t temptemp;
	temptemp = temperature ();

	// Convert it into Celsius degrees, with a resolution of about half a degree
	if (temptemp > -100)
	{
		// If this is a DS1822, shift 3 bits of resolution-not-accuracy stuff out
		if (type_ID == 0x22)
		{
			temptemp >>= 3;
		}
		temptemp *= 5;
	}

	return (temptemp);
}


//--------------------------------------------------------------------------------------
/** This method gets a temperature reading in Celsius, then converts it to an old old
 *  fashioned Fahrenheit reading.
 *  @return The temperature in tenths of a degree Fahrenheit
 */

int16_t avr_ds182x::fahrenheit (void)
{
	// Get a binary temperature reading
	int16_t temptemp;
	temptemp = temperature ();

	// Convert it into Fahrenheit degrees, with a resolution of about a degree
	if (temptemp > -100)
	{
		// If this is a DS1822, shift 3 bits of resolution-not-accuracy stuff out
		if (type_ID == 0x22)
		{
			temptemp >>= 3;
		}
		temptemp *= 9;
		temptemp += 320;
	}

	return (temptemp);
}


//--------------------------------------------------------------------------------------
/** This overloaded operator allows a temperature reading to be printed on a serial 
 *  device such as a regular serial port or radio module in text mode. This allows a
 *  display in the style of 'cout.' The temperature is printed in Fahrenheit to a 
 *  precision of 0.1 degrees by default. Note that the sensor isn't so accurate, but
 *  we might as well not waste what accuracy we have through quantization error. 
 *  @param serial A reference to the serial-type object to which to print
 *  @param sensor A reference to the DS182X object to be displayed
 */

emstream& operator<< (emstream& serial, avr_ds182x& sensor)
{
	int16_t temp_temp;                      // Temporary storage for temperature

	temp_temp = sensor.celsius ();

	serial << (temp_temp / 10) << "." << (temp_temp % 10);

	return (serial);
}
