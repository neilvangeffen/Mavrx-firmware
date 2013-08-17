
void control_throttle(){


}


// ****************************************************************************
// *** Attitude PID Control
// ****************************************************************************	

void control_attitude(){

	float pitcherror, rollerror, yawerror;
	float pitchcorrection, rollcorrection, yawcorrection;

	// This section of code applies some throttle increase with high tilt angles
	//It doesn't seem hugely effective and maybe completely redundant when Barometer control is implemented
	// TODO: Reassess whether it is useful or not
	float M9temp;
	if (M9 > 0) M9temp = M9;
	else M9temp = -M9;
	throttle_angle = ((throttle / M9temp) - throttle); 
	if (throttle_angle < 0) throttle_angle = 0;
	
	// This section of code limits the rate at which the craft is allowed to track angle demand changes
	if ((pitchDemandSpin - pitchDemandSpinold) > PITCH_SPL) pitchDemandSpin = pitchDemandSpinold + PITCH_SPL;
	if ((pitchDemandSpin - pitchDemandSpinold) < -PITCH_SPL) pitchDemandSpin = pitchDemandSpinold - PITCH_SPL;
	pitchDemandSpinold = pitchDemandSpin;		
	if ((rollDemandSpin - rollDemandSpinold) > ROLL_SPL) rollDemandSpin = rollDemandSpinold + ROLL_SPL;
	if ((rollDemandSpin - rollDemandSpinold) < -ROLL_SPL) rollDemandSpin = rollDemandSpinold - ROLL_SPL;
	rollDemandSpinold = rollDemandSpin;
			
			
	// This part of the code sets the maximum angle the quadrotor can go to in the pitch and roll axes
	if(pitchDemandSpin > LIM_ANGLE) pitchDemandSpin = LIM_ANGLE;	
	if(pitchDemandSpin < -LIM_ANGLE) pitchDemandSpin = -LIM_ANGLE;
	if(rollDemandSpin > LIM_ANGLE) rollDemandSpin = LIM_ANGLE;
	if(rollDemandSpin < -LIM_ANGLE) rollDemandSpin = -LIM_ANGLE;

	// Create the demand derivative (demand and external rotations are split) term for the attitude motor control
	pitch.derivative = (pitchDemandSpin - pitch.demandOld);	
	roll.derivative = (rollDemandSpin - roll.demandOld);
	yaw.derivative = (yaw.demand - yaw.demandOld);	   
	pitch.demandOld = pitchDemandSpin;
	roll.demandOld = rollDemandSpin;
	yaw.demandOld = yaw.demand;
	
	// Use the Current and demanded angles to create the error for the proportional part of the PID loop
	// TODO: it might be more mathematically elegant (but harder to understand) 
	//to express the demand as a quaternion, but this is not a high priority
	pitcherror = pitchDemandSpin + thetaAngle;
	rollerror = rollDemandSpin - phiAngle;
	yawerror = yaw.demand + psiAngle; 
	
	//Rescues craft if error gets too large at high throttles
	// TODO: Test to see if this code solves the problem
	if (((pitcherror > 0.08) || (pitcherror < -0.08) || (rollerror > 0.08) || (rollerror < -0.08)) && (throttle > 600)) throttle -= 200;
	
	// This section ensures that on swapping between -PI and PI, 
	// the craft always takes the shortest route to the desired angle
	if(pitcherror > M_PI) pitcherror -= M_TWOPI;
	else if(pitcherror < -M_PI) pitcherror += M_TWOPI;
	if(rollerror > M_PI) rollerror -= M_TWOPI;
	else if(rollerror < -M_PI) rollerror += M_TWOPI;  
	if(yawerror > M_PI) yawerror -= M_TWOPI;
	else if(yawerror < -M_PI) yawerror += M_TWOPI;
	
	// Creating the integral for the motor PID
	// TODO: Is this the cause of poor leveling on takeoff? (took out wierd throttle dependent rates)
	//TODO: Check to see if added Yaw integral solves yaw offsets in autonomous flight
	pitch.integral += pitcherror;
	roll.integral += rollerror;
	yaw.integral += yawerror;
	pitch.integral *= PITCH_De;
	roll.integral *= ROLL_De;
	yaw.integral *= YAW_De;
	
	// Detune at high throttle - We turn the tunings down at high throttle to prevent oscillations
	// happening on account of the higher energy input to the system
	float throttlefactor = throttle/MAXSTICK;
	if(throttlefactor > 1) throttlefactor = 1;	   
	float detunefactor = 1-(throttlefactor * DETUNE);
	float thisPITCH_Kd = PITCH_Kd;
	float thisPITCH_Kdd = PITCH_Kdd * detunefactor;
	float thisROLL_Kd = ROLL_Kd;
	float thisROLL_Kdd = ROLL_Kdd * detunefactor;
	float thisPITCH_Ki = PITCH_Ki;
	float thisROLL_Ki = ROLL_Ki;
	
			
	// Attitude control PID Assembly - We use a proportional, derivative, and integral on pitch roll and yaw
	// we add a double derivative term for pitch and roll.
	pitchcorrection = -((float)Gyro.Y.value - PITCH_Boost*pitch.derivative) * thisPITCH_Kd;
	pitchcorrection += -thisPITCH_Kdd*((float)Gyro.Y.value - pitch.valueOld);
	pitchcorrection += -PITCH_Kp*pitcherror;
	pitchcorrection += -thisPITCH_Ki*pitch.integral;

	rollcorrection = -((float)Gyro.X.value - ROLL_Boost*roll.derivative) * thisROLL_Kd;
	rollcorrection += -thisROLL_Kdd*((float)Gyro.X.value - roll.valueOld);
	rollcorrection += ROLL_Kp*rollerror;
	rollcorrection += thisROLL_Ki*roll.integral;

	yawcorrection = -((float)Gyro.Z.value + YAW_Boost*yaw.derivative) * YAW_Kd; 
	yawcorrection += -YAW_Kp*yawerror;
	// TODO: Check direction of yaw integral
	yawcorrection += -YAW_Ki*yaw.integral;
	
	// If the craft is upsidedown, turn off pitch and yaw control until roll control brings it back upright.
	// TODO: Test this code
	if ((phiAngle > M_PI_2) || (phiAngle < -M_PI_2)) {
		yawcorrection = 0;
		pitchcorrection = 0;
	}
	
	pitch.valueOld = (float)Gyro.Y.value;
	roll.valueOld = (float)Gyro.X.value;
   
	//Assigning the PID results to the correct motors
	// TODO: add support for multiple orientations here
	motorN = pitchcorrection + rollcorrection;
	motorE = pitchcorrection - rollcorrection;
	motorS = -pitchcorrection - rollcorrection;
	motorW = -pitchcorrection + rollcorrection;
	motorN -= yawcorrection;
	motorE += yawcorrection;
	motorS -= yawcorrection;
	motorW += yawcorrection;
	
	// We run an LPF filter on the outputs to ensure they aren't too noisey and don't demand changes too quickly
	// This seems to reduce power consumption a little and ESC heating a little also
	motorNav *= LPF_OUT;
	motorNav += (1-LPF_OUT) * motorN;		
	motorEav *= LPF_OUT;
	motorEav += (1-LPF_OUT) * motorE;		
	motorSav *= LPF_OUT;
	motorSav += (1-LPF_OUT) * motorS;		
	motorWav *= LPF_OUT;
	motorWav += (1-LPF_OUT) * motorW;
}



// ****************************************************************************
// *** Handle throttle and motor outputs
// ****************************************************************************	

void control_motors(){	

	// Combine attitude stabilisation demands from PID loop with throttle demands
	tempN = (signed short)motorNav + (signed short)throttle + THROTTLEOFFSET + (signed short)throttle_angle;
	tempE = (signed short)motorEav + (signed short)throttle + THROTTLEOFFSET + (signed short)throttle_angle;
	tempS = (signed short)motorSav + (signed short)throttle + THROTTLEOFFSET + (signed short)throttle_angle;
	tempW = (signed short)motorWav + (signed short)throttle + THROTTLEOFFSET + (signed short)throttle_angle;

	// TODO: Add Auto Land on rxLoss!
	if (rcInput[RX_THRO] - throttletrim <  OFFSTICK || throttleHoldOff > 0 || rxLoss > 25) {
		
		// Set throttle off
		throttle = 0;
					
		// Reset Important variables
		pitch.integral=0;
		roll.integral=0;
		motorN = 0;
		motorE = 0;
		motorS = 0;
		motorW = 0;
		motorNav = 0;
		motorEav = 0;
		motorSav = 0;
		motorWav = 0;
		// Reseting the yaw demand to the actual yaw angle continuously helps stop yawing happening on takeoff
		yaw.demand = -psiAngle;
		yaw.demandOld = -psiAngle;


		
		// Reset the throttle hold variable, this prevents reactivation of the throttle until 
		// the input is dropped and re-applied
		if(rcInput[RX_THRO] - throttletrim <  OFFSTICK) throttleHoldOff = 0;
		
		// If the craft is armed, set the PWM channels to the PWM value corresponding to off!
		if(armed) PWMSetNESW(THROTTLEOFFSET, THROTTLEOFFSET, THROTTLEOFFSET, THROTTLEOFFSET);
		
		// Output the motor PWM demand on the telemetry link
		ilink_outputs0.channel[0] = THROTTLEOFFSET;
		ilink_outputs0.channel[1] = THROTTLEOFFSET;
		ilink_outputs0.channel[2] = THROTTLEOFFSET;
		ilink_outputs0.channel[3] = THROTTLEOFFSET;
		

	}
	else if(armed) {
		
		float temp;
		
		// Limit the maximum throttle output to a percentage of the highest throttle available 
		// to allow additional throttle for manouevering
		if(throttle > MAXTHROTTLE*MAXTHROTTLEPERCENT) throttle = MAXTHROTTLE*MAXTHROTTLEPERCENT;
		
		// Set the PWM channels. Maximum of MAXTHROTTLE + THROTTLEOFFSET, MINMUM OF IDLETHROTTLE + THROTTLE OFFSET
		// Throttle offset offsets the throttle readings (which start at 0) to the PWM values (in ms?) which need to start at around 1000
		temp = tempN;
		if(temp > (MAXTHROTTLE + THROTTLEOFFSET)) temp = (MAXTHROTTLE + THROTTLEOFFSET);
		else if(temp < (IDLETHROTTLE + THROTTLEOFFSET)) temp = (IDLETHROTTLE + THROTTLEOFFSET);
		PWMSetN(temp);
		ilink_outputs0.channel[0] = temp;
		
		temp = tempE;
		if(temp > (MAXTHROTTLE + THROTTLEOFFSET)) temp = (MAXTHROTTLE + THROTTLEOFFSET);
		else if(temp < (IDLETHROTTLE + THROTTLEOFFSET)) temp = (IDLETHROTTLE + THROTTLEOFFSET);
		PWMSetE(temp);
		ilink_outputs0.channel[1] = temp;
		
		temp = tempS;
		if(temp > (MAXTHROTTLE + THROTTLEOFFSET)) temp = (MAXTHROTTLE + THROTTLEOFFSET);
		else if(temp < (IDLETHROTTLE + THROTTLEOFFSET)) temp = (IDLETHROTTLE + THROTTLEOFFSET);
		PWMSetS(temp);
		ilink_outputs0.channel[2] = temp;
		
		temp = tempW;
		if(temp > (MAXTHROTTLE + THROTTLEOFFSET)) temp = (MAXTHROTTLE + THROTTLEOFFSET);
		else if(temp < (IDLETHROTTLE + THROTTLEOFFSET)) temp = (IDLETHROTTLE + THROTTLEOFFSET);
		PWMSetW(temp);
		ilink_outputs0.channel[3] = temp;
		ilink_outputs0.isNew = 1;

	}
}