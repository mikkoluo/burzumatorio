/*
 * mpu_bmp_example.c
 *
 *  Created on: 29.10.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 */
#include <stdio.h>
#include <string.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

/* Board Header files */
#include "Board.h"
#include "bmp280.h"
#include "mpu9250.h"

#define STACKSIZE 4096
Char taskStack[STACKSIZE];

// *******************************
//
// MPU GLOBAL VARIABLES
//
// *******************************
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

// MPU9250 uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

// SENSOR TASK
Void sensorFxn(UArg arg0, UArg arg1) {

    // *******************************
    //
    // USE TWO DIFFERENT I2C INTERFACES
    //
    // *******************************
	I2C_Handle i2c; // INTERFACE FOR OTHER SENSORS
	I2C_Params i2cParams;
	I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
	I2C_Params i2cMPUParams;

	float ax, ay, az, gx, gy, gz;
	double pres,temp;
	char str[80];

    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    // *******************************
    //
    // MPU OPEN I2C
    //
    // *******************************
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }

    // *******************************
    //
    // MPU POWER ON
    //
    // *******************************
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

    // WAIT 100MS FOR THE SENSOR TO POWER UP
	Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();

    // *******************************
    //
    // MPU9250 SETUP AND CALIBRATION
    //
    // *******************************
	System_printf("MPU9250: Setup and calibration...\n");
	System_flush();

	mpu9250_setup(&i2cMPU);

	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();

    // *******************************
    //
    // MPU CLOSE I2C
    //
    // *******************************
    I2C_close(i2cMPU);

    // *******************************
    //
    // OTHER SENSOR OPEN I2C
    //
    // *******************************
    i2c = I2C_open(Board_I2C, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }

    // BMP280 SETUP
    bmp280_setup(&i2c);

    // *******************************
    //
    // OTHER SENSOR CLOSE I2C
    //
    // *******************************
    I2C_close(i2c);

    // LOOP FOREVER
	while (1) {

	    // *******************************
	    //
	    // OTHER SENSORS OPEN I2C
	    //
	    // *******************************
	    i2c = I2C_open(Board_I2C, &i2cParams);
	    if (i2c == NULL) {
	        System_abort("Error Initializing I2C\n");
	    }

	    // *******************************
	    //
	    // BMP280 ASK DATA
	    //
	    // *******************************
	    bmp280_get_data(&i2c, &pres, &temp);

    	sprintf(str,"%f %f\n",pres,temp);
    	System_printf(str);
    	System_flush();

	    // *******************************
	    //
	    // OTHER SENSORS CLOSE I2C
	    //
	    // *******************************
	    I2C_close(i2c);

	    // *******************************
	    //
	    // MPU OPEN I2C
	    //
	    // *******************************
	    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	    if (i2cMPU == NULL) {
	        System_abort("Error Initializing I2CMPU\n");
	    }

	    // *******************************
	    //
	    // MPU ASK DATA
		//
        //    Accelerometer values: ax,ay,az
	 	//    Gyroscope values: gx,gy,gz
		//
	    // *******************************
		mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);

    	sprintf(str,"%f %f %f %f %f %f\n", ax,ay,az,gx,gy,gz);
    	System_printf(str);
    	System_flush();

	    // *******************************
	    //
	    // MPU CLOSE I2C
	    //
	    // *******************************
	    I2C_close(i2cMPU);

	    // WAIT 100MS
    	Task_sleep(100000 / Clock_tickPeriod);
	}

	// MPU9250 POWER OFF 
	//     Because of loop forever, code never goes here
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
}

int main(void) {

	Task_Handle task;
	Task_Params taskParams;

    Board_initGeneral();
    Board_initI2C();

    // *******************************
    //
    // OPEN MPU POWER PIN
    //
    // *******************************
    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
    	System_abort("Pin open failed!");
    }

    Task_Params_init(&taskParams);
    taskParams.stackSize = STACKSIZE;
    taskParams.stack = &taskStack;
    task = Task_create((Task_FuncPtr)sensorFxn, &taskParams, NULL);
    if (task == NULL) {
    	System_abort("Task create failed!");
    }

    /* Start BIOS */
    BIOS_start();

    return (0);
}
