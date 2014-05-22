//*************************************************************************************
/** \file avr_1wire.cpp
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

#include <stdlib.h>                         // Standard AVR library
#include <avr/io.h>                         // AVR I/O port definitions

#include "FreeRTOS.h"
#include "task.h"

#include "avr_1wire.h"                      // Include header for this file


//-------------------------------------------------------------------------------------
/** This constructor creates a bit-banged one-wire port object. The I/O input and 
 *  output ports as well as the bitmask for the one-wire pin must be given. 
 *  @param data_in_port The input port of the data pin, such as PIND
 *  @param data_out_port The output port of the data pin, such as PORTD
 *  @param data_dir_reg The data direction register for the I/O port, such as DDRD
 *  @param data_bit The bit number for the data pin, such as 4 for pin 4
 *  @param p_ser_dev A pointer to a serial device for debugging messages 
 *                   (default: NULL, which means no messages will be sent out)
 */

avr_1wire::avr_1wire (volatile uint8_t& data_in_port, volatile uint8_t& data_out_port, 
					  volatile uint8_t& data_dir_reg, uint8_t data_bit,
					  emstream* p_ser_dev)
	: data_inport (data_in_port), data_outport (data_out_port), data_ddr (data_dir_reg)
{
	data_mask = (1 << data_bit);            // Save the pin's bitmask

	p_serial = p_ser_dev;                   // Save a pointer to the serial device
	reset_pulse_dur = AOWI_RESET_D;         // Set default value

	errors = 0;                             // Zero the count of errors

	data_ddr &= ~data_mask;                 // Set data direction to input
	data_outport |= data_mask;              // Turn pullup resistor on
	AOWI_DELAY (AOWI_90us_D);               // Wait a few microseconds
}


//-------------------------------------------------------------------------------------
/** This method sends a reset sequence to the one-wire interface. This is accomplished
 *  by sending a long (>480 us) pulse, then making sure a presence pulse was sent back
 *  by at least one device on the bus. 
 *  @return True if a presence pulse was detected, false if not
 */

bool avr_1wire::reset (void)
{
	volatile uint32_t end_tout;             // Counter used to check for timeout

	// Send a low pulse of at least 480 us
	data_outport &= ~data_mask;             // Put a zero on the output line
	data_ddr |= data_mask;                  // Force data line to output zero
	AOWI_DELAY (AOWI_RESET_D);              // Wait a while
	data_ddr &= ~data_mask;                 // Set data direction to input
    data_outport |= data_mask;              // Turn pullup resistor on
	AOWI_DELAY (AOWI_PRES_D);               // Wait before checking for presence pulse

	// If there's a logic 1 immediately after the presence pulse, nobody's home
	if (data_inport & data_mask)
	{
		DBG (p_serial, PMS ("1-Wire no presence on pin 0x") << hex << data_mask << dec
			 << endl);
		return false;
	}

	for (end_tout = 0; (data_inport & data_mask) == 0; end_tout++)
	{
		if (end_tout >= AOWI_PRES_END)
		{
			DBG (p_serial, PMS ("1-Wire reset timeout on pin 0x") << hex << data_mask 
				 << dec << endl);
			return false;
		}
	}
// 	DBG (p_serial, PMS ("1-Wire reset OK on pin 0x") << hex << data_mask << dec << endl);

	AOWI_DELAY (AOWI_90us_D);				// Wait before doing the next thing
	return (true);
}


//-------------------------------------------------------------------------------------
/** This method writes a logic 0 to the one-wire bus. It does so by pulling the data
 *  line low for about 90 microseconds, then letting it go high for about 1 us. 
 */

void avr_1wire::write_0 (void)
{
	#ifdef FREERTOS_CONFIG_H
		portENTER_CRITICAL ();
	#endif

	data_outport &= ~data_mask;             // Set output data to 0
	data_ddr |= data_mask;                  // Force data line to output zero
	AOWI_DELAY (AOWI_90us_D);               // Hold it there for about 90 us
	data_ddr &= ~data_mask;                 // Let the line go to logic 1
	data_outport |= data_mask;              // Turn pullup resistor on
	AOWI_DELAY (AOWI_1us_D);                // Leave it at logic 1 for about 1 us

	#ifdef FREERTOS_CONFIG_H
		portEXIT_CRITICAL ();
	#endif
}


//-------------------------------------------------------------------------------------
/** This method writes a logic 1 to the one-wire bus. It does so by pulling the data
 *  line low for about a microsecond, then letting it go high for about 90 us. 
 */

void avr_1wire::write_1 (void)
{
	#ifdef FREERTOS_CONFIG_H
		portENTER_CRITICAL ();
	#endif

	data_outport &= ~data_mask;             // Set output data to 0
	data_ddr |= data_mask;                  // Force data line to output zero
	AOWI_DELAY (AOWI_1us_D);                // Hold it there for about 1 us
	data_ddr &= ~data_mask;                 // Let the line go to logic 1
	data_outport |= data_mask;              // Turn pullup resistor on

	#ifdef FREERTOS_CONFIG_H
		portEXIT_CRITICAL ();
	#endif

	AOWI_DELAY (AOWI_90us_D);               // Leave it at logic 1 for about 90 us
}


//--------------------------------------------------------------------------------------
/** This method reads a bit of data from a device on the one-wire bus.  Other methods
 *  have caused the device to initiate a data transmission. The bit is read by sending
 *  a short pulse of about a microsecond, then floating the bus high and sampling it
 *  after about 15 microseconds. 
 *  NOTE: The read delay was changed from AOWI_45us_D to AOWI_15us_D, 15-Dec-2011, and
 *        an extra 45 microsecond delay was added to keep subsequent actions in synch
 *  @return True if the received bit was a 1, false if it was a 0
 */

bool avr_1wire::read_bit (void)
{
	#ifdef FREERTOS_CONFIG_H
		portENTER_CRITICAL ();
	#endif

	data_outport &= ~data_mask;             // Set output data to 0
	data_ddr |= data_mask;                  // Force data line to output zero
	AOWI_DELAY (AOWI_1us_D);                // Hold it there for about 1 us
	data_ddr &= ~data_mask;                 // Let the line go to logic 1
	data_outport |= data_mask;              // Turn pullup resistor on
	AOWI_DELAY (AOWI_15us_D);               // Wait for about 15 us
	uint8_t temp_byte = data_inport;        // Read the byte in the data port
	AOWI_DELAY (AOWI_45us_D);               // Wait for ~45 us before other action

	#ifdef FREERTOS_CONFIG_H
		portEXIT_CRITICAL ();
	#endif

	// The value returned is what's found on the bit in question
	return (bool (temp_byte & data_mask));
}


//-------------------------------------------------------------------------------------
/** This method writes a byte to the one-wire bus, least significant bit first. It 
 *  writes each of the 8 bits in turn.
 *  @param the_byte The byte (usually a command) which is to be written to the bus
 *  @return True always because there's no timeout
 */

bool avr_1wire::write_byte (uint8_t the_byte)
{
	for (uint8_t bmask = 0x01; bmask > 0; bmask <<= 1)
	{
		if (the_byte & bmask)               // Check each bit in the byte and send a
		{                                   // 0 or 1 as appropriate
			write_1 ();
		}
		else
		{
			write_0 ();
		}
	}
	return (true);
}


//-------------------------------------------------------------------------------------
/** This method writes a byte to the one-wire bus, most significant bit first. It just 
 *  writes each of the 8 bits in the given byte in turn. 
 *  @param the_byte The byte (usually a command) which is to be written to the bus
 *  @return True if the acknowledgement bit occurred, false if we timed out instead
 */

bool avr_1wire::write_byte_rev (uint8_t the_byte)
{
	for (uint8_t bmask = 0x80; bmask > 0; bmask >>= 1)
	{
		if (the_byte & bmask)               // Check each bit in the byte and send a
		{                                   // 0 or 1 as appropriate
			write_1 ();
		}
		else
		{
			write_0 ();
		}
	}
	return (true);
}


//-------------------------------------------------------------------------------------
/** This method issues a Match ROM command, seeking a match with the device whose 
 *  identifier is at the given location in the identifier table. This method should be
 *  called right after reset(). 
 *  @param index The number of the device's identifier in the table
 */

void avr_1wire::match_ROM (uint8_t index)
{
	write_byte (0x55);

	// Let's try this one bit at a time
	for (uint8_t bit = 0; bit < 64; bit++)
	{
		if (get_ID_bit (index, bit))
		{
			write_1 ();
		}
		else
		{
			write_0 ();
		}
	}
}


//-------------------------------------------------------------------------------------
/** This method reads one byte from a device on the One-Wire bus and stores the result
 *  in the given character. If something goes wrong, it returns true; if no problem is
 *  detected, it returns false.
 *  @param  ch_in Pointer to the character where the data is stored
 *  @return True if there is a problem reading the data, false otherwise. There is
 *          currently no test for correct data, so this method always returns false
 */

bool avr_1wire::read_byte (uint8_t* ch_in)
{
	*ch_in = 0;                             // Zero the character

	for (uint8_t bmask = 0x01; bmask > 0; bmask <<= 1)
	{
		data_outport &= ~data_mask;         // Set data output to zero
		data_ddr |= data_mask;              // Force data line to output zero
		AOWI_DELAY (AOWI_1us_D);            // Hold it there for about 1 us
		data_ddr &= ~data_mask;             // Let the line float 
		data_outport |= data_mask;          // Turn pullup resistor on
		AOWI_DELAY (AOWI_15us_D);           // Wait for about 15 us

		if (data_inport & data_mask)        // If a 1 is read at the port, set
		{                                   // the corresponding data bit to a 1
			*ch_in |= bmask;
		}

		AOWI_DELAY (AOWI_45us_D);			// Wait for another 45 us to be safe
	}
	return (false);
}


//-------------------------------------------------------------------------------------
/** This method reads the 64-bit identifier code from a single device on the bus and
 *  stores that code in the data field 'identifier.' It only works if there is only
 *  one device on the bus; if there are more, we must go through the whole search ROM
 *  procedure to identify all the devices on the bus. 
 */

void avr_1wire::read_ID (void)
{
	write_byte (0x33);                      // Send the "Read ROM" command

	// Read each of the 8 bytes in the identifier
	for (uint8_t index = 0; index < 8; index++)
		read_byte (&((identifiers[0].bytes)[index]));
}


//-------------------------------------------------------------------------------------
/** This method returns one unique 64-bit device identifier from the table of 
 *  identifiers kept by this object. If the table index is out of bounds, zero is
 *  returned instead. 
 *  @param which_one The index number in the table
 *  @return The unique 64-bit identifier of the given device
 */

uint64_t avr_1wire::get_ID (uint8_t which_one)
{
	if (which_one >= AOWI_NUM_IDS)
	{
		return ((uint64_t)0);
	}

	return (identifiers[which_one].num64);
}


//-------------------------------------------------------------------------------------
/** This method finds the index in the device identifier table of the device whose 
 *  identifier number matches the given number. If no such device is in the table, 
 *  0xFF is returned.
 *  @param an_ID The identifier which is to be matched within the table
 *  @return The index of the device ID which we want, or 0xFF if it's not there
 */

unsigned char avr_1wire::find_by_ID (uint64_t an_ID)
{
	uint8_t index;                          // Index into ID table

	// Try to match the given index to each entry in the table
	for (index = 0; index < AOWI_NUM_IDS; index++)
	{
		if ((identifiers[index]).num64 == an_ID)
		{
			return (index);
		}
	}

	// Oops, we didn't find the correct number in the table
	return (0xFF);
}


//-------------------------------------------------------------------------------------
/** This method finds the index in the device identifier table of the device whose 
 *  device type number matches the given number. The device type number is the lowest
 *  byte in the 64-bit identifier for the device. If no such device is in the table, 
 *  0xFF is returned. If two or more devices with a given type are on the same bus, 
 *  the ID of just one of the devices is returned, and the other is ignored. 
 *  @param type_ID The device identifier code to be matched within the table
 *  @return The index of the device ID which we want, or 0xFF if it's not there
 */

uint8_t avr_1wire::find_by_type (uint8_t type_ID)
{
	uint8_t index;							// Index into ID table

	// Try to match the given index to each entry in the table
	for (index = 0; index < AOWI_NUM_IDS; index++)
	{
		if (identifiers[index].bytes[0] == type_ID)
		{
			return (index);
		}
	}

	// Oops, we didn't find the correct number in the table
	return (0xFF);
}


//-------------------------------------------------------------------------------------
/** This method returns the bit in a One-Wire identifier at a given bit position from
 *  0 to 63. Indices are not checked, but this method is not for general use anyway. 
 *  @param which_ID The index of the identifier in the table of identifiers
 *  @param which_bit The position in the number, from 0 to 63
 *  @return The bit which was found, true (1) or false (0)
 */

bool avr_1wire::get_ID_bit (uint8_t which_ID, uint8_t which_bit)
{
	if ((identifiers[which_ID]).bytes[which_bit / 8] & (0x01 << (which_bit % 8)))
	{
		return (true);
	}
	else
	{
		return (false);
	}
}


//-------------------------------------------------------------------------------------
/** This method sets the bit in a One-Wire identifier at a given bit position from
 *  0 to 63 to a given value. Indices are not checked, but this method is not for 
 *  general use anyway. 
 *  @param which_ID The index of the identifier in the table of identifiers
 *  @param which_bit The position in the number, from 0 to 63
 *  @param new_bit The new value for the bit, true (1) or false (0)
 */

void avr_1wire::set_ID_bit (uint8_t which_ID, uint8_t which_bit, bool new_bit)
{
	uint8_t& pByte = (identifiers[which_ID]).bytes[which_bit / 8];
	uint8_t bit_mask = 0x01 << (which_bit % 8);
	if (new_bit)
	{
		pByte |= bit_mask;
	}
	else
	{
		pByte &= ~bit_mask;
	}
}


//-------------------------------------------------------------------------------------
/** This method searches the One-Wire bus for all connected devices. It issues the
 *  "Search ROM" command, then makes guesses about the bits in the devices's ID
 *  numbers, one bit at a time; the devices each respond with "right" or "wrong" 
 *  indications which are wired-OR'ed with each other, allowing the master to know if
 *  its guesses were correct or not.  The whole process is a complicated mess described
 *  in http://www.maxim-ic.com/products/ibutton/ibuttons/standard.pdf. 
 */

void avr_1wire::search (void)
{
	uint8_t which_ID;						// Which identifier is being found out
	bool bit_error = false;					// True if a rotten situation occurred
	uint8_t bit;							// Which bit in address is being tested
	bool returned_A, returned_B;			// Two bytes returned by the devices
	bool conflict;							// How many conflicts in each pass
	uint8_t high_conf = 99;					// Highest bit where conflict occurred
	uint8_t last_hcnf;						// Previous value of highest conflict bit

	// First zero all the entries in the device table
	for (uint8_t index = 0; index < AOWI_NUM_IDS; index++)
	{
		identifiers[index].num64 = (uint64_t)0;
	}

	// Scan for device identifiers until all devices have been found
	for (which_ID = 0; which_ID < AOWI_NUM_IDS; which_ID++)
	{
		if (!reset ())                              // Send and check reset pulse
		{
			return;
		}
		write_byte (0xF0);                          // Send the "Read ROM" command
		conflict = false;                           // No bit conflicts seen yet
		last_hcnf = high_conf;                      // Save position of highest con-
		high_conf = 99;                             // flict for previous device

		for (bit = 0; bit < 64 && !bit_error; bit++)
		{
			returned_A = read_bit ();               // See what two bits are sent by
			returned_B = read_bit ();               // the device(s) on the bus

			if (!returned_A && !returned_B)         // 00 means a conflict; if 
			{                                       // last_hcnf is this bit, choose
                                                    // 1 for this bit because the
                                                    // previous device had 0 here, 
				if (last_hcnf == bit)               // and this is a resolved conflict
				{
					write_1 ();
					set_ID_bit (which_ID, bit, true);
				}
				else                                // This is a new, unresolved
				{                                   // conflict 
					high_conf = bit;
					conflict = true;
					write_0 ();
				}
			}
			else if (returned_A && !returned_B)     // 10 means all devices have 1
			{                                       // in this position
				set_ID_bit (which_ID, bit, true);
				write_1 ();
			}
			else if (!returned_A && returned_B)     // 01 means all devices have 0
			{                                       // in this position
				write_0 ();
			}
			else                                    // 11 means an error; probably 
			{                                       // no device is on the bus
				bit_error = true;
				DBG (p_serial, PMS ("1-Wire bit error, pin ") << hex << data_mask
					 << dec << " bit " << bit << endl);
			}
		}

		// If we're done, we need to exit this loop
		if (!conflict || bit_error) break;
	}
}


//-------------------------------------------------------------------------------------
/** This method displays a list of devices which have been found on the 1-wire bus. 
 *  They must have previously been found by search(). 
 *  @param debug_port A pointer to a serial object on which to display the results
 *  @param how_many The number of devices to show. If there aren't as many devices as
 *      asked for, a bunch of zeros will be displayed for the absent devices. 
 *      (Default: AOWI_NUM_IDS, the number of elements in the device ID array)
 */

void avr_1wire::show_devices (emstream* debug_port, uint8_t how_many)
{
	// Never attempt to display something on a nonexistent serial device
	if (debug_port == NULL)
	{
		return;
	}

	// The maximum number of device ID's is the number the array can hold
	if (how_many > AOWI_NUM_IDS)
	{
		how_many = AOWI_NUM_IDS;
	}

	*debug_port << hex;
	for (uint8_t fred = 0; fred < how_many; fred++)
	{
		*debug_port << get_ID (fred) << endl;
	}
	*debug_port << dec;
}


//-------------------------------------------------------------------------------------
/** This method tries to accomplish a successful reset of the 1-wire device by varying
 *  the timing of 1-wire signals. 
 *  @return True if a timing was found which works, false if not
 */

bool avr_1wire::auto_timing (void)
{
// 	uint16_t presence_pulse_wait;           // How long to wait for a presence pulse
	volatile uint32_t end_tout;             // Counter used to check for timeout

	// Begin by trying reset pulses of short duration, increasing the duration until
	// a presence pulse is detected from the one-wire device
	for (reset_pulse_dur = 4; reset_pulse_dur < 100; reset_pulse_dur += 2)
	{
		data_outport &= ~data_mask;         // Put a zero on the output line
		data_ddr |= data_mask;              // Force data line to output zero
		AOWI_DELAY (reset_pulse_dur);       // Wait a while

		data_ddr &= ~data_mask;             // Set data direction to input
		data_outport |= data_mask;          // Turn pullup resistor on
		AOWI_DELAY (AOWI_PRES_D);           // Wait before checking for presence pulse

		if (data_inport & data_mask)
		{
			DBG (p_serial, PMS ("RST ") << reset_pulse_dur << PMS (" bad, "));
		}
		else
		{
			DBG (p_serial, PMS ("RST ") << reset_pulse_dur << PMS (" OK") << endl);
			break;
		}
	}

	// Add just a little to be on the safe side
	reset_pulse_dur += 2;

	data_ddr &= ~data_mask;                 // Set data direction to input
	data_outport |= data_mask;              // Turn pullup resistor on

	// Send a low pulse of at least 480 us
	data_outport &= ~data_mask;             // Put a zero on the output line
	data_ddr |= data_mask;                  // Force data line to output zero
	AOWI_DELAY (AOWI_RESET_D);              // Wait a while
	data_ddr &= ~data_mask;                 // Set data direction to input
    data_outport |= data_mask;              // Turn pullup resistor on
	AOWI_DELAY (AOWI_PRES_D);               // Wait before checking for presence pulse

	// If there's a logic 1 immediately after the presence pulse, nobody's home
	if (data_inport & data_mask)
		{
		DBG (p_serial, PMS ("1W: no pres. 0b") << bin << data_mask << dec << endl);
		return false;
		}

	for (end_tout = 0; (data_inport & data_mask) == 0; end_tout++)
		{
		if (end_tout >= AOWI_PRES_END)
			{
			DBG (p_serial, PMS ("1W: RST tout 0b") << bin << data_mask << dec << endl);
			return false;
			}
		}
	DBG (p_serial, PMS ("1W: RST OK 0b") << bin << data_mask << dec << endl);

	AOWI_DELAY (AOWI_PRES_D);				// Wait before doing the next thing
	return (true);
}

