//**************************************************************************************
/** \file task_user.h
 *    This file contains header stuff for a user interface task for a ME507/FreeRTOS
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

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _TASK_USER_H_
#define _TASK_USER_H_

#include <stdlib.h>							// Prototype declarations for I/O functions

#include "FreeRTOS.h"						// Primary header for FreeRTOS
#include "task.h"							// Header for FreeRTOS task functions
#include "queue.h"							// FreeRTOS inter-task communication queues

#include "rs232int.h"						// ME405/507 library for serial comm.
#include "time_stamp.h"						// Class to implement a microsecond timer
#include "frt_task.h"						// Header for ME405/507 base task class
#include "frt_queue.h"						// Header of wrapper for FreeRTOS queues
#include "frt_text_queue.h"					// Header for a "<<" queue class
#include "frt_shared_data.h"				// Header for thread-safe shared data

#include "shares.h"							// Global ('extern') queue declarations


/// This macro defines a string that identifies the name and version of this program. 
#define PROGRAM_VERSION		PMS ("PolyDAQ/FreeRTOS Test V0.2 ")


//-------------------------------------------------------------------------------------
/** This task reads measurements that were taken by the data acquisition task in
 *  task_daq.* and puts those measurements into a queue. 
 */

class task_user : public frt_task
{
private:

protected:
	/** This pointer allows all the methods in this task to use the serial port, or
	 *  whatever serial device is being used to communicate with the user.
	 */
	emstream* p_serial;

	// This method displays a simple help message telling the user what to do. It's
	// protected so that only methods of this class or possibly descendents can use it
	void print_help_message (void);

	/// \cond NON_INCLDD This method displays cryptic messages involving obfuscation.
	void vernal_equinox (void);
	/// \endcond

	/// This variable counts the number of runs through this task's loop.
	uint32_t runs;

	// This method displays information about the status of the system
	void show_status (void);

   void motor_menu (void);
	void motor_settings (void);

public:
	// This constructor creates a user interface task object
	task_user (const char*, unsigned portBASE_TYPE, size_t, emstream*);

	/** This run method is called by the RTOS and contains a loop in which the task
	 *  checks for user input and characters in the print queue, dealing with each in
	 *  the appropriate manner. 
	 */
	void run (void);

	// Print how this task is doing on its tests
	void print_status (emstream&);
};

#endif // _TASK_USER_H_
