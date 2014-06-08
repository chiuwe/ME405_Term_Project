/** \file encoder_driver.cpp
 * Class for creating a way to check on a moter encoder. Initializes the pin to use
 * for interupts in the constructer, and creates an ISR to monitor the motor functionality.
 * Comes with a couple of getter and setter functions
 */

// This define prevents this .H file from being included multiple times in a .CPP file
#ifndef _AVR_ENCODER_DRIVER_H_
#define _AVR_ENCODER_DRIVER_H_

#include "emstream.h"                       // Header for serial ports and devices
#include "FreeRTOS.h"                       // Header for the FreeRTOS RTOS
#include "task.h"                           // Header for FreeRTOS task functions
#include "queue.h"                          // Header for FreeRTOS queues
#include "semphr.h"                         // Header for FreeRTOS semaphores
#include "frt_queue.h"                      // Header of wrapper for FreeRTOS queues
#include "frt_shared_data.h"                // Header for thread-safe shared data
#include "frt_text_queue.h"                 // Header for text queue class
#include "shares.h"

//-------------------------------------------------------------------------------------
/** \brief This class reads the input from an encoder.
 *  \details This class takes in a port and its inputs and masks for a 
 *  specific motherboard, as well as a serial port.
 */

class encoder_driver {
   protected:
      /// The encoder driver class uses this pointer print to the serial port.
      emstream* ptr_to_serial;

   public:
      /// Constructor initializes the pin, and the rest are getter and setter functions
      encoder_driver (emstream *p_serial_port, uint8_t bit, uint8_t trigger);
      int32_t get_count (void);
      void zero (void);
      void set_position (int32_t);

}; // end of class encoder driver


#endif // _AVR_ENCODER_DRIVER_H_
