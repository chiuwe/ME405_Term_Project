//*************************************************************************************
/** \file i2c_master.cpp
 *    This file contains a base class for classes that use the I2C (also known as TWI)
 *    interface on an AVR. The terms "I2C" (the two means squared) and "TWI" are
 *    essentially equivalent; Philips has trademarked the former, and Atmel doesn't pay
 *    them a license fee, so Atmel chips that meet exactly the same specification are
 *    not allowed to use the "I2C" name, even though everything works the same. 
 *
 *    Note: The terms "master" and "slave" are standard terminology used in the
 *    electronics industry to describe interactions between electronic components only.
 *    The use of such terms in this documentation is made only for the purpose of 
 *    usefully documenting electronic hardware and software, and such use must not be
 *    misconstrued as diminishing our revulsion at the socially diseased human behavior 
 *    which is described using the same terms, nor implying any insensitivity toward
 *    people from any background who have been affected by such behavior. 
 *
 *  Revised:
 *    \li 12-24-2012 JRR Original file, as a standalone HMC6352 compass driver
 *    \li 12-28-2012 JRR I2C driver split off into a base class for optimal reusability
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
#include "i2c_master.h"                     // Header for this class


//-------------------------------------------------------------------------------------
/** This constructor creates an I2C driver object.
 *  @param p_debug_port A serial port, often RS-232, for debugging text (default: NULL)
 */

i2c_master::i2c_master (emstream* p_debug_port)
{
	p_serial = p_debug_port;                // Set the debugging serial port pointer

	TWBR = I2C_TWBR_VALUE;                  // Set the bit rate for the I2C port
}


//-------------------------------------------------------------------------------------
/** This method causes a start condition on the I2C bus. In hardware, a start condition
 *  means that the SDA line is dropped while the SCL line stays high. This gets the
 *  attention of all the other devices on the bus so that they will listen for their
 *  addresses. 
 */

void i2c_master::start (void)
{
	// Cause the start condition to happen
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	// Wait for the TWINT bit to indicate that the start condition has been completed
	for (uint8_t tntr = 0; !(TWCR & (1 << TWINT)); tntr++)
	{
		if (tntr > 250)
		{
			DBG (p_serial, PMS ("I2C start timeout") << endl);
			break;
		}
	}

	// Check that the start condition was transmitted OK
	if ((TWSR & 0b11111000) != 0x08)
	{
		DBG (p_serial, PMS ("I2C start: 0x") << hex << TWSR << PMS (" not 0x08") 
			 << dec << endl); 
	}
}


//-------------------------------------------------------------------------------------
/** This method causes a repeated start condition on the I2C bus. This is similar to
 *  a regular start condition, except that a different return code is expected if
 *  things go as they should. 
 */

void i2c_master::repeated_start (void)
{
	// Cause the start condition to happen
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	// Wait for the TWINT bit to indicate that the start condition has been completed
	for (uint8_t tntr = 0; !(TWCR & (1 << TWINT)); tntr++)
	{
		if (tntr > 250)
		{
			DBG (p_serial, PMS ("I2C re-start timeout") << endl);
			break;
		}
	}

	// Check that the start condition was transmitted OK
	if ((TWSR & 0b11111000) != 0x10)
	{
		DBG (p_serial, PMS ("I2C re-start: 0x") << hex << TWSR << PMS (" not 0x10") 
			 << dec << endl); 
	}
}


//-------------------------------------------------------------------------------------
/** This method performs an I2C send to transfer a byte to a remote device. The 
 *  expected response code varies depending on what is being sent at what time; some
 *  examples of expected responses are as follows:
 *  \li \c 0x18 - When one has sent SLA+W, a slave address for a write command, and a
 *                good ACK has been received
 *  \li \c 0x40 - When one has sent SLA+R, a slave address for a read command, and a
 *                good ACK has been received
 *  \li \c 0x28 - When one has transmitted a data byte and received a good ACK
 *  @param byte_to_send The byte which is being sent to the remote device
 *  @param expected_response A response byte which the I2C port in the AVR will give 
 *                           if the transfer goes correctly
 *  @return True if the transmission was successful and false if not
 */

bool i2c_master::send (uint8_t byte_to_send, uint8_t expected_response)
{
	TWDR = byte_to_send;
	TWCR = (1 << TWINT) | (1 << TWEN);
	for (uint8_t tntr = 0; !(TWCR & (1 << TWINT)); tntr++)
	{
		if (tntr > 250)
		{
			DBG (p_serial, PMS ("I2C send timeout") << endl);
			return (false);
		}
	}

	// Check that the address thingy was transmitted OK
	if ((TWSR & 0b11111000) != expected_response)
	{
		DBG (p_serial, PMS ("I2C send: 0x") << hex << TWSR << PMS (" not 0x") 
			 << expected_response << dec << endl);
		return (false);
	}
	return (true);
}


//-------------------------------------------------------------------------------------
/** This method receives a byte from the I2C bus. Other code must have already run the
 *  \c start() command and sent and address byte which got the HMC6352's attention.
 *  @param ack True if we end our data request with ACK, telling the slave that we 
 *             want more data after this; false if we end our data request with NACK,
 *             telling the slave that we don't want more data after this
 *  @return The byte which was received from the remote device
 */

uint8_t i2c_master::receive (bool ack)
{
	uint8_t expected_response;              // Code we expect from the AVR's I2C port

	if (ack)  // If we expect more data after this, send an ACK after we get the data
	{
		TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
		expected_response = 0x50;
	}
	else      // We're not going to ask for more data; send a NACK when we're done
	{
		TWCR = (1 << TWINT) | (1 << TWEN);
		expected_response = 0x58;
	}

	for (uint8_t tntr = 0; !(TWCR & (1 << TWINT)); tntr++)
	{
		if (tntr > 250)
		{
			DBG (p_serial, PMS ("I2C receive timeout") << endl);
			break;
		}
	}

	// Check that the address thingy was transmitted OK
	if ((TWSR & 0b11111000) != expected_response)
	{
		DBG (p_serial, PMS ("I2C receive: 0x") << hex << TWSR << PMS (" not 0x") 
			 << expected_response << dec << endl);
	}

	return (TWDR);
}

