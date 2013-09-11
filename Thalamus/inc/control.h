#ifndef __CONTROL_H__
#define __CONTROL_H__

// Control/PID
extern unsigned char got_setpoint;

#define THROTTLEOFFSET	950				/*!< Corresponds to less than zero output PWM. Nominally 1060=1.06ms, but lower just in case */
#define IDLETHROTTLE		125			/*!< Minimum PWM output to ESC when in-flight to avoid motors turning off. THROTTLEOFFSET+IDLETHROTTLE > 1060 */
#define MAXTHROTTLE	 	1000			/*!< Maximum PWM output to ESC (THROTTLEOFFSET + MAXTHROTTLE) ~= 1860 */
#define MAXTHROTTLEPERCENT	0.9	 		/*!< Maximum percentage throttle should be (reserves some extra output for stabilisation at high throttle). */
#define MIDDLE				1500	 		/*!< Maximum percentage throttle should be (reserves some extra output for stabilisation at high throttle). */

extern unsigned char thal_throt_cont;
extern unsigned char thal_motor_off;

extern float GPS_KerrI;
extern float ULT_KerrI;
extern float targetZ_ult;
extern float alt_tkoff;

extern float ROLL_SPL_set;
extern float PITCH_SPL_set;
extern float YAW_SPL_set;



void control_throttle(void);
void control_motors(void);

#endif