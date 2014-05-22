//*************************************************************************************
/** \file adc.cpp
 *    This file contains a very simple A/D converter driver. The driver is hopefully
 *    thread safe in FreeRTOS due to the use of a mutex to prevent its use by multiple
 *    tasks at the same time. There is no protection from priority inversion, however,
 *    except for the priority elevation in the mutex.
 *
 *  Revisions:
 *    \li 01-15-2008 JRR Original (somewhat useful) file
 *    \li 10-11-2012 JRR Less original, more useful file with FreeRTOS mutex added
 *    \li 10-12-2012 JRR There was a bug in the mutex code, and it has been fixed
 *
 *  License:
 *    This file is copyright 2012 by JR Ridgely and released under the Lesser GNU 
 *    Public License, version 2. It intended for educational use only, but its use
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

#include <stdlib.h>                         // Include standard library header files
#include <avr/io.h>

#include "rs232int.h"                       // Include header for serial port class
#include "adc.h"                            // Include header for the A/D class


//-------------------------------------------------------------------------------------
/** \brief This constructor sets up an A/D converter. 
 *  \details \b Details: The A/D converter is enabled and the division factor is set to 32.
 *  @param p_serial_port A pointer to the serial port where debugging info is written. 
 */

adc::adc (emstream* p_serial_port) {  
	ptr_to_serial = p_serial_port;
   
   // Set AVCC with external capacitor at AREF pin.
	ADMUX = (1 << REFS0);
	ADMUX_init = ADMUX;
	
	// Enable ADC and set prescaler to 32.
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS0);

	// Print a handy debugging message
  	// DBG (ptr_to_serial, "A/D constructor OK" << endl);
}


//-------------------------------------------------------------------------------------
/** \brief This method takes one A/D reading from the given channel and returns it. 
 *  \details \b Details: This method selects a channel to read and goes through the A/D conversion.
 *  This method also guards against time-out.
 *  @param  ch The A/D channel which is being read must be from 0 to 7.
 *  @return The result of the A/D conversion.
 */

uint16_t adc::read_once (uint8_t ch) {
   uint8_t i = 0;
   uint16_t result;

   ADMUX = ADMUX_init;
   ADMUX |= ch;

   // start conversion with ADSC
   ADCSRA |= (1 << ADSC);
   while (ADCSRA & (1 << ADSC) && i < 65) {
      i++;
   }
   
   result = ADCL | (ADCH << 8);
   //DBG (ptr_to_serial, "result: " << result << endl);
   return i == 65 ? -1 : result;
}


//-------------------------------------------------------------------------------------
/** \brief This method finds the average result while guarding against overflow.
 *  \details \b Details: This method first checks to see whether the number of samples 
 *  will threaten overflow and then saturates the number of samples if necessary. It 
 *  then takes the average.
 *  @param channel A selected channel to read from.
 *  @param samples The chosen number of samples.
 *  @return The average readings from a chosen number of samples.
 */

uint16_t adc::read_oversampled (uint8_t channel, uint8_t samples) {
	uint16_t sum = 0;
   uint8_t i;

   if (samples > 32) {
      samples = 32;
   }
   
   for (i = 0; i < samples; i++) {
      sum += read_once(channel);
   }

   //DBG (ptr_to_serial, "average: " << sum / samples << endl);
   return sum / samples;
}


//-------------------------------------------------------------------------------------
/** \brief This overloaded operator "prints" the A/D converter. 
 *  \details \b Details: This overloaded operator will return ADMUX and ADCSRA in binary.
 *  @param serpt The serial port where the printout will be printed.
 *  @param a2d The A/D driver which is being printed.
 *  @return A reference to the same serial device that the information is written to.
 *          This is used to string together things to write with "<<" operators
 */

emstream& operator << (emstream& serpt, adc& a2d) {

	serpt << "ADMUX: " << bin << ADMUX << endl;
   serpt << "ADCSRA: " << bin << ADCSRA << endl;

	return (serpt);
}

