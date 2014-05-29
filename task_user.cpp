//**************************************************************************************
/** \file task_user.cpp
 *    This file contains source code for a user interface task for a ME507/FreeRTOS
 *    test suite. 
 *
 *  Revisions:
 *    \li 09-30-2012 JRR Original file was a one-file demonstration with two tasks
 *    \li 10-05-2012 JRR Split into multiple files, one for each task
 *    \li 10-25-2012 JRR Changed to a more fully C++ version with class task_user
 *    \li 11-04-2012 JRR Modified from the data acquisition example to the test suite
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
//**************************************************************************************

#include <avr/io.h>							// Port I/O for SFR's
#include <avr/wdt.h>							// Watchdog timer header

#include "nRF24L01_text.h"					// Header for Nordic Semi radio module

#include "task_user.h"						// Header for this file


/** This constant sets how many RTOS ticks the task delays if the user's not talking.
 *  The duration is calculated to be about 5 ms.
 */
const portTickType ticks_to_delay = ((configTICK_RATE_HZ / 1000) * 5);
/// Determines which motor is being selected in the user interface.
bool motor_select;


//-------------------------------------------------------------------------------------
/** This constructor creates a new data acquisition task. Its main job is to call the
 *  parent class's constructor which does most of the work.
 *  @param a_name A character string which will be the name of this task
 *  @param a_priority The priority at which this task will initially run (default: 0)
 *  @param a_stack_size The size of this task's stack in bytes 
 *                      (default: configMINIMAL_STACK_SIZE)
 *  @param p_ser_dev Pointer to a serial device (port, radio, SD card, etc.) which can
 *                   be used by this task to communicate (default: NULL)
 */

task_user::task_user (const char* a_name, 
					  unsigned portBASE_TYPE a_priority, 
					  size_t a_stack_size,
					  emstream* p_ser_dev
					 )
	: frt_task (a_name, a_priority, a_stack_size, p_ser_dev)
{
	// Save a pointer to the serial port so it can be used to communicate with the user
	p_serial = p_ser_dev;

	// Initialize the runs counter
	runs = 0;
}


//-------------------------------------------------------------------------------------
/** This task is the main loop that runs the menu. You can chose options from 
	the main menu, including printing the time, version, and stack, or you can
	go to the motor menu implemented in motor_menu()
 */

void task_user::run (void)
{
	char char_in;							// Character read from serial device
	time_stamp a_time;						// Holds the time so it can be displayed

	// Right before running the loop, print the help message so the user knows which 
	// keys to press to do something
	print_help_message ();

	// This is the task loop. Once the task has been initialized in the code just
	// above, the task loop runs, and it keeps running until the power is shut off.
	// In the loop, we check for characters typed into the serial port and also check
	// for characters in the print queue; those characters are to be displayed
	for (;;)
	{
		// Check for any characters which might have been typed by the user
		if (p_serial->check_for_char ())
		{
			char_in = p_serial->getchar ();			// Read the character

			switch (char_in)
			{
				// The 't' command asks what time it is now
				case 'n':
					*p_serial << (a_time.set_to_now ()) << endl;
					break;

				// The 's' command means dump all the tasks' stacks for examination
				case 's':
					print_task_stacks (p_serial);
					break;

				// Pressing 'v' gives the version number and setup of this program
				case 'v':
					show_status ();
					break;

				// Pressing 'm' gives the motor setting menu
				case 'm':
				   motor_menu ();
				   motor_settings ();
				   break;

				// A '?' or 'h' is a plea for help; respond with a help message
				case '?':
				case 'h':
					print_help_message ();
					break;

				// The number 3 is control-C, which means reset the AVR processor
				case (3):
					*p_serial << PMS ("Resetting AVR") << endl;
					wdt_enable (WDTO_120MS);
					for (;;)
					{
					}
					break;

				// If the character isn't recognized, ask: What's That Function?
				default:
					p_serial->putchar (char_in);
					*p_serial << PMS (":WTF?") << endl;
					break;
			};
		}
		// If no character was typed in, check the print queue to see if another task
		// has sent this task something to be printed
		else if (print_ser_queue->check_for_char ())
		{
			p_serial->putchar (print_ser_queue->getchar ());
		}
		// If no character has been typed and no other task sent us something to print, 
		// wait for approximately a millisecond before we check again
		else	
		{
			vTaskDelay (configTICK_RATE_HZ / 1000);
		}

		// We've made it safely through the loop one more time; claim some credit
		runs++;
	}
}


//-------------------------------------------------------------------------------------
/** This method prints out the diferent options for the main menu imlpmented in run(). 
	It shows the possible inputs it accepts as well as what they do.
 */

void task_user::print_help_message (void)
{
	*p_serial << PMS ("FreeRTOS Task Communications Test Program help") << endl;
	*p_serial << PMS (" n:  Show the real time NOW") << endl;
	*p_serial << PMS (" v:  Show program version and setup") << endl;
	*p_serial << PMS (" s:  Dump all tasks' stacks") << endl;
	*p_serial << PMS (" m:  Click this for total control. Muahahaha") << endl;
	*p_serial << PMS (" h:  Print this help message") << endl;
	*p_serial << PMS ("^C:  Reboot the AVR") << endl;
}


//-------------------------------------------------------------------------------------
/** This method displays information about the status of the system, including
 *    \li The name and version of the program
 *    \li The name, status, priority, and free stack space of each task
 *    \li Processor cycles used by each task
 *    \li Amount of heap space free and setting of RTOS tick timer
 */

void task_user::show_status (void)
{
	time_stamp the_time;					// Holds current time for printing

	*p_serial << endl << PROGRAM_VERSION << PMS (__DATE__) << endl 
			  << PMS ("System time: ") << the_time.set_to_now () << endl << endl;

	// Have the tasks print their status
	print_task_list (p_serial);

	// Show free heap and the configured heap size
	*p_serial << PMS ("Heap: ") << heap_left() << "/" << configTOTAL_HEAP_SIZE;

	// Show how the timer/counter is set up to cause RTOS timer ticks
	#ifdef OCR5A
		*p_serial << PMS (", OCR5A=") << OCR5A << endl;
	#elif (defined OCR3A)
		*p_serial << PMS (", OCR3A=") << OCR3A << endl;
	#else
		*p_serial << PMS (", OCR1A=") << OCR1A << endl;
	#endif
}


// void task_user::run_vTaskList (void)
// {
// 	// If task execution time profiling is turned on, show the report
// 	#if (configGENERATE_RUN_TIME_STATS == 1)
// 		*p_serial << PMS ("Task\t\tCycles\t\t% Time") << endl;
// 		*p_serial << PMS ("----\t\t------\t\t------");
// 		vTaskGetRunTimeStats ((signed char*)task_info_buffer);
// 		*p_serial << task_info_buffer << endl;
// 	#endif // configGENERATE_RUN_TIME_STATS
// }


//-------------------------------------------------------------------------------------
/** This method prints information about the status of this task. It is called by the
 *  overloaded "<<" operator so that when the task prints itself to a serial device,
 *  whatever this method wants printed gets printed. 
 *  @param ser_thing The serial device to which information will be printed
 */

void task_user::print_status (emstream& ser_thing)
{
	// Call the parent task's printing function first
	frt_task::print_status (ser_thing);

	// Now add the additional data
	ser_thing << "\t " << runs << PMS (" runs");
}

/**
	This method prints out the diferent options for the motor menu imlpmented in run(). 
	It shows the possible inputs it accepts as well as what they do.
 */
void task_user::motor_menu (void)
{
   *p_serial << PMS ("Motor Settings") << endl;
  	*p_serial << PMS (" w:  Set number of steps (<0 for backwards)") << endl;
  	*p_serial << PMS (" f:  FIRE!!!!") << endl;
	*p_serial << PMS (" h:  print this help message") << endl;
	*p_serial << PMS (" x:  Exit motor setting menu") << endl;
}

/**
	This function is the implementation of the motor menu. Given correct inputs it will let you
	change motor, change pot state, change power settings, run the brakes, and switch back to
	the main menu. Incorrect input are more or less ignored
*/
void task_user::motor_settings (void)
{
	char char_in;
	char buf[4];
	int i = 0;
	int num;
	bool exit = false;

   while (!exit) {
   	if (p_serial->check_for_char()) {
			char_in = p_serial->getchar();

			switch (char_in)
			{
				case 'w':
				   *p_serial << PMS ("Enter number of steps: ");
				   i = 0;
				   while (!p_serial->check_for_char());
				   while ((char_in = p_serial->getchar()) != '\r')
				   {
				   	buf[i] = char_in;
				   	i++;
				   	*p_serial << char_in;
				   	while (!p_serial->check_for_char());
				   }
				   *p_serial << endl;
				   // TODO: no error checking yet...
				   num = strtol(buf, NULL, 10);
				   p_numSteps->put(num);
					break;
				case 'f':
					p_fire->put(true);
					break;
				case 'x':
					*p_serial << PMS ("Returning to main...") << endl;
					exit = true;
				   break;
				case 'h':
					motor_menu();
					break;
				default:
					p_serial->putchar (char_in);
					*p_serial << PMS (":WTF?") << endl;
				   break;
			}
		}
   }
}

