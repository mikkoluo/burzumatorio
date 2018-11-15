/************************************************************* Sending messages *********************************/
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

// Napinpainalluksen käsittelijäfunktio create the pinInterruptHandler with the following
void button0Fxn(PIN_Handle handle, PIN_Id pinId)
{
	// Vaihdetaan led-pinnin tilaa negaatiolla
	// eli luetaan pinnin tila ja samantien asetetaan sen negaatio
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


/**************************************************** oma Task *****************************************************************/
Void taskFxn(UArg arg0, UArg arg1)
{
	I2C_Handle      i2c;
	I2C_Params      i2cParams;

	/* Create I2C for usage */
	I2C_Params_init(&i2cParams);
	i2cParams.bitRate = I2C_400kHz;
	// I2C_open-kutsulla avaamme yhteyden valittuun i2c-pinniin, tässä valmiiksi määritelty pinniin I2C0
	// Johon anturi on kytketty (onko?) define osoittaa: Board_I2C
	i2c = I2C_open(Board_I2C0, &i2cParams);
	if (i2c == NULL)
	{
		System_abort("Error Initializing I2C\n");
	}
	else
	{
		System_printf("I2C Initialized!\n");
		System_flush();
	}
/*********************************************************** Display ********************************************************/

	Display_Params displayParams;
	// Miksi seuraava ei vaikuta?
	displayParams.lineClearMode = DISPLAY_CLEAR_NONE;	//pyyhitäänkö edelliset pois alta DISPLAY_CLEAR_BOTH
	Display_Params_init(&displayParams);

	hDisplay = Display_open(Display_Type_LCD, &displayParams);

	if (hDisplay == NULL)
	{
		System_abort("Error initializing Display\n");
	}
	System_printf("oma Task lopussa\n");
	System_flush();
	//Display_clear(hDisplay);

} // Task loppuu

/************************************************** Communication Task **************************************************'*/
Void commFxn(UArg arg0, UArg arg1)
{

   char payload[80]; 	// viestipuskuri
   char str[80];		// tulostuspuskuri
   char tx_viesti[80];
   uint16_t senderAddr;
   uint8_t marker = 0;
   uint8_t viestinro = 1;

    // Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if(result != true)
	{
		System_abort("Wireless receive mode failed");
	}
	else
	{
		System_printf("Wireless receive OK\n");
		System_flush();
	}

	// YOUR CODE HERE TO SEND EXAMPLE MESSAGE
	//Display_print0(hDisplay, 1, 0, "Sending message");
	//Send6LoWPAN(0x29A, "Terve", 5);

	// Takaisin vastaanottotilaan
	//StartReceive6LoWPAN();

    while (1)
    {
    	// Lähetetään viestejä sekunnin välein
		sprintf(tx_viesti, "Terve%i", viestinro);
    	Display_print0(hDisplay, 1, 0, "Sending message");
    	Send6LoWPAN(0x29A, tx_viesti, 5);
    	Task_sleep(2 * 1000000 / Clock_tickPeriod); //kahden sekunnin venaus
		viestinro++;

    	//vastaanotto pois käytöstä
    	/*
    	 * if (GetRXFlag() == true)
    	{
    		// Viesti odottaa, luetaan viesti puskuriin
			 if (Receive6LoWPAN(&senderAddr, payload, 80) != -1)
			 {
				sprintf(str, "Viesti: %s\n", payload); // Tulostetaan viesti näytölle
				Display_print0(hDisplay, 3, 0, str);
			 }

        }
    	// Ei viestiä, ruudulle sepostusta
    	else
    	{
    		Display_print0(hDisplay, 0, 0, "Ei viestii...");
    		Display_print0(hDisplay, 1, marker, "-");
    	}
    	if (marker == 16)
    		marker = -1;
    	marker++;
    	*/

    }	// While loop loppuu
}		// Task loppuu

Int main(void) {

    // Task variables
	Task_Handle task;
	Task_Params taskParams;
	Task_Handle taskComm;
	Task_Params taskCommParams;

    // Initialize board
    Board_initGeneral();
    Board_initI2C();

	/* Buttons */
	hPowerButton = PIN_open(&sPowerButton, cPowerButton);
	if(!hPowerButton) {
		System_abort("Error initializing button shut pins\n");
	}
	if (PIN_registerIntCb(hPowerButton, &powerButtonFxn) != 0) {
		System_abort("Error registering button callback function");
	}
	/* omaa paskaa: Button0 */
	//
	hButton0 = PIN_open(&sButton0, cButton0);
	if(!hButton0) {
		System_abort("Error initializing button shut pins\n");
	}
    /* Leds */
    hLed = PIN_open(&sLed, cLed);
    if(!hLed) {
        System_abort("Error initializing LED pin\n");
    }
	/* enable interrupts and register the callback:
	 * pinnin keskeytyksen käsittelijä, parametrina funktio: button0Fxn*/
	if (PIN_registerIntCb(hButton0, &button0Fxn) != 0) {
		System_abort("Error registering button callback function");
	}

    /* Task */
	// Täällä aletaan ajamaan omaa taskia taskFxn
	// Siihen piirtelyt ja muut
    Task_Params_init(&taskParams);
    taskParams.stackSize = STACKSIZE;
    taskParams.stack = &taskStack;
    taskParams.priority = 2; //Miksi 2?

    task = Task_create(taskFxn, &taskParams, NULL);
    if (task == NULL) {
    	System_abort("Task create failed!");
    }

    /* Communication Task */
    Init6LoWPAN();

    Task_Params_init(&taskCommParams);
    taskCommParams.stackSize = STACKSIZE;
    taskCommParams.stack = &taskCommStack;
    taskCommParams.priority = 1;

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

