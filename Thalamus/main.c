// GENERAL TODO list in order of priority
// Specific TODO's are listed next to the relevant bit of code.

//TODO: (HENRY DOES THIS, NEEDS LOTS OF SPACE AND SPARE PARTS!)
//Diagnose loss of control experienced at full throttle
//I ascended at full throttle while probably applying some pitch and roll demands and suddenly the craft just seemed to loose stability.
//It rolled/ pitched upside down a few times, luckily it recovered and I was able to bring it  back safely. 
//However, have heard that a backer had a similar problem. 
//Possible causes are Yuan's Max throttle stuff, my roll angle prioritisation (less likely as wasn't in the backers code), ROLL/ PITCH SPL (spin limit). Or other!
// Have done some test flights and witnessed the loss of control at high throttles, it only does it when applying a high throttle and a large attitude demand, it inverts then enters a continual spin at high throttle
// Have added code on line 51 of control.h to try and solve this problem
//Rescues craft if error gets too large at high throttles by detecting roll and pitch error getting too large and reducing the throttle.
	// TODO: Test to see if this code solves the problem
	// Tried this: if (((pitcherror > 0.08) || (pitcherror < -0.08) || (rollerror > 0.08) || (rollerror < -0.08)) && (throttle > 600)) throttle -= 200;
// Should really be able to solve with better PID tuning.

//TODO: check all integrators for decoupling, and make sure they are locked in this case

// TODO: Assess and Improve leveling on take off and leveling in flight
// Key areas to investigate are integral gain on takeoff
// Accelerometer feedback gain
// Accelerometer feedback method - additional filtering needed?
// Landing Gear separation seems to play a large part, it causes undesired integral wind-up before the craft is in the air
// Landing gear would need switches or be sprung and damped with longish travel

// TODO: Measure loop rates instead of just assuming it
// The control needs to know how fast they're going, right now we assume the loops are going at their specified rate
// however, it would be better to just time instead.  Use one of the hardware timers to get sub-ms resolution.

// TODO: Check for some discontinuities in flight
// Throttle and attitude blips when flying under GPS
// Throttle blips when flying normally

/////////////////////// Libraries to include /////////////
#include "all.h"


// Initialiser function: sets everything up.
void setup() {	
	// *** Startup PWM
		// the ESC will beep when it doesn not receive a PWM signal, we use this to indicate that the craft is powered but disarmed
		// however, the ESC will not beep the first time it is powered, so we start up the PWM here and turn it off quickly so that
		// under ALL circumstances the ESC will beep when the craft is disarmed.
		PWMInit(PWM_ALL);
		PWMSetNESW(THROTTLEOFFSET, THROTTLEOFFSET, THROTTLEOFFSET, THROTTLEOFFSET);
	
	// *** LED setup
		LEDInit(PLED | VLED);
		LEDOff(PLED | VLED);
		
	// *** Timers and couters
		SysTickInit();  // SysTick enable (default 1ms)
		
	// *** Parameters
		eeprom_load_all();
	
	// *** Establish ILink
		ilink_thalstat.sensorStatus = 1; // set ilink status to boot
		ilink_thalstat.flightMode = (0x1 << 0); // attitude control TODO: sort out these modes
		ilink_identify.deviceID = WHO_AM_I;
		ilink_identify.firmVersion = FIRMWARE_VERSION;
		ILinkInit(SLAVE);
	
	// *** Initialise input
		RXInit();
				
	// *** Initialise Ultrasound
		UltraInit();

	// *** Battery sensor
		ADCInit(CHN7);
		ADCTrigger(CHN7);
		Delay(1);
		ilink_thalstat.battVoltage = (ADCGet() * 6325) >> 10; // Because the factor is 6325/1024, we can do this in integer maths by right-shifting 10 bits instead of dividing by 1024.
		
	// *** Calibrate Sensors
		SensorInit();
		sensor_zero();
		
		TrigBaroTemp(); // get parometer temperature for temperature compensation.
		Delay(15);
		GetBaroTemp();

		// Temporarily store drift correction multipliers
		float tempAccelKp = DRIFT_AccelKp;
		float tempMagKp = DRIFT_MagKp;
		
		// Set high confidence in accelerometer/magneto to rotate AHRS to initial heading
		DRIFT_MagKp = 20;
		DRIFT_AccelKp = 20;
		
		//Timer for AHRS
		Timer0Init(59);
		Timer0Match0(1200000/FAST_RATE, INTERRUPT | RESET);
		Delay(1000);
	   
		// Now Set them back to their original values
		DRIFT_MagKp = tempMagKp;
		DRIFT_AccelKp = tempAccelKp;
		
	// *** Initialise timers and loops
		PWMSetNESW(0, 0, 0, 0);
		RITInitms(1000/MESSAGE_LOOP_HZ);
		flashPLED = 0;
		LEDOff(PLED);
}

//Main loop, nothing much happens in here.
void loop() {
	//if(idleCount < IDLE_MAX) idleCount++; // this is the counter for CPU idle time
	//Deal with button push for entering bind mode for RX
		if(PRGBlankTimer == 0) {
			if(PRGTimer > 3000) {
				RXBind();
				PRGPushTime = 0;
				PRGTimer = 0;
				PRGBlankTimer = 200;
			}
		}
	
	__WFI();
}

// SysTick timer: deals with general timing
void SysTickInterrupt(void) {
	sysMS += 1;
	sysUS += 1000;
	
	//Deal with flashing LEDs
		if(sysMS % 25 == 0) {
			if(sysMS % 100 == 0) {
				if(flashPLED) LEDToggle(PLED);
				if(flashVLED) LEDToggle(VLED);
				if(flashRLED) LEDToggle(RLED);
			}
			else {
				if(flashPLED == 2) LEDToggle(PLED);
				if(flashVLED == 2) LEDToggle(VLED);
				if(flashRLED == 2) LEDToggle(RLED);
			}
		}
	
	// Time the button pushes
		if(PRGPoll() == 0) PRGTimer++;
		else {
			PRGPushTime = PRGTimer;
			PRGTimer = 0;
			if(PRGBlankTimer) PRGBlankTimer--;
		}
}

// RIT interrupt, deal with timed iLink messages.
void RITInterrupt(void) {
	
	// Deal with iLink parameter transmission
		unsigned int i;
		if(paramSendCount < paramCount) {
			unsigned short thisParam = paramSendCount; // store this to avoid race hazard since paramSendCount can change outside this interrupt
			ilink_thalparam_tx.paramID = thisParam;
			ilink_thalparam_tx.paramValue = paramStorage[thisParam].value;
			ilink_thalparam_tx.paramCount = paramCount;
			for(i=0; i<16; i++) {
				ilink_thalparam_tx.paramName[i] = paramStorage[thisParam].name[i];
				if(paramStorage[thisParam].name[i] == '\0') break;
			}
			if(ILinkSendMessage(ID_ILINK_THALPARAM, (unsigned short *) & ilink_thalparam_tx, sizeof(ilink_thalparam_tx)/2 -1)) {
				if(paramSendSingle) {
					paramSendSingle = 0;
					paramSendCount = paramCount;
				}
				else {
					paramSendCount = thisParam+1;
				}
			}
		}
}



//Main functional periodic loop
void Timer0Interrupt0() {

	
	if(++slowSoftscale >= SLOW_DIVIDER) {
		slowSoftscale = 0;

		// These run at SLOW_RATE - The order they run in matters
        trig_batt_voltage();
		read_mag_sensors();
		read_rx_input();		
		read_ultrasound();
		read_barometer();
		read_batt_voltage();
		filter_gps_baro();
		gps_status();
		state_machine();
		// We have to reread everytime transmitted data is used
		read_rx_input();
			
	}

	// These run at FAST_RATE  - The order they run in matters
	read_acc_sensors();
	read_gyr_sensors();
	
	a_h_r_s();
	
	control_throttle();
	control_motors();


}
