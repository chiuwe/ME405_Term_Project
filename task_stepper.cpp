//**************************************************************************************
/** \file task_stepper.cpp
 *    This file contains the code for a motor controller class which controls speed and
 *    direction of a motor using a voltage measured from the A/D as input. One button
 *    will trigger stop and go. A second button will determine which motor is being
 *    controlled. */
//**************************************************************************************

#include "frt_text_queue.h"                 // Header for text queue class
#include "task_stepper.h"                     // Header for this motor controller
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

task_stepper::task_stepper (const char* a_name, 
                         unsigned portBASE_TYPE a_priority, 
                         size_t a_stack_size,
                         emstream* p_ser_dev,
                         Stepper* p_driver,
                         shared_data<int64_t>* p_speed,
                         shared_data<int16_t>* p_numSteps
                        )
   : frt_task (a_name, a_priority, a_stack_size, p_ser_dev) {
   driver = p_driver;
   speed = p_speed;
   numSteps = p_numSteps;
}


//-------------------------------------------------------------------------------------
/** This method is called once by the RTOS scheduler. Each time around the for (;;)
 *  loop, it reads the A/D converter and change the selected motors speed. Each loop
 *  also check the two additional buttons, which control the brakes of the individual
 *  motors.
 */

void task_stepper::run (void) {   
   // This is the task loop for the motor control task. This loop runs until the
   // power is turned off or something equally dramatic occurs.
   for (;;) {
     if (speed->get()) {
        driver->setSpeed(speed->get());
        speed->put(0);
     }
     if(numSteps->get()){
        driver->step(numSteps->get());
        numSteps->put(0);
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

void task_stepper::print_status (emstream& ser_thing) {
   // Call the parent task's printing function first
   frt_task::print_status (ser_thing);

   // Now add the additional data
   ser_thing << "\t " << runs << PMS (" runs");
}

