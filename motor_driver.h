//======================================================================================
/** \file motor_driver.h
 *    This file provides the creation and interface for running a simple motor. You can
 *    change the power, which will take you from full forward to full reverse. If you 
 *    put the power at half it will freewheel. You can also break at any time.
 */
//======================================================================================

// This define prevents this .H file from being included multiple times in a .CPP file
#ifndef _AVR_MOTOR_DRIVER_H_
#define _AVR_MOTOR_DRIVER_H_

#include "emstream.h"                       // Header for serial ports and devices
#include "FreeRTOS.h"                       // Header for the FreeRTOS RTOS
#include "task.h"                           // Header for FreeRTOS task functions
#include "queue.h"                          // Header for FreeRTOS queues
#include "semphr.h"                         // Header for FreeRTOS semaphores


//-------------------------------------------------------------------------------------
/** \brief This class runs the motor on the h-bridge chip.
 *  \details This class takes in several ports and their inputs and masks for a 
 *  specific motherboard, as well as a serial port.
 */

class motor_driver
{
   protected:
    /// The motor driver class uses this pointer print to the serial port.
    emstream* ptr_to_serial;
    /// This points to the compare register.
    volatile uint16_t *compare;
    /// This points to the direction register.
    volatile uint8_t *direction;
    /// Enable pin on h-bridge chip.
    uint8_t enable;

   public:
      // The constructor sets up the motor for use. It takes in the serial port, and various registers and their inputs/masks.
      motor_driver (emstream *p_serial_port,
                   volatile uint8_t *p_ddr,
                   uint8_t ddr_mask, 
                   volatile uint8_t *pwm,
                   uint8_t pwm_mask,
                   volatile uint8_t *p_port,
                   uint8_t enable_mask,
                   volatile uint8_t *p_tccra,
                   uint8_t tccra_mask,
                   volatile uint8_t *p_tccrb,
                   uint8_t tccrb_mask,
                   volatile uint16_t *p_ocr);
      
      void set_power (int16_t power);

      void brake (void);

}; // end of class motor driver


#endif // _AVR_MOTOR_DRIVER_H_
