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

float get_acceleration(int* xaxis,int* yaxis,int* zaxis){
	int32_t x1,x2,y1,y2,z1,z2;
	u8 accelerometerbuff[10]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	dof_ag_Write(0x20,0x48);   //setting accelerometer settings
	dof_ag_Read(0x27,accelerometerbuff,7);   //reading accelerometer reading
	x1=(int32_t)accelerometerbuff[2];
	x2=(int32_t)accelerometerbuff[3];
	y1=(int32_t)accelerometerbuff[4];
	y2=(int32_t)accelerometerbuff[5];
	z1=(int32_t)accelerometerbuff[6];
	z2=(int32_t)accelerometerbuff[7];
	xaxis=(x2<<8)|x1;
	yaxis=(y2<<8)|y1;
	zaxis=(z2<<8)|z1;
	//return xaxis,yaxis,zaxis;
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
    	int xaxis,yaxis,zaxis;
    	get_acceleration(&xaxis,&yaxis,&zaxis);
    	for(int i=0; i<10;i++){};
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
