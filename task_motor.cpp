//**************************************************************************************
/** \file task_motor.cpp
 *    This file contains the code for a motor controller class which controls speed and
 *    direction of a motor using a voltage measured from the A/D as input. One button
 *    will trigger stop and go. A second button will determine which motor is being
 *    controlled. */
//**************************************************************************************

#include "frt_text_queue.h"                 // Header for text queue class
#include "task_motor.h"                     // Header for this motor controller
#include "shares.h"                         // Shared inter-task communications


//-------------------------------------------------------------------------------------
/** This constructor creates a task which controls the speed of a motor using
 *  input from an A/D converter run through a potentiometer as well as an input from.
 *  a button. The main job of this constructor is to call the
 *  constructor of parent class (\c frt_task ); the parent's constructor the work.
 *  @param a_name A character string which will be the name of this task
 *  @param a_priority The priority at which this task will initially run (default: 0)
 *  @param a_stack_size The size of this task's stack in bytes 
 *                      (default: configMINIMAL_STACK_SIZE)
 *  @param brake_mask Mask for brake value.
 *  @param p_driver   Pointer to a motor driver.
 *  @param p_brake    Pointer to the shared brake boolean.
 *  @param p_power    Pointer to the shared power value.
 *  @param p_pot      Pointer to the shared potentiometer boolean.
 *  @param adc_mask   Mask for which adc to read from.
 *  @param p_ser_dev  Pointer to a serial device (port, radio, SD card, etc.) which can
 *                    be used by this task to communicate (default: NULL)
 *  
 */

task_motor::task_motor (const char* a_name, 
                         unsigned portBASE_TYPE a_priority, 
                         size_t a_stack_size,
                         uint8_t brake_mask,
                         motor_driver* p_driver,
                         shared_data<bool>* p_brake,
                         shared_data<int16_t>* p_power,
                         shared_data<bool>* p_pot,
                         uint8_t adc_mask,
                         emstream* p_ser_dev
                        )
   : frt_task (a_name, a_priority, a_stack_size, p_ser_dev) {
   brake_pin = brake_mask;
   driver = p_driver;
   brake = p_brake;
   power = p_power;
   pot = p_pot;
   adc_select = adc_mask;
}


//-------------------------------------------------------------------------------------
/** This method is called once by the RTOS scheduler. Each time around the for (;;)
 *  loop, it reads the A/D converter and change the selected motors speed. Each loop
 *  checks if it is being controled by a pot or by a given value and reacts accordingly
 */

void task_motor::run (void) {
   uint16_t a2d_reading;

   adc *p_my_adc = new adc(p_serial);
   PORTC |= (1 << 3) | (1 << 4);
   
   // This is the task loop for the motor control task. This loop runs until the
   // power is turned off or something equally dramatic occurs.
   for (;;) {
      if (PINC & (1 << brake_pin) || brake->get()) {
         driver->brake();
      } else {
         if (pot->get()) {
            a2d_reading = p_my_adc->read_once(adc_select);
            correctPos->put(a2d_reading * 2);
            if(abs(count->get() - correctPos->get()) > 40)
              isCorrectPos->put(false);
            //driver->set_power((a2d_reading / 2) - 255);
         } else {
           isCorrectPos->put(false);
         }
      }
      runs++;
      delay (100);
   }
}

/**
 * Self explanitory. This function takes in a serial port in the form of an
 * address and passes it straight to the parent constructor so it can print
 * out the status. Also prints out the number of runs
 */

void task_motor::print_status (emstream& ser_thing) {
   // Call the parent task's printing function first
   frt_task::print_status (ser_thing);

   // Now add the additional data
   ser_thing << "\t " << runs << PMS (" runs");
}

