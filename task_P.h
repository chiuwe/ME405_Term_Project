//**************************************************************************************
/** \file task_P.h
 *    This file contains the header for the logic that gives a perportional control to 
 *    one specific motor. The values are hardCoded to work with the PittMan motor using
 *    no more then one full rotation*/
//**************************************************************************************

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _TASK_P_H_
#define _TASK_P_H_

#include <stdlib.h>                    // Prototype declarations for I/O functions

#include "FreeRTOS.h"                  // Primary header for FreeRTOS
#include "task.h"                      // Header for FreeRTOS task functions
#include "queue.h"                     // FreeRTOS inter-task communication queues

#include "frt_task.h"                  // ME405/507 base task class
#include "rs232int.h"                  // ME405/507 library for serial comm.
#include "time_stamp.h"                // Class to implement a microsecond timer
#include "frt_queue.h"                 // Header of wrapper for FreeRTOS queues
#include "frt_shared_data.h"           // Header for thread-safe shared data
#include "frt_text_queue.h"            // Header for text queue class
#include "shares.h"                         // Shared inter-task communications
#include "task_motor.h"               //motor driver wrapper

//-------------------------------------------------------------------------------------
/** \brief Starts up a new task and grabs the pointer to the motor it will be manipulating
 */

class task_P : public frt_task
{
private:
  
protected:
	motor_driver* motor;

public:

   // This constructor creates a generic task of which many copies can be made
   task_P (const char* a_name, 
                         unsigned portBASE_TYPE a_priority, 
                         size_t a_stack_size,
                         emstream* p_ser_dev,
                         motor_driver *mDriver
                        );

   /** This run method is called by the RTOS and contains a loop in which the task
    *  checks if the motor needs to change possition. 
    */
   void run (void);
};

#endif // _TASK_P_H_
