
#define OPT3001_DATA_REG	0x00 	// Vakio: sensorin datarekisterin osoite
#define Board_OPT3001_ADDR	0x45  	// tämä oliki jo muuallaki

// Muuntaa datan LUX-arvoksi, mutta ei tätä nyt tarvi varsinaisesti
float sensorOpt3001Convert(uint16_t rawData)
{
  uint16_t e, m;
 
  m = rawData & 0x0FFF;
  e = (rawData & 0xF000) >> 12;
 
  return m * (0.01 * pow(2.0,e));
}

// Light Sensor Taskifunktio
Void light_sensorFxn(UArg arg0, UArg arg1) 
{
	unsigned int    i;
	float        	valoisuus;
	uint8_t         txBuffer[1];
	uint8_t         rxBuffer[2];
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
		valoisuus = 0.01 * (1 << (rxBuffer >> 12)) * (rxBuffer & 0x0FFF);
		
		System_printf("Valoisuus on %.2f C\n", valoisuus);
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