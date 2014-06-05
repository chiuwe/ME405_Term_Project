#ifndef Solenoid_h
#define Solenoid_h

#include <util/delay.h>

#include "emstream.h"                       // Header for serial ports and devices
#include "FreeRTOS.h"                       // Header for the FreeRTOS RTOS
#include "task.h"                           // Header for FreeRTOS task functions
#include "queue.h"                          // Header for FreeRTOS queues
#include "semphr.h"                         // Header for FreeRTOS semaphores
#include "frt_queue.h"                      // Header of wrapper for FreeRTOS queues
#include "frt_shared_data.h"                // Header for thread-safe shared data
#include "frt_text_queue.h"                 // Header for text queue class
#include "shares.h"

// library interface description
class Solenoid {
  public:
    // constructors:
    Solenoid( emstream *p_serial_port, 
                  uint8_t activationPin,
                  volatile uint8_t *p_ddr,
                  volatile uint8_t * pPort);

    void release();
  protected:
    void myDelayMS(uint64_t waitTime);
    
    /// The motor driver class uses this pointer print to the serial port.
    emstream* ptr_to_serial;

    // motor pin numbers:
    uint8_t activation_Pin;

    volatile uint8_t * p_port;
    
};

#endif

