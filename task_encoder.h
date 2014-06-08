/** \file task_encoder.cpp
 * This class is basically a wrapper for the encoder_driver. It creates a new
 * encoder driver, and loops forever waiting for the encoder_driver ISR to do
 * something. Outputs to the serial queue to show how the encoder_driver is doing 
*/

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _TASK_ENCODER_H_
#define _TASK_ENCODER_H_

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
#include "shares.h"
#include "encoder_driver.h"


/** 
 * \brief This task creates a new encoder_driver and outputs it's status on a regular basis.
 */
class task_encoder : public frt_task
{
private:

protected:
   /// A pointer to the driver encoder.
   encoder_driver* encoder;

public:
   uint32_t runs;                   ///< How many times through the task loop

   // This constructor creates a generic task of which many copies can be made
   task_encoder (const char*, unsigned portBASE_TYPE, size_t, emstream*, uint8_t, uint8_t);

   /** This run method is called by the RTOS and contains a loop in which the task
    *  checks for data and sends it if appropriate.
    */
   void run (void);

   // Print how this task is doing on its tests
   void print_status (emstream&);
};

#endif // _TASK_ENCODER_H_
