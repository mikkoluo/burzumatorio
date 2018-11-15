#include <stdio.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>

/* Board Header files */
#include "Board.h"

#include "wireless/comm_lib.h"
#include "sensors/bmp280.h"
#include "sensors/opt3001.h"

/* Task */
#define STACKSIZE 4906
Char taskStack[STACKSIZE];
Char taskCommStack[STACKSIZE];

/* Display */
Display_Handle hDisplay;

/* Pin Button1 configured as power button */
static PIN_Handle hPowerButton;
static PIN_State sPowerButton;
PIN_Config cPowerButton[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};
PIN_Config cPowerWake[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
    PIN_TERMINATE
};

/* Pin Button0 configured */
static PIN_Handle hButton0;
static PIN_State sButton0;
PIN_Config cButton0[] = {
    Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};

/* Leds */
static PIN_Handle hLed;
static PIN_State sLed;
PIN_Config cLed[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

/* Handle power button */
Void powerButtonFxn(PIN_Handle handle, PIN_Id pinId) {

    Display_clear(hDisplay);
    Display_close(hDisplay);
    Task_sleep(100000 / Clock_tickPeriod);

	PIN_close(hPowerButton);

    PINCC26XX_setWakeup(cPowerWake);
	Power_shutdown(NULL,0);
}

// Napinpainalluksen käsittelijäfunktio
void button0Fxn(PIN_Handle handle, PIN_Id pinId) {
	// Vaihdetaan led-pinnin tilaa negaatiolla
	// eli luetaan pinnin tila jasamantien asetetaan sen negaatio
   PIN_setOutputValue( hLed, Board_LED0, !PIN_getOutputValue( Board_LED0 ) );
}
// useammalle pinnille seuraavalla tavalla
/*void buttonFxn(PIN_Handle handle, PIN_Id pinId) {

   if(pinId == Board_BUTTON0) {
      tee_jotain1();
   } else if(pinId == Board_BUTTON1) {
      tee_jotain2();
   }
}*/

/*************************************** Communication Task ********************************************/
Void commFxn(UArg arg0, UArg arg1)
{
	//char payload[80]; // viestipuskuri
	//uint16_t senderAddr;
	//char viestin_tulostus[90];

	// Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if(result != true)
	{
		System_abort("Wireless receive mode failed");
	}
	// YOUR CODE HERE TO SEND EXAMPLE MESSAGE
	// Toiseen kamppeeseen Send-rakenne aikanaan...
	// Joka kerta, ennenkuin lähetät viestiä, testaa että funktio GetTXFlag palauttaa arvon false
	/* Odotamme, että edellinen viesti on lähtenyt
      	if (GetTXFlag() == false) {
         // Lähetä uuden viesti: Terve
         Send6LoWPAN(0xFFFF, "Terve", 5);
         // Jokaisen lähetetyn viestin jälkeen radio pitää asettaa
         // vastaanotto-tilaan funktiokutsulla StartReceive6LoWPAN
         StartReceive6LoWPAN();   */

	while (1)
	{
		if (GetRXFlag() == true)
		{
			// Viesti odottaa, luetaan viesti puskuriin
			/*if (Receive6LoWPAN(&senderAddr, payload, 80) != -1)
			{
			sprintf(viestin_tulostus, "Viesti: %s\n", payload);
			System_printf(viestin_tulostus);
			System_flush();
			}*/
		}
		// THIS WHILE LOOP DOES NOT USE Task_sleep
	}
}

/******************************************************** Light Sensor Taskifunktio *************************************************/
Void light_sensorFxn(UArg arg0, UArg arg1)
{

	uint8_t    i;
	uint16_t 		valoisuus;					// Ei toimi floatilla ??
	char 			txBuffer[4];
	char 			str[20];
	uint16_t  		rxBuffer[1];
	I2C_Handle      i2c;
	I2C_Params      i2cParams;
	I2C_Transaction i2cTransaction;

	I2C_Params_init(&i2cParams);
	i2cParams.bitRate = I2C_400kHz;
	i2c = I2C_open(Board_I2C0, &i2cParams);

	if (i2c == NULL)
	{
		System_abort("Error Initializing I2C\n");
	}
    else
    {
    	opt3001_setup(&i2c); // setup opt-sensor & I2c
    }

	/* Read sensor data */
	i2cTransaction.slaveAddress = Board_OPT3001_ADDR; 		// Laitteen osoite
	txBuffer[0] = OPT3001_DATA_REG; 						// Laiterekisteri
	i2cTransaction.writeBuf = txBuffer; 					// Lähetyspuskuri
	i2cTransaction.writeCount = 1; 							// Datan koko
	i2cTransaction.readBuf = rxBuffer; 						// Vastaanottopuskuri
	i2cTransaction.readCount = 2; 							// Datan koko kaksi tavua

/*float sensorOpt3001Convert(uint16_t rawData)
{
  uint16_t e, m;

  m = rawData & 0x0FFF;
  e = (rawData & 0xF000) >> 12;

  return m * (0.01 * pow(2.0,e));
}*/
	// Kymmenen valoisuusarvoa
	for (i = 0; i < 20; i++)
	{
		if (I2C_transfer(i2c, &i2cTransaction))
		{
		/* Muunna luettu data rxBufferista valoisuusarvoksi datakirjassa kerrotun mukaisesti */
		valoisuus = (rxBuffer[0] & 0x0FFF) * (0.01 * (1 << (rxBuffer[0] >> 12)));

		//System_printf("Valoisuus on %d LUX\n", (1 << (rxBuffer[0] >> 12)));
		//System_printf("Valoisuus on %d LUX\n", (rxBuffer[0] & 0x0FFF));
		//caca = (1 << (rxBuffer[0] >> 12)) * (rxBuffer[0] & 0x0FFF);
		System_printf("Valoisuus on %d LUX\n", valoisuus);

		//sprintf(str,"Lightning: %.2f luxia\n", valoisuus);	// tällä tulostuu desimaalit
		//System_printf(str);

		System_flush();
		}
		else
		{
		System_printf("I2C Bus fault\n");
		}
	/* 2sekunnin viive */
	Task_sleep(2 *1000000 / Clock_tickPeriod);
	}
	/* Sulje i2c-yhteys */
	I2C_close(i2c);
}

Int main(void) {

    // Task variables
	Task_Handle task;
	Task_Params taskParams;
	Task_Handle taskComm;
	Task_Params taskCommParams;

    // Initialize board
    Board_initGeneral();
    Board_initI2C();

/******************************************************* Buttons *************************************************/
    // Ensin annettu power-button 'alanapille' miten tämä ohjelmaan käyttöön??
	hPowerButton = PIN_open(&sPowerButton, cPowerButton);
	if(!hPowerButton) {
		System_abort("Error initializing button shut pins\n");
	}
	if (PIN_registerIntCb(hPowerButton, &powerButtonFxn) != 0) {
		System_abort("Error registering button callback function");
	}
	/* Sitten omaa kamaa: Button0:lle pinni */
	hButton0 = PIN_open(&sButton0, cButton0);
	if(!hButton0) {
		System_abort("Error initializing button shut pins\n");
	}
    /* Leds */
    hLed = PIN_open(&sLed, cLed);
    if(!hLed) {
        System_abort("Error initializing LED pin\n");
    }
	/* Seuraavassa 'ylänappi' käyttöön
	 * pinnin keskeytyksen käsittelijä, parametrina funktio: button0Fxn*/
	if (PIN_registerIntCb(hButton0, &button0Fxn) != 0) {
		System_abort("Error registering button callback function");
	}
/****************************************************************************************************************/
    /* Task */
	// Täällä alustetaan oma task (taskFxn alunperin)
	// Siihen piirtelyt, sensorit ja muut
    Task_Params_init(&taskParams);
    taskParams.stackSize = STACKSIZE;
    taskParams.stack = &taskStack;
    taskParams.priority=2; // Miksi 2, koska tiedonsiirto 1

    // Oma taski taskFxn Ajoon taskFxn tai light_sensorFxn
    task = Task_create(light_sensorFxn, &taskParams, NULL);
    if (task == NULL) {
    	System_abort("Task create failed!");
    }

/************************************************** Communication Task **********************************************/
    Init6LoWPAN();

    Task_Params_init(&taskCommParams);
    taskCommParams.stackSize = STACKSIZE;
    taskCommParams.stack = &taskCommStack;
    taskCommParams.priority=1;

    taskComm = Task_create(commFxn, &taskCommParams, NULL);
    if (taskComm == NULL) {
    	System_abort("Task create failed!");
    }

    System_printf("Hello darkness, my old friend!\n");
    System_flush();
    
    /* Start BIOS */
    BIOS_start();

    return (0);
}

