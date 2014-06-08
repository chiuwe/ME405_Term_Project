/** \file task_encoder.cpp
 * This class is basically a wrapper for the encoder_driver. It creates a new
 * encoder driver, and loops forever waiting for the encoder_driver ISR to do
 * something. Outputs to the serial queue to show how the encoder_driver is doing.
 */


#include "task_encoder.h"                   // Header for this encoder


//-------------------------------------------------------------------------------------

/**
 * Basic constructor. Assigns all the appropriate paramiters to their respective 
 * instance variables, creates a new encoder to start running, and initializes
 * runs to 0
 * @param a_name A character string which will be the name of this task
 * @param a_priority The priority at which this task will initially run (default: 0)
 * @param a_stack_size The size of this task's stack in bytes
 *                   (default: configMINIMAL_STACK_SIZE)
 * @param p_ser_dev Pointer to a serial device which can be used by this task to communicate
 * @param bit which pin on PORTE to use as an external interupt
 * @param trigger a mask to put on the external interupt control register to make sure
 *  that the ISR is called both on the rising and falling edge.
 */
task_encoder::task_encoder (const char* a_name,
                            unsigned portBASE_TYPE a_priority,
                            size_t a_stack_size,
                            emstream* p_ser_dev,
                            uint8_t bit,
                            uint8_t trigger)
   : frt_task (a_name, a_priority, a_stack_size, p_ser_dev) {

    encoder = new encoder_driver(p_ser_dev, bit, trigger);
    runs = 0;
}


//-------------------------------------------------------------------------------------
/** This method is called once by the RTOS scheduler. Each time around the for (;;)
 *  loop, it sends the current number of errors and the current count to the screen.
 *  The main point is to wait for interputs to be called in encoder_driver, which will
 *  change |error| and |count| based on different functionally.
 */

void task_encoder::run (void) {

   for (;;) {
     //*p_serial << PMS ("Error: ") << error->get() << " " << PMS ("Count: ") << encoder->get_count() << endl;
     delay (100);
     runs++;
   }
}

/**
 * Self explanitory. This function takes in a serial port in the form of an
 * address and passes it straight to the parent constructor so it can print
 * out the status. Also prints out the number of runs
 */

void task_encoder::print_status (emstream& ser_thing) {
   // Call the parent task's printing function first
   frt_task::print_status (ser_thing);

   // Now add the additional data
   ser_thing << "\t " << runs << PMS (" runs");
}

