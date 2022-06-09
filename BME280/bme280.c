
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xstatus.h"
#include "xspi.h"
#include "Xil_io.h"
#include <math.h>

XSpi_Config *ConfigPtr;	/* Pointer to Configuration data */
XSpi Spi;

float readbmetemp();
float readbmepress();
float readbmehumid();
void BMEconfig();
int spiInit(void);

#define BME280_REGISTER_DIG_TEMP_START 0x88
#define BME280_REGISTER_DIG_PRES_START 0x8E
#define BME280_REGISTER_DIG_HUM_START  0xE1

//function to read from bme registers using spi
void BME_Read(u8 Add, u8 * buffer, u8 N){
	u8 Write[N+1];
	Write[0] = Add | 0x80;			//R/W = 1
	XSpi_Transfer(&Spi, Write, buffer, N+1);
}
//function to write in bme registers using spi
void BME_Write(u8 Add, u8 val){
	u8 Write[2], Read[2];
	Write[0] = Add & 0x7F;		//R/W = 0
	Write[1] = val;
	XSpi_Transfer(&Spi, Write, Read, 2);
}


void BME_Wait(void) {
	u8 Read[2];
	do{
		BME_Read(0xF3, Read, 1 );
	} while (Read[1] != 0);
}

//declaring global variables
struct GLOBALS {
	int32_t t_fine;
	int32_t read_dig_t1, read_dig_t2, read_dig_t3;
	int64_t read_dig_p1, read_dig_p2, read_dig_p3, read_dig_p5, read_dig_p4, read_dig_p6, read_dig_p7, read_dig_p8, read_dig_p9;
	int32_t read_dig_h1, read_dig_h2, read_dig_h3, read_dig_h5, read_dig_h4, read_dig_h6;
} g;

//function to read trimming parameters
void trimming_readout(){
	u8 readtemp[10];
	u8 readpress[20];
	u8 readhumid[10];
	u8 readhumidA1[2];

	BME_Read(BME280_REGISTER_DIG_TEMP_START,readtemp,6);

	g.read_dig_t1 = (unsigned short)(readtemp[1] + (u16)readtemp[2]*256);
	g.read_dig_t2 = (signed short)(readtemp[3] + (u16)readtemp[4]*256);
	g.read_dig_t3 = (signed short)(readtemp[5] + (u16)readtemp[6]*256);

	BME_Read(BME280_REGISTER_DIG_PRES_START,readpress,18);

	g.read_dig_p1 = (unsigned short)(readpress[1] + (u16)readpress[2]*256);
	g.read_dig_p2 = (signed short)(readpress[3] + (u16)readpress[4]*256);
	g.read_dig_p3 = (signed short)(readpress[5] + (u16)readpress[6]*256);
	g.read_dig_p4 = (signed short)(readpress[7] + (u16)readpress[8]*256);
	g.read_dig_p5 = (signed short)(readpress[9] + (u16)readpress[10]*256);
	g.read_dig_p6 = (signed short)(readpress[11] + (u16)readpress[12]*256);
	g.read_dig_p7 = (signed short)(readpress[13] + (u16)readpress[14]*256);
	g.read_dig_p8 = (signed short)(readpress[15] + (u16)readpress[16]*256);
	g.read_dig_p9 = (signed short)(readpress[17] + (u16)readpress[18]*256);

	BME_Read(0xA1,readhumidA1,1);
	BME_Read(BME280_REGISTER_DIG_HUM_START,readhumid,7);
	g.read_dig_h1 = (unsigned char)readhumidA1[1];
	g.read_dig_h2 = (signed short)(readhumid[1] + (u16)readhumid[2]*256);
	g.read_dig_h3 = (unsigned char)readhumid[3];
	g.read_dig_h4 = (signed short)((readhumid[5] & 0x0F) + (u16)readhumid[4]*16);
	g.read_dig_h5 = (signed short)(((readhumid[5] & 0xF0)>>4) + ((u16)readhumid[6]*16));
	g.read_dig_h6 = (signed char)readhumid[7];
}

int main()
{
    init_platform();
    spiInit();
    BME_Write(0xE0, 0xB6); //Reset the Device
    BME_Wait();
    trimming_readout();

    while(1){
    	float temp1=readbmetemp();    //Reading temperature
    	for(int i=0; i<10;i++){};
    	float press=readbmepress();   //Reading pressure
    	for(int i=0; i<10;i++){};
    	float humid = readbmehumid();  //Reading humidity
    	for(int i=0; i<10;i++){};
    }
    cleanup_platform();
    return 0;
}

//function to initialise spi
int spiInit(void){
	int Status;
	ConfigPtr = XSpi_LookupConfig(0U);
	if (ConfigPtr == NULL) {
		return XST_DEVICE_NOT_FOUND;
	}
	Status = XSpi_CfgInitialize(&Spi, ConfigPtr,ConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	Status = XSpi_SetOptions(&Spi, XSP_MASTER_OPTION );
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	XSpi_Start(&Spi);
	XSpi_IntrGlobalDisable(&Spi);
	XSpi_SetSlaveSelect(&Spi, 0x01);
}

//function to calculate temperature
float readbmetemp(){
	int32_t tvar0, tvar1=0;
	int32_t tvar2=0;

	u8 Readbufftemp[10];

	BME_Read(0xD0, Readbufftemp, 1);
	if(Readbufftemp[1] != 0x60){
		return NAN;
	}

	BME_Write(0xF4, 0x41);	//Calculate only temperature in Forced Mode

	BME_Read(0xF7, Readbufftemp, 8);

	int32_t read_adc = {(Readbufftemp[6]>>4) + (int32_t)Readbufftemp[5] * 16 + (int32_t)Readbufftemp[4] * 256 * 16};

	tvar0 = (read_adc>>3)-(g.read_dig_t1<<1);
	tvar1 = (tvar0*g.read_dig_t2)>>11;
	tvar2 = ((((tvar0 >> 1)*(tvar0 >> 1))>>12)*(g.read_dig_t3))>>14;

	g.t_fine = tvar1 + tvar2 ;
	int32_t T = (g.t_fine * 5 + 128) / 256;
	return (float) T/100.0;
}

//function to calculate pressure
float readbmepress(){
	readbmetemp();
	int64_t pvar1, pvar2, pvar3, pvar4=0;
	u8 Readbuffpress[10]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    BME_Write(0xE0, 0xB6); //Reset the Device
    BME_Wait();

	BME_Read(0xD0, Readbuffpress, 1);
	if(Readbuffpress[1] != 0x60){
		return NAN;
	}

	BME_Write(0xF4, 0x09);	//Calculate only pressure in Forced Mode

	BME_Read(0xF7, Readbuffpress, 8);

	int32_t adc_p = {(Readbuffpress[3]>>4) + (int32_t)Readbuffpress[2] * 16 + (int32_t)Readbuffpress[1] * 256 * 16};

	pvar1 = ((int64_t)g.t_fine) - 128000;
	pvar2 = pvar1 * pvar1 * g.read_dig_p6;
	pvar2 = pvar2 + ((pvar1 * g.read_dig_p5) * 131072);
	pvar2 = pvar2 + ((g.read_dig_p4) * 34359738368);
	pvar1 = ((pvar1 * pvar1 * g.read_dig_p3) / 256) + ((pvar1 * (g.read_dig_p2) * 4096));
	pvar3 = ((int64_t)1) * 140737488355328;
	pvar1 = (pvar3 + pvar1) * (g.read_dig_p1) / 8589934592;

	if (pvar1 == 0) {
	   return 0; // avoid exception caused by division by zero
	}

	pvar4 = 1048576 - adc_p;
	pvar4 = (((pvar4 * 2147483648) - pvar2) * 3125) / pvar1;
	pvar1 = ((g.read_dig_p9) * (pvar4 / 8192) * (pvar4 / 8192)) / 33554432;
	pvar2 = ((g.read_dig_p8) * pvar4) / 524288;
	pvar4 = ((pvar4 + pvar1 + pvar2) / 256) + ((g.read_dig_p7) * 16);

	float P = pvar4 / 256.0;

	return P;
}

//function to calculate humidity
float readbmehumid(){
	readbmetemp();
	int32_t hvar1, hvar2, hvar3, hvar4, hvar5;

	u8 Readbuffhumid[3]={0x00,0x00,0x00};

	BME_Read(0xD0, Readbuffhumid, 1);
	if(Readbuffhumid[1] != 0x60){
		return NAN;
	}

	BME_Write(0xF2, 0x02);	//Calculate only humidity
	BME_Read(0xFD, Readbuffhumid, 2);

	int32_t adc_h = {(Readbuffhumid[2]) + (int32_t)Readbuffhumid[1] * 16 *16};
	hvar1 = g.t_fine - ((int32_t)76800);
	hvar2 = (int32_t)(adc_h * 16384);
	hvar3 = (int32_t)(((int32_t)g.read_dig_h4) * 1048576);
	hvar4 = ((int32_t)g.read_dig_h5) * hvar1;
	hvar5 = (((hvar2 - hvar3) - hvar4) + (int32_t)16384) / 32768;
	hvar2 = (hvar1 * ((int32_t)g.read_dig_h6)) / 1024;
	hvar3 = (hvar1 * ((int32_t)g.read_dig_h3)) / 2048;
	hvar4 = ((hvar2 * (hvar3 + (int32_t)32768)) / 1024) + (int32_t)2097152;
	hvar2 = ((hvar4 * ((int32_t)g.read_dig_h2)) + 8192) / 16384;
	hvar3 = hvar5 * hvar2;
	hvar4 = ((hvar3 / 32768) * (hvar3 / 32768)) / 128;
	hvar5 = hvar3 - ((hvar4 * ((int32_t)g.read_dig_h1)) / 16);
	hvar5 = (hvar5 < 0 ? 0 : hvar5);
	hvar5 = (hvar5 > 419430400 ? 419430400 : hvar5);
	uint32_t H = (uint32_t)(hvar5 / 4096);

	return (float)H / 1024.0;
}
