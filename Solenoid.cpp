#include <stdlib.h>                         // Include standard library header files
#include <avr/io.h>

#include "rs232int.h"                       // Include header for serial port class
#include "Solenoid.h"

/*
 * two-wire constructor.
 * Sets which wires should control the motor.
 */
Solenoid::Solenoid( emstream *p_serial_port, 
                  uint8_t activationPin,
                  volatile uint8_t *p_ddr,
                  volatile uint8_t * pPort) {

  activation_Pin = activationPin;
  p_port = pPort;
  // setup the pins on the microcontroller:
  *p_ddr |= (1 << activation_Pin);

  //start it high to close it
  *p_port &= ~(1 << activation_Pin);

  DBG(ptr_to_serial, "Solenoid constructor OK" << endl);
}

void Solenoid::release() {
  *p_port |= (1 << activation_Pin);
   _delay_ms(200);
   *p_port &= ~(1 << activation_Pin);
}

void Solenoid::myDelayMS(uint64_t waitTime) {
  while(waitTime--) {
    _delay_ms(1);
  }
} 