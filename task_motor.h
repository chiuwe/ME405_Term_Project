//**************************************************************************************
/** \file task_motor.h
 *    This file contains the header for motor controller class which controls speed and
 *    direction of a motor using a voltage measured from the A/D as input. One button
 *    will trigger stop and go. A second button will determine which motor is being
 *    controlled.*/
//**************************************************************************************

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _TASK_MOTOR_H_
#define _TASK_MOTOR_H_

#include <stdlib.h>                    // Prototype declarations for I/O functions

#include "FreeRTOS.h"                  // Primary header for FreeRTOS
#include "task.h"                      // Header for FreeRTOS task functions
#include "queue.h"                     // FreeRTOS inter-task communication queues

#include "frt_task.h"                  // ME405/507 base task class
#include "rs232int.h"                  // ME405/507 library for serial comm.
#include "time_stamp.h"                // Class to implement a microsecond timer
#include "frt_queue.h"                 // Header of wrapper for FreeRTOS queues
#include "frt_shared_data.h"           // Header for thread-safe shared data
#include "motor_driver.h"
#include "adc.h"


//-------------------------------------------------------------------------------------
/** \brief This task determines what commands to send to the motor driver.
 */

class task_motor : public frt_task
{
private:

protected:
   /// Brake pin mask.
   uint8_t brake_pin;

   /// Value for which adc should read from.
   uint8_t adc_select;

   /// A pointer to motor_driver.
   motor_driver* driver;

   /// A pointer to the brake shared data.
   shared_data<bool>* brake;

   /// A pointer to the power shared data.
   shared_data<int16_t>* power;

   /// A pointer to the potentiometer shared data.
   shared_data<bool>* pot;


public:
   uint32_t runs;                   ///< How many times through the task loop

   // This constructor creates a generic task of which many copies can be made
   task_motor (const char*, unsigned portBASE_TYPE, size_t, uint8_t, motor_driver*, shared_data<bool>*, shared_data<int16_t>*, shared_data<bool>*, uint8_t, emstream*);

   /** This run method is called by the RTOS and contains a loop in which the task
    *  checks for data and sends it if appropriate.
    */
   void run (void);

   // Print how this task is doing on its tests
   void print_status (emstream&);
};

#endif // _TASK_MOTOR_H_
