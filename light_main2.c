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
	unsigned int    i;
	float        	valoisuus;
	uint8_t         txBuffer[1];
	uint16_t        rxBuffer[1];
	I2C_Handle      i2c;
	I2C_Params      i2cParams;
	I2C_Transaction i2cTransaction;

	I2C_Params_init(&i2cParams);
	i2cParams.bitRate = I2C_400kHz;
	i2c = I2C_open(Board_I2C0, &i2cParams); // mikä ekaksi parametriksi??

	if (i2c == NULL)
	{
		System_abort("Error Initializing I2C\n");
	}
    else
    {
        System_printf("I2C Initialized!\n");
    }

	txBuffer[0] = OPT3001_DATA_REG; 						// Laiterekisteri
	i2cTransaction.slaveAddress = Board_OPT3001_ADDR; 		// Laitteen osoite
	i2cTransaction.writeBuf = txBuffer; 					// Lähetyspuskuri
	i2cTransaction.writeCount = 1; 							// Datan koko
	i2cTransaction.readBuf = rxBuffer; 						// Vastaanottopuskuri
	i2cTransaction.readCount = 2; 							// Datan koko kaksi tavua

	// Kymmenen valoisuusarvoa
	for (i = 0; i < 10; i++)
	{
		if (I2C_transfer(i2c, &i2cTransaction))
		{
		/* Muunna luettu data rxBufferista valoisuusarvoksi datakirjassa kerrotun mukaisesti */
		valoisuus = 0.01 * (1 << (rxBuffer[0] >> 12)) * (rxBuffer[0] & 0x0FFF);

		System_printf("Valoisuus on %.2 LUX\n", valoisuus);
		System_flush();
		}
		else
		{
		System_printf("I2C Bus fault\n");
		}
	/* sekunnin viive */
	Task_sleep(1000000 / Clock_tickPeriod);
	}
	/* Sulje i2c-yhteys */
	I2C_close(i2c);
}



/********************************************************* OMA TASK ***********************************************/
Void taskFxn(UArg arg0, UArg arg1)
{
    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    // pressure-muuttujat myöhemmin tulevaa get_data funktiota varten
    double temp = 0.0;						// pitäsiköhän nämä olla muualla
    double pres = 0.0;

    /* Create I2C for usage */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    // I2C_open-kutsulla avaamme yhteyden valittuun i2c nolla-pinniin, tässä valmiiksi määritelty pinniin I2C0
    // define osoittaa: Board_I2C.... = 0
    i2c = I2C_open(Board_I2C0, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    else {
        System_printf("I2C Initialized!\n");
    }

    // SETUP SENSORS HERE
    bmp280_setup(&i2c);

    /* Display */
    // tietorakenne Display_Params löytyy Display.h kirjastosta
    // muutin BOTH -> NONE in Display.c
	Display_Params displayParams;
	// Miksi seuraava ei vaikuta??
	displayParams.lineClearMode = DISPLAY_CLEAR_NONE;//pyyhitäänkö edelliset pois alta DISPLAY_CLEAR_BOTH
	Display_Params_init(&displayParams);

	hDisplay = Display_open(Display_Type_LCD, &displayParams);
	if (hDisplay == NULL) {
	 System_abort("Error initializing Display\n");
	}

    // tänne varmaankin tutkimus siitä onko Tagilla jo Aasi
    // ja aloitus-menu sen mukaan

    while (1)
    {
/*	PIIRRETÄÄN RUUDULLE KERROSRAKENNE JA NUMEROINTI **********************************************************/

    	// kaikki piirtely oltava täällä, koska muuten ukkelin paikkaa ei voi muuttaa ja rivit pyyhkivät edeltävät
    	uint8_t ukko_x = 0, ukko_y = 0;			//bmp-ukkelin piirtokoordinaatit
		Display_clear(hDisplay);
		// pysyvät tulostukset, joiden väliin myöhemmin painetieto
		// miten poispyyhintä POIS?? Eri handleilla eri piirrot??
		//Display_print0(hDisplay, 0, 0, "Pres:");
		//Display_print0(hDisplay, 0, 7, "hPa");
		//Display_print0(hDisplay, 10, 8, "Temp:");

		// Kerrosnumerointi
		Display_print0(hDisplay, 1, 0, "5");
		Display_print0(hDisplay, 3, 0, "4");
		Display_print0(hDisplay, 5, 0, "3");
		Display_print0(hDisplay, 7, 0, "2");
		Display_print0(hDisplay, 9, 0, "1");

	/*	// labran nettiosoitteen tulostus sprinf:llä
		char osoite[3+1];
		sprintf(osoite, "In address %d", IEEE80154_MY_ADDR);
		Display_print0(hDisplay, 6, 1, osoite);
		Task_sleep(1000000 / Clock_tickPeriod); //sekunti
	*/
		// kerrosrakenteen piirto VIIVOILLA
		tContext *pContext = DisplayExt_getGrlibContext(hDisplay);
		if (pContext)
		{
		 //Display_clear(hDisplay);
		 GrLineDraw(pContext,13,8,13,84);	// pystyviiva
		 GrLineDraw(pContext,0,20,35,20);	// vaakaviivat kerroksille
		 GrLineDraw(pContext,0,36,35,36);
		 GrLineDraw(pContext,0,52,35,52);
		 GrLineDraw(pContext,0,68,35,68);
		 GrLineDraw(pContext,0,84,35,84);
		 GrFlush(pContext);
		 //Task_sleep(1000000 / Clock_tickPeriod); //sekunti
		}
/******************************************************************************* Painetulostukset ********************/
		//Display_clear(hDisplay);
		// muuttujat siirretty taskin alkuun
	    // seuraavassa NOUDETAAN painetieto
		bmp280_get_data(&i2c, &pres, &temp);

		float vertailu = pres;
		// iffeillä kerrostieto, sen myötä ukolle x ja y
		// ja jos ei mikään alla olevista...
		if (vertailu > 1086.0)								//kerros 5.
		{
			ukko_x = 16;
			ukko_y = 9;
		}
		if (vertailu > 1090.1)								//kerros 4.
		{
			ukko_x = 16;
			ukko_y = 25;
		}
		if (vertailu > 1090.4)								//kerros 3.
		{
			ukko_x = 16;
			ukko_y = 41;
		}
		if (vertailu > 1090.7)								//kerros 2.
		{
			ukko_x = 16;
			ukko_y = 56;
		}
		if (vertailu > 1091.0)								//kerros 1.
		{
			ukko_x = 16;
			ukko_y = 73;
		}

		char paine[15];												// eihän tähän pitäis edes mahtua...
		char lampo[15];												// minkälaiseen muuttujaan kanssii tallentaa
		sprintf(paine, "Pres:%.3fhPa", pres);
		Display_print0(hDisplay, 0, 0, paine);
		sprintf(lampo, "Temp: %.1f C", (temp-6.8));
		Display_print0(hDisplay, 11, 0, lampo);

/******************************************** Ukkeli bmp:nä ***********************************************************************/
		// Jos ukkelin siirron tekis 'maalaamalla' päälle tyhjää, ei tarvis piirtää koko roskaa uudestaan
		const uint8_t imgData[11] = {0x1C, 0x1C, 0x1C, 0x08, 0x3E, 0x49, 0x08, 0x1C, 0x2A, 0x41, 0x41}; //itse kuva
		uint32_t imgPalette[] = {0, 0xFFFFFF}; // värit musta ja valkoinen
		// Kuvan määrittelyt
		const tImage image =
		{
		    .BPP = IMAGE_FMT_1BPP_UNCOMP,
		    .NumColors = 2,
		    .XSize = 1,					// leveys tavuina
		    .YSize = 11,				// korkeus tavuina
		    .pPalette = imgPalette,
		    .pPixel = imgData
		};
		if (pContext)
		{
			// Kuvan paikan koordinaatit annettava muuttujina ukko_x, ukko_y
			GrImageDraw(pContext, &image, ukko_x, ukko_y);
			GrFlush(pContext);
			Task_sleep(2 * 1000000 / Clock_tickPeriod); //sekunti
		}
/**********************************************************************************************************************/
    } // whilen loppu
}	// taskFxn loppu

Int main(void) {

    // Task variables
	Task_Handle task;
	Task_Params taskParams;
	Task_Handle taskComm;
	Task_Params taskCommParams;

    // Initialize board
    Board_initGeneral();
    Board_initI2C();

/******************************************************* Buttons ***********************/
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
/***************************************************************************************/
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

/************************************************** Communication Task ********************/
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

