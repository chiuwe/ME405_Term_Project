/*
  Stepper.h - - Stepper library for Wiring/Arduino - Version 0.4
  
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

// ensure this library description is only included once
#ifndef Stepper_h
#define Stepper_h

#include <util/delay.h>

#include "emstream.h"                       // Header for serial ports and devices
#include "FreeRTOS.h"                       // Header for the FreeRTOS RTOS
#include "task.h"                           // Header for FreeRTOS task functions
#include "queue.h"                          // Header for FreeRTOS queues
#include "semphr.h"                         // Header for FreeRTOS semaphores

// library interface description
class Stepper {
  public:
    // constructors:
    Stepper(emstream *p_serial_port, 
            uint16_t numberOfSteps,
            uint8_t motorPin1,
            uint8_t motorPin2,
            uint8_t powerPin,
            volatile uint8_t *p_ddr,
            volatile uint8_t * pPort);

    // speed setter method:
    void setSpeed(uint64_t whatSpeed);

    // mover method:
    void step(int16_t numberOfSteps);

    void printStatus(void);
  protected:
    void stepMotor(uint8_t thisStep);
    void myDelayMS(uint64_t waitTime);
    
    uint8_t direction;        // Direction of rotation
    uint64_t step_delay;    // delay between steps, in ms, based on speed
    uint16_t number_of_steps;      // total number of steps this motor can take
    uint32_t step_number;        // which step the motor is on
    
    /// The motor driver class uses this pointer print to the serial port.
    emstream* ptr_to_serial;

    // motor pin numbers:
    uint8_t motor_pin_1;
    uint8_t motor_pin_2;
    uint8_t power_pin;
    volatile uint8_t * p_port;
    
};

#endif

