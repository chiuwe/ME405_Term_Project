/** \file encoder_driver.cpp
 * Class for creating a way to check on a moter encoder. Initializes the pin to use
 * for interupts in the constructer, and creates an ISR to monitor the motor functionality.
 * Comes with a couple of getter and setter functions
 */

#include <stdlib.h>                         // Include standard library header files
#include <avr/io.h>

#include "rs232int.h"                       // Include header for serial port class
#include "encoder_driver.h"                 // Include header for the encoder class

//-------------------------------------------------------------------------------------
/**\brief This constructor sets up a encoder driver. 
 * \details \b Details: Sets up the interupt pin, initializes the serial port and count
 * and error. 
 * @param p_serial_port pointer to the serial port
 * @param bit which pin on PORTE to use as an external interupt
 * @param trigger a mask to put on the external interupt control register to make sure
 *  that the ISR is called both on the rising and falling edge.
 */

encoder_driver::encoder_driver (emstream *p_serial_port, uint8_t bit, uint8_t trigger) {  

   ptr_to_serial = p_serial_port;
   count->put(0);
   error->put(0);
   
   sei ();
   PORTE |= 1 << bit;
   EICRB = trigger;
   EIMSK |= 1 << bit;

   DBG(ptr_to_serial, "Encoder driver constructor OK" << endl);
}

/**
 * Returns the number of ISR calls there have been
 */
int32_t encoder_driver::get_count (void) {
   return count->get();
}

/**
 * Sets the current count to 0
 */

void encoder_driver::zero (void) {
   count->put(0);
}

/**
 * Setter function for count.
 * @param position Sets count to this value.
 */
void encoder_driver::set_position (int32_t position) {
   count->put(position);
}

/**
 * Interupt service rutine that gets called whenever interupt pin 4 or 5 go high or low.
 * Takes the current values of the two pins, and compairs them to what they were last time.
 * If it's valid, increment/decrement count based on direction. If not, increment error. 
 */
ISR (INT4_vect) {
   static uint8_t lastA = 0, lastB = 0;
   uint8_t currentA, currentB;

   currentA = PINE & (1 << PE4);
   currentB = PINE & (1 << PE5);

   switch(currentB | currentA){
      case 0b000000:
         switch(lastB | lastA){
            case 0b010000:
               count->put(count->get() - 1); //reverse
               break;
            case 0b100000:
               count->put(count->get() + 1); //forward
               break;
            default:
               error->put(error->get() + 1);
               break;
         }
         break;
      case 0b010000: 
         switch(lastB | lastA){
            case 0b000000:
               count->put(count->get() + 1); //forward
               break;
            case 0b110000:
               count->put(count->get() - 1); //reverse
               break;
            default:
               error->put(error->get() + 1);
               break;
         }
         break;
      case 0b100000: 
         switch(lastB | lastA){
            case 0b000000:
               count->put(count->get() - 1); //reverse
               break;
            case 0b110000:
               count->put(count->get() + 1); //forward
               break;
            default:
               error->put(error->get() + 1);
               break;
         }
         break;
      case 0b110000: 
         switch(lastB | lastA){
            case 0b010000:
               count->put(count->get() + 1); //forward
               break;
            case 0b100000:
               count->put(count->get() - 1); //reverse
               break;
            default:
               error->put(error->get() + 1);
               break;
         }
         break;
   }
   lastA = currentA;
   lastB = currentB;
}

/**
 * An alias ISR for pin 5.
 */
ISR (INT5_vect, ISR_ALIASOF(INT4_vect));


