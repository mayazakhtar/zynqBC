/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xgpio.h"
#include "xstatus.h"
#include "xspi.h"
#include "Xil_io.h"

XSpi_Config *ConfigPtr;	/* Pointer to Configuration data */
XSpi Spi;

int spiInit(void);

//function to read from 9dof accelerometer/gyro registers using spi
void dof_ag_Read(u8 Add, u8 * buffer, u8 N){
	u8 Write[N+1];
	Write[0] = Add | 0x80;			//R/W = 1
	XSpi_Transfer(&Spi, Write, buffer, N+1);
}

//function to write in 9dof accelerometer/gyro registers using spi
void dof_ag_Write(u8 Add, u8 val){
	u8 Write[2], Read[2];
	Write[0] = Add & 0x7F;		//R/W = 0
	Write[1] = val;
	XSpi_Transfer(&Spi, Write, Read, 2);
}

void ag_check( void){
	u8 readbuffcheck[2];
	dof_ag_Read(0x0F,readbuffcheck,2);
	if (readbuffcheck[1]==0x68){
		print("AG Spi is working\n\r");
	}
	else {
		print("AG Spi is not working\n\r");
	}
}

float get_acceleration(int16_t* axaxis,int16_t* ayaxis,int16_t* azaxis){
	int16_t ax1,ax2,ay1,ay2,az1,az2;
	u8 accelerometerbuff[10]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	dof_ag_Write(0x20,0x48);   //setting accelerometer settings
	dof_ag_Read(0x27,accelerometerbuff,7);   //reading accelerometer values from register
	ax1=(int16_t)accelerometerbuff[2];
	ax2=(int16_t)accelerometerbuff[3];
	ay1=(int16_t)accelerometerbuff[4];
	ay2=(int16_t)accelerometerbuff[5];
	az1=(int16_t)accelerometerbuff[6];
	az2=(int16_t)accelerometerbuff[7];
	axaxis=(ax2<<8)|ax1;
	ayaxis=(ay2<<8)|ay1;
	azaxis=(az2<<8)|az1;
	//return xaxis,yaxis,zaxis;
}

float get_gyro(int16_t* gxaxis,int16_t* gyaxis,int16_t* gzaxis){
	int16_t gx1,gx2,gy1,gy2,gz1,gz2;
	u8 gyrobuff[10];
	dof_ag_Write(0x10,0x20);   //setting gyro
	dof_ag_Read(0x17,gyrobuff,7);  //reading gyro values from register
	gx1=(int16_t)gyrobuff[2];
	gx2=(int16_t)gyrobuff[3];
	gy1=(int16_t)gyrobuff[4];
	gy2=(int16_t)gyrobuff[5];
	gz1=(int16_t)gyrobuff[6];
	gz2=(int16_t)gyrobuff[7];
	gxaxis=(gx2<<8)|gx1;
	gyaxis=(gy2<<8)|gy1;
	gzaxis=(gz2<<8)|gz1;
}

int main()
{
    init_platform();
    spiInit();
    ag_check();

    while(1){
//    	u8 accelbuff[10]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
//    	dof_ag_Write(0x20,0x48);
//    	dof_ag_Read(0x28,accelbuff,6);
    	int16_t axaxis,ayaxis,azaxis,gxaxis,gyaxis,gzaxis;
    	get_acceleration(&axaxis,&ayaxis,&azaxis);
    	//for(int i=0; i<10;i++){};
    	get_gyro(&gxaxis,&gyaxis,&gzaxis);
    	//for(int i=0; i<10;i++){};
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
