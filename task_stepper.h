//**************************************************************************************
/** \file task_motor.h
 *    This file contains the header for motor controller class which controls speed and
 *    direction of a motor using a voltage measured from the A/D as input. One button
 *    will trigger stop and go. A second button will determine which motor is being
 *    controlled.*/
//**************************************************************************************

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _TASK_STEPPER_H_
#define _TASK_STEPPER_H_

#include <stdlib.h>                    // Prototype declarations for I/O functions

#include "FreeRTOS.h"                  // Primary header for FreeRTOS
#include "task.h"                      // Header for FreeRTOS task functions
#include "queue.h"                     // FreeRTOS inter-task communication queues

#include "frt_task.h"                  // ME405/507 base task class
#include "rs232int.h"                  // ME405/507 library for serial comm.
#include "time_stamp.h"                // Class to implement a microsecond timer
#include "frt_queue.h"                 // Header of wrapper for FreeRTOS queues
#include "frt_shared_data.h"           // Header for thread-safe shared data
#include "Stepper.h"
#include "adc.h"


//-------------------------------------------------------------------------------------
/** This task determines what commands to send to the motor driver.
 */

class task_stepper : public frt_task
{
private:

protected:
   /// A pointer to motor_driver.
   Stepper* driver;

   /// A pointer to the brake shared data.
   shared_data<int64_t>* speed;

   /// A pointer to the power shared data.
   shared_data<int16_t>* numSteps;

public:
   uint32_t runs;                   ///< How many times through the task loop

   // This constructor creates a generic task of which many copies can be made
   task_stepper (const char* a_name, 
                         unsigned portBASE_TYPE a_priority, 
                         size_t a_stack_size,
                         emstream* p_ser_dev,
                         Stepper* p_driver,
                         shared_data<int64_t>* p_speed,
                         shared_data<int16_t>* p_numSteps
                        );
   /** This run method is called by the RTOS and contains a loop in which the task
    *  checks for data and sends it if appropriate.
    */
   void run (void);

   // Print how this task is doing on its tests
   void print_status (emstream&);
};

#endif // _TASK_STEPPER_H_
