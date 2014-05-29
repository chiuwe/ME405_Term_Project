
#include "frt_text_queue.h"                 // Header for text queue class
#include "task_solenoid.h"                     // Header for this motor controller
#include "shares.h"                         // Shared inter-task communications



task_solenoid::task_solenoid (const char* a_name, 
                         unsigned portBASE_TYPE a_priority, 
                         size_t a_stack_size,
                         emstream* p_ser_dev,
                         Solenoid* p_driver,
                         shared_data<bool>* p_fire
                        )
   : frt_task (a_name, a_priority, a_stack_size, p_ser_dev) {
   driver = p_driver;
   fire = p_fire;
}


//-------------------------------------------------------------------------------------
/** This method is called once by the RTOS scheduler. Each time around the for (;;)
 *  loop, it reads the A/D converter and change the selected motors speed. Each loop
 *  also check the two additional buttons, which control the brakes of the individual
 *  motors.
 */

void task_solenoid::run (void) {   
   // This is the task loop for the motor control task. This loop runs until the
   // power is turned off or something equally dramatic occurs.
   for (;;) {
     if (fire->get()) {
        driver->release();
        fire->put(false);
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

void task_solenoid::print_status (emstream& ser_thing) {
   // Call the parent task's printing function first
   frt_task::print_status (ser_thing);

   // Now add the additional data
   ser_thing << "\t " << runs << PMS (" runs");
}

