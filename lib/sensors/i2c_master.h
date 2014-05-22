//*************************************************************************************
/** \file i2c_master.h
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

// This define prevents this file from being included more than once in a *.cpp file
#ifndef _I2C_MASTER_H_
#define _I2C_MASTER_H_

#include <stdlib.h>                         // Standard C/C++ library stuff
#include "emstream.h"                       // Header for base serial devices


/// This is the desired bit rate for the I2C interface in bits per second.
#define I2C_BITRATE         100000L

/// This value is put in the TWBR register to set the desired bitrate. 
const uint8_t I2C_TWBR_VALUE = (((F_CPU / I2C_BITRATE) - 16) / 2);


//-------------------------------------------------------------------------------------
/** \brief This class is a simple driver for an I2C (also known as TWI) bus on an AVR
 *  processor. 
 *  \details It encapsulates basic I2C functionality such as the ability to send and
 *  receive bytes through the TWI bus. Currently only operation of the AVR as an I2C
 *  bus master is supported; this is what's needed for the AVR to interface with most
 *  I2C based sensors. 
 */

class i2c_master
{
protected:
	/// This is a pointer to a serial port object which is used for debugging the code.
	emstream* p_serial;

public:
	// This constructor sets up the driver
	i2c_master (emstream* = NULL);

	// This destructor doesn't exist...psych

	// This method causes a start condition on the TWI bus
	void start (void);

	// This method causes a repeated start on the TWI bus
	void repeated_start (void);

	/** This method causes a stop condition on the I2C bus. It's inline because 
	 *  causing a stop condition is a one-liner (in C++ and even in assembly). 
	*/
	void stop (void)
	{
		TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
	}

	// This method sends a byte out the TWI bus
	bool send (uint8_t, uint8_t);

	// This method receives a byte from the TWI bus
	uint8_t receive (bool);
};

#endif // _I2C_MASTER_H_

