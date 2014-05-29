/*
  Stepper.cpp - - Stepper library for Wiring/Arduino - Version 0.4
  
  Original library     (0.1) by Tom Igoe.
  Two-wire modifications   (0.2) by Sebastian Gassner
  Combination version   (0.3) by Tom Igoe and David Mellis
  Bug fix for four-wire   (0.4) by Tom Igoe, bug fix from Noah Shibley  

  Drives a unipolar or bipolar stepper motor using  2 wires or 4 wires

  When wiring multiple stepper motors to a microcontroller,
  you quickly run out of output pins, with each motor requiring 4 connections. 

  By making use of the fact that at any time two of the four motor
  coils are the inverse  of the other two, the number of
  control connections can be reduced from 4 to 2. 

  A slightly modified circuit around a Darlington transistor array or an L293 H-bridge
  connects to only 2 microcontroler pins, inverts the signals received,
  and delivers the 4 (2 plus 2 inverted ones) output signals required
  for driving a stepper motor.

  The sequence of control signals for 4 control wires is as follows:

  Step C0 C1 C2 C3
     1  1  0  1  0
     2  0  1  1  0
     3  0  1  0  1
     4  1  0  0  1

  The sequence of controls signals for 2 control wires is as follows
  (columns C1 and C2 from above):

  Step C0 C1
     1  0  1
     2  1  1
     3  1  0
     4  0  0

  The circuits can be found at 
 
http://www.arduino.cc/en/Tutorial/Stepper
 
 
 */
#include <stdlib.h>                         // Include standard library header files
#include <avr/io.h>

#include "rs232int.h"                       // Include header for serial port class
#include "Stepper.h"

/*
 * two-wire constructor.
 * Sets which wires should control the motor.
 */
Stepper::Stepper( emstream *p_serial_port, 
                  uint16_t numberOfSteps,
                  uint8_t motorPin1,
                  uint8_t motorPin2,
                  volatile uint8_t *p_ddr,
                  volatile uint8_t * pPort) {
  ptr_to_serial = p_serial_port;
  step_number = 0;      // which step the motor is on
  direction = 0;      // motor direction
  number_of_steps = numberOfSteps;    // total number of steps for this motor
  
  // Arduino pins for the motor control connection:
  motor_pin_1 = motorPin1;
  motor_pin_2 = motorPin2;
  motor_pin_3 = 0;
  motor_pin_4 = 0;
  pin_count = 2;

  p_port = pPort;
  // setup the pins on the microcontroller:
  *p_ddr |= (1 << motor_pin_1) | (1 << motor_pin_2);


  DBG(ptr_to_serial, "Motor driver 2 pins constructor OK" << endl);
}

/*
 * 4 wire constructor. 
 * sets which wire should control the motor
 */
Stepper::Stepper( emstream *p_serial_port, 
                  uint16_t numberOfSteps,
                  uint8_t motorPin1,
                  uint8_t motorPin2,
                  uint8_t motorPin3,
                  uint8_t motorPin4,
                  volatile uint8_t *p_ddr,
                  volatile uint8_t * pPort) {
  ptr_to_serial = p_serial_port;
  step_number = 0;      // which step the motor is on
  direction = 0;      // motor direction
  number_of_steps = numberOfSteps;    // total number of steps for this motor
  
  // Arduino pins for the motor control connection:
  motor_pin_1 = motorPin1;
  motor_pin_2 = motorPin2;
  motor_pin_3 = motorPin3;
  motor_pin_4 = motorPin4;
  pin_count = 4;

  p_port = pPort;
  // setup the pins on the microcontroller:
  *p_ddr |= (1 << motor_pin_1) | (1 << motor_pin_2) | (1 << motor_pin_3) | (1 << motor_pin_4);
  DBG(ptr_to_serial, "Motor driver 4 pins constructor OK" << endl);
}

/*
  Sets the speed in revs per minute

*/
void Stepper::setSpeed(uint64_t whatSpeed)
{
  step_delay = 60L * 1000L / number_of_steps / whatSpeed;

}

/*
  Moves the motor steps_to_move steps.  If the number is negative, 
   the motor moves in the reverse direction.
 */
void Stepper::step(int16_t steps_to_move)
{  
  int steps_left = abs(steps_to_move);  // how many steps to take
  
  // determine direction based on whether steps_to_mode is + or -:
  if (steps_to_move > 0) {direction = 1;}
  if (steps_to_move < 0) {direction = 0;}
  
  // decrement the number of steps, moving one step each time:
  while(steps_left > 0) {
    // move only if the appropriate delay has passed:
    myDelayMS(step_delay);
    // get the timeStamp of when you stepped:
    // increment or decrement the step number,
    // depending on direction:
    if (direction == 1) {
      step_number++;
      if (step_number == number_of_steps) {
        step_number = 0;
      }
    } 
    else { 
      if (step_number == 0) {
        step_number = number_of_steps;
      }
      step_number--;
    }
    // decrement the steps left:
    steps_left--;
    // step the motor to step number 0, 1, 2, or 3:
    stepMotor(step_number % 4);
  }
}

/*
 * Moves the motor forward or backwards.
 */
void Stepper::stepMotor(uint8_t thisStep)
{
  if(pin_count == 2){
    switch (thisStep) {
      case 0: /* 01 */
        *p_port &= ~(1 << motor_pin_1);
        *p_port |= (1 << motor_pin_2);
        break;
      case 1: /* 11 */
        *p_port |= (1 << motor_pin_1);
        *p_port |= (1 << motor_pin_2);
        break;
      case 2: /* 10 */
        *p_port |= (1 << motor_pin_1);
        *p_port &= ~(1 << motor_pin_2);
        break;
      case 3: /* 00 */
        *p_port &= ~(1 << motor_pin_1);
        *p_port &= ~(1 << motor_pin_2);
        break;
    } 
  } else if( pin_count == 4){
    switch (thisStep) {
      case 0: // 1010
        *p_port |= (1 << motor_pin_1);
        *p_port &= ~(1 << motor_pin_2);
        *p_port |= (1 << motor_pin_3);
        *p_port &= ~(1 << motor_pin_4);
        break;
      case 1: // 0110
        *p_port &= ~(1 << motor_pin_1);
        *p_port |= (1 << motor_pin_2);
        *p_port |= (1 << motor_pin_3);
        *p_port &= ~(1 << motor_pin_4);
        break;
      case 2: //0101
        *p_port &= ~(1 << motor_pin_1);
        *p_port |= (1 << motor_pin_2);
        *p_port &= ~(1 << motor_pin_3);
        *p_port |= (1 << motor_pin_4);
        break;
      case 3: //1001
        *p_port |= (1 << motor_pin_1);
        *p_port &= ~(1 << motor_pin_2);
        *p_port &= ~(1 << motor_pin_3);
        *p_port |= (1 << motor_pin_4);
        break;
    } 
  }
  
}

void Stepper::printStatus(void){
  DBG(ptr_to_serial, "Step number: " << step_number << endl);
  DBG(ptr_to_serial, "Total number of steps: " << number_of_steps << endl);
}

void Stepper::myDelayMS(uint64_t waitTime) {
  while(waitTime--) {
    _delay_ms(1);
  }
} 