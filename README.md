# Semi-automated-blood-pressure-and-Heart-rate-measuring-device

**Objective:**
	To build a semi-automated blood pressure/ Heart rate measuring device

**Required Components/Software:**
	1. Honeywell MPRLS0300YG00001BB
	2. Sphygmomanometer cuff and tube
	3. STM32F429 Discovery Board
	4. F-F wires -4
	5. Tubing and sealant - to avoid leaks
	6. C++ editor -Visual Studio Code, Platform IO , HAL -mBed

**Setup:**
	The outlet of the sphygmomanometer tube is connected to the pressure sensor via a tubing and sealed properly using a sealant. The sensor is connected to the microcontroller for I2C communication in the following way.

**Connection:**
	Sensor
	SDA 	-	PortC pin9
	SCL 	-	PortA pin8
	VCC 	-	3V
	GND 	-	GND
	The microcontroller is connected to a i7 Windows machine with 16GB RAM.

**Description:**
	The arm cuff is worn and inflated until the pressure value of 150mmHg is reached.
	The cuff pressure is slowly released at the rate of 4 mmHg until the pressure drops to 30mmHg.
	The measured pressure values with its time are stored. The slope at every measurement is also calculated.
	The slopes are used to calculate systole and dialstole values. The time values are required to calculate the heartbeat.
	
**Calculation:**
	Pressure = [(Output - Outputmin) * (Pressuremax- Pressuremin)/(Outputmax - Outputmin)]+Pmin
	Systole  = The value of pressure whose slope is 50% of the maximum threshold value
	Diastole = The value of pressure whose slope is 80% of the threshold value
	Heartbeat = Number of samples/second *60

**Output:**
	Blood pressure levels are printed as following,
					120/80  	  - normal
					121-129/80    - elevated
					130-140/81-89 - Hypertension Stage 1
					140+/90+      - Hypertension Stage 2

**Future Scope:**
	The system could be fully autonomous and avoid manual intervention. Also, accessibility features could be added especially for visually impaired individuals.
