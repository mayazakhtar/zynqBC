
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xgpio.h"
#include "xstatus.h"
#include "xspi.h"
#include "Xil_io.h"

XGpio Gpio;
XSpi_Config *ConfigPtr;	/* Pointer to Configuration data */
XSpi Spi;

int spiInit(void);

//function to read from 9dof accelerometer/gyro registers using spi
void dof_Read(u8 Add, u8 * buffer, u8 N){
	u8 Write[N+1];
	Write[0] = (Add & 0x7F) | 0x80;			//R/W = 1
	XSpi_Transfer(&Spi, Write, buffer, N+1);
}

void mag_Read(u8 Add, u8 * buffer, u8 N){
	u8 Write[N+1];
	Write[0] = (Add & 0x3F) | 0xC0;			//R/W = 1 MS=1
	XSpi_Transfer(&Spi, Write, buffer, N+1);
}

//function to write in 9dof accelerometer/gyro registers using spi
void dof_Write(u8 Add, u8 val){
	u8 Write[2], Read[2];
	Write[0] = Add & 0x7F;		//R/W = 0
	Write[1] = val;
	XSpi_Transfer(&Spi, Write, Read, 2);
}

struct GLOBALS {
	int16_t axaxis,ayaxis,azaxis,gxaxis,gyaxis,gzaxis,mxaxis,myaxis,mzaxis;
} g;

//this function is to select gyro/accelerometer sensor reading or magnetic sensor reading
int selector(int sel){    //set sel 0 for magnetic sensor and 1 for gyro/accelerometer sensor
	int Status;
	Status = XGpio_Initialize(&Gpio, 0);
	if (Status != XST_SUCCESS) {
		xil_printf("Gpio Initialisation Failed\r\n");
	    return XST_FAILURE;
	}
	XGpio_DiscreteWrite(&Gpio, 1, sel);
}

//function for initialising gyro sensor
void initGyro(){
	dof_Write(0x10,0xC0);
	dof_Write(0x11,0x00);
	dof_Write(0x12,0x00);
	dof_Write(0x1E,0x38);
}

//function for initialising accelerometer sensor
void initAccel(){
	dof_Write(0x1F,0x38);
	dof_Write(0x20,0x00);
	dof_Write(0x21,0x00);
}

//function for initialising magnetic sensor
void initMag(){
	dof_Write(0x20,0x1C);
	dof_Write(0x21,0x00);
	dof_Write(0x22,0x00);
	dof_Write(0x23,0x00);
	dof_Write(0x24,0x00);
}

//function for reading values from accelerometer sensors registers
float get_acceleration(){
	selector(1);     //setting selector to 1 for accelerometer sensor
	int16_t ax1,ax2,ay1,ay2,az1,az2;
	u8 accelbuff[10]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	dof_Read(0x27,accelbuff,7);   //reading accelerometer values from register
	ax1=(int16_t)accelbuff[2];
	ax2=(int16_t)accelbuff[3];
	ay1=(int16_t)accelbuff[4];
	ay2=(int16_t)accelbuff[5];
	az1=(int16_t)accelbuff[6];
	az2=(int16_t)accelbuff[7];
	g.axaxis=(ax2<<8)|ax1;
	g.ayaxis=(ay2<<8)|ay1;
	g.azaxis=(az2<<8)|az1;
}

//function for reading values from gyro sensors registers
float get_gyro(){
	selector(1);   //setting selector to 1 for gyro sensor
	int16_t gx1,gx2,gy1,gy2,gz1,gz2;
	u8 gyrobuff[10]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	dof_Read(0x17,gyrobuff,7);  //reading gyro values from register
	gx1=(int16_t)gyrobuff[2];
	gx2=(int16_t)gyrobuff[3];
	gy1=(int16_t)gyrobuff[4];
	gy2=(int16_t)gyrobuff[5];
	gz1=(int16_t)gyrobuff[6];
	gz2=(int16_t)gyrobuff[7];
	g.gxaxis=(gx2<<8)|gx1;
	g.gyaxis=(gy2<<8)|gy1;
	g.gzaxis=(gz2<<8)|gz1;
}

//function for reading values from magnetic sensors registers
float get_mag(){
	selector(0);   //setting selector to 0 for magnetic sensor
	int16_t mx1,mx2,my1,my2,mz1,mz2;
	u8 magbuff[10]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	mag_Read(0x27,magbuff,7);  //reading magnetic sensors values from registers
	mx1=(int16_t)magbuff[2];
	mx2=(int16_t)magbuff[3];
	my1=(int16_t)magbuff[4];
	my2=(int16_t)magbuff[5];
	mz1=(int16_t)magbuff[6];
	mz2=(int16_t)magbuff[7];
	g.mxaxis=(mx2<<8)|mx1;
	g.myaxis=(my2<<8)|my1;
	g.mzaxis=(mz2<<8)|mz1;
}

int main()
{
    init_platform();
    spiInit();
    initAccel();
    initGyro();
    initMag();

    while(1){
    	get_acceleration();
    	get_gyro();
    	get_mag();
    }

    cleanup_platform();
    return 0;
}

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
