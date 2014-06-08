//**************************************************************************************
/** \file task_motor.cpp
 *    This file contains the code for a motor controller class which controls speed and
 *    direction of a motor using a voltage measured from the A/D as input. One button
 *    will trigger stop and go. A second button will determine which motor is being
 *    controlled. */
//**************************************************************************************


#include "task_P.h"                     // Header for this motor controller


//-------------------------------------------------------------------------------------
/** This constructor creates a task which controls the rotation of a motor using
 *  input from an A/D converter run through a potentiometer as well as an input from.
 *  an interface. The main job of this constructor is to call the
 *  constructor of parent class (\c frt_task ); the parent's constructor the work.
 *  @param a_name A character string which will be the name of this task
 *  @param a_priority The priority at which this task will initially run (default: 0)
 *  @param a_stack_size The size of this task's stack in bytes 
 *                      (default: configMINIMAL_STACK_SIZE)
 *  @param p_ser_dev  Pointer to a serial device (port, radio, SD card, etc.) which can
 *                    be used by this task to communicate (default: NULL)
 *	@param mDriver A pointer to the driver being controled.
 *  
 */

task_P::task_P (const char* a_name, 
                         unsigned portBASE_TYPE a_priority, 
                         size_t a_stack_size,
                         emstream* p_ser_dev,
                         motor_driver *mDriver
                        )
   : frt_task (a_name, a_priority, a_stack_size, p_ser_dev) {
	motor = mDriver;
}


//-------------------------------------------------------------------------------------
/**  This is the main function of task_p. Every iteration it checks if the motor is 
 *	where it is saposed to be. If it's not, it tries to get there. The speeds are 
 *	relative to the distance to travel, with a hardcoded lower and upper bound specific
 *	to the PittMan motor. 
 */

void task_P::run (void) {

   uint32_t speed, offset;
   for (;;) {
   		if(!isCorrectPos->get()){
   			while((offset = abs(correctPos->get() - count->get())) > 0){
   				speed = offset / 10;
   				if (speed > 45)
   					speed = 45;
   				if(speed < 30)
   					speed = 30;
   				if(correctPos->get() < count->get()){
   					motor->set_power(-speed);
   				} else {
   					motor->set_power(speed);
   				}

   			}
   			motor->set_power(0);
   			isCorrectPos->put(true);
   		}
   		delay (100);
   }
}
