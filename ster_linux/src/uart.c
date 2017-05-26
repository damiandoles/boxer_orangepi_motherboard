/*
 * uart.c
 *
 *  Created on: 26 May 2017
 *      Author: dolewdam
 */

#include "uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

#define RX_BUFF_SIZE	64

static char rxBuffer[RX_BUFF_SIZE] = {0};
static int rxBuffIndex = 0;
static int uart0_filestream = -1;

void Uart_Init(void)
{
	//----- SETUP USART 0 -----
	//-------------------------
	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	uart0_filestream = open("/dev/ttyACM0", O_RDWR | O_NOCTTY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}

	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;

	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);
}

void Uart_RxHandler(void)
{
	//----- CHECK FOR ANY RX BYTES -----
	if (uart0_filestream != -1)
	{
		char recvByte = 0;
		if (read(uart0_filestream, &recvByte, 1) != 0)
		{
			rxBuffer[rxBuffIndex] = recvByte;
			rxBuffIndex++;

			char * startStrAddr  = strstr(rxBuffer, "STA");

			if (startStrAddr != NULL)
			{
				char * endStrAddr    = strstr(rxBuffer, "END");
				if (endStrAddr != NULL)
				{
					char ReceivedString [8][32] = {{0},{0}};
					char * splitStr = 0;
					int i = 0;
					splitStr = strtok (rxBuffer, " ");
					strcpy(ReceivedString[i], splitStr);

					while (splitStr != NULL)
					{
						if (i < RX_BUFF_SIZE)
						{
							i++;
						}
						else
						{
							break;
						}

						splitStr = strtok (NULL, " ");
						strcpy(ReceivedString[i], splitStr);
					}

					if (strcmp(ReceivedString[0], "STA") == 0)
					{
						printf("STA ");
						if (strcmp(ReceivedString[1], "SL") == 0)
						{
							printf("SL %s %s %s ", ReceivedString[2], ReceivedString[3], ReceivedString[4]);
							if (strcmp(ReceivedString[5], "END") == 0)
							{
								printf("END\r\n");
								rxBuffIndex = 0;
								memset(rxBuffer, 0, RX_BUFF_SIZE);
							}
						}
						else if (strcmp(ReceivedString[1], "ST") == 0)
						{
							printf("ST %s %s ", ReceivedString[2], ReceivedString[3]);
							if (strcmp(ReceivedString[4], "END") == 0)
							{
								printf("END\r\n");
								rxBuffIndex = 0;
								memset(rxBuffer, 0, RX_BUFF_SIZE);
							}
						}
						else if (strcmp(ReceivedString[1], "SF") == 0)
						{
							printf("SF %s %s ", ReceivedString[2], ReceivedString[3]);
							if (strcmp(ReceivedString[4], "END") == 0)
							{
								printf("END\r\n");
								rxBuffIndex = 0;
								memset(rxBuffer, 0, RX_BUFF_SIZE);
							}
						}
						else if (strcmp(ReceivedString[1], "CP") == 0)
						{
							printf("CP %s ", ReceivedString[2]);
							if (strcmp(ReceivedString[3], "END") == 0)
							{
								printf("END\r\n");
								rxBuffIndex = 0;
								memset(rxBuffer, 0, RX_BUFF_SIZE);
							}
						}
						else if (strcmp(ReceivedString[1], "SI") == 0)
						{
							printf("SI %s %s %s ", ReceivedString[2], ReceivedString[3], ReceivedString[4]);
							if (strcmp(ReceivedString[5], "END") == 0)
							{
								printf("END\r\n");
								rxBuffIndex = 0;
								memset(rxBuffer, 0, RX_BUFF_SIZE);
							}
						}
						else if (strcmp(ReceivedString[1], "R") == 0)
						{
							printf("R ");
							if (strcmp(ReceivedString[2], "END") == 0)
							{
								printf("END\r\n");
								rxBuffIndex = 0;
								memset(rxBuffer, 0, RX_BUFF_SIZE);
							}
						}
						else if (strcmp(ReceivedString[1], "DEF") == 0)
						{
							printf("DEF ");
							if (strcmp(ReceivedString[2], "SETT") == 0)
							{
								printf("SETT ");
								if (strcmp(ReceivedString[3], "END") == 0)
								{
									printf("END\r\n");
									rxBuffIndex = 0;
									memset(rxBuffer, 0, RX_BUFF_SIZE);
								}
							}
						}
						else
						{
							rxBuffIndex = 0;
							memset(rxBuffer, 0, RX_BUFF_SIZE);
						}
					}
					else
					{
						rxBuffIndex = 0;
						memset(rxBuffer, 0, RX_BUFF_SIZE);
					}
				}
			}
		}
	}
}