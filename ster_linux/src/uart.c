/*
 * uart.c
 *
 *  Created on: 26 May 2017
 *      Author: Damian Dolewski
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <errno.h>

#include "uart.h"
#include "types.h"
#include "database.h"
#include "debug.h"
#include "defines.h"

#define RX_BUFF_SIZE				64
#define MAX_SPLITED_COUNT			16
#define MAX_SPLITED_LEN				32

static char rxBuffer[RX_BUFF_SIZE] 	= {0};
static unsigned int rxBuffIndex 	= 0;
static int uart_stream 				= -1;

void * uartRxThread(void * param);
void * uartTxThread(void * param);



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
//
// CONFIGURE THE UART
// The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600,
//  B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
//	CSIZE:- CS5, CS6, CS7, CS8
//	CLOCAL - Ignore modem status lines
//	CREAD - Enable receiver
//	IGNPAR = Ignore characters with parity errors
//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
//	PARENB - Parity enable
//	PARODD - Odd parity (else even)
void Uart_Init(void)
{
	uart_stream = open(SERIAL_PORT_NAME, O_RDWR | O_NOCTTY);		//Open in non blocking read/write mode
	if (uart_stream == -1)
	{
#ifdef DEBUG_UART_RX
		printf("Uart_Init[Error]: Unable to open UART. Ensure it is not in use by another application\n\r");
#endif
		exit(EXIT_FAILURE);
	}
	else
	{
		struct termios uart;
		tcgetattr(uart_stream, &uart);
		uart.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
		uart.c_iflag = IGNPAR;
		uart.c_oflag = 0;
		uart.c_lflag = 0;

		tcflush(uart_stream, TCIFLUSH);
		tcsetattr(uart_stream, TCSANOW, &uart);

#ifdef DEBUG_UART_RX
		printf("Uart_Init[Success]: Opened port COM successfully\n\r");
#endif
		errno = pthread_create(&uartTxThreadHandler, NULL, uartTxThread, NULL);
		errno = pthread_create(&uartRxThreadHandler, NULL, uartRxThread, NULL);
	}
}

void Uart_RxHandler(void)
{
	if (uart_stream != -1)
	{
		char recvByte = 0;
		if (read(uart_stream, &recvByte, 1) != 0)
		{
			rxBuffer[rxBuffIndex] = recvByte;
			if (rxBuffer[0] != 'S')
			{
				rxBuffIndex = 0;
				memset(rxBuffer, 0, RX_BUFF_SIZE);
			}
			else
			{
				if (rxBuffIndex < RX_BUFF_SIZE)
				{
					rxBuffIndex++;
				}
				else
				{
					rxBuffIndex = 0;
					memset(rxBuffer, 0, RX_BUFF_SIZE);
				}

				char * startStrAddr  = strstr(rxBuffer, "STA");

				if (startStrAddr != NULL)
				{
					char * endStrAddr    = strstr(rxBuffer, "END");
					if (endStrAddr != NULL)
					{
						char ReceivedString [MAX_SPLITED_COUNT][MAX_SPLITED_LEN] = {{0},{0}};
						char * splitStr = 0;
						int i = 0;
						splitStr = strtok (rxBuffer, " ");
						strcpy(ReceivedString[i], splitStr);

						while (splitStr != NULL)
						{
							if (i < MAX_SPLITED_COUNT - 1)
							{
								i++;
							}
							else
							{
								break;
							}

							splitStr = strtok (NULL, " ");
							if (splitStr != 0x0)
							{
								strcpy(ReceivedString[i], splitStr);
							}
							else
							{
								break;
							}
						}

						if (strcmp(ReceivedString[0], "STA") == 0)
						{
							#ifdef DEBUG_UART_RX
							printf("Received frame: STA ");
							#endif
							if (strcmp(ReceivedString[1], "PRIMARYMEAS") == 0)
							{
								//STA PRIMARYMEAS hum lux temp_up temp_middle temp_down soil_moisture END
								#ifdef DEBUG_UART_RX
								printf("PRIMARYMEAS %s %s %s %s %s %s ",
										ReceivedString[2],
										ReceivedString[3],
										ReceivedString[4],
										ReceivedString[5],
										ReceivedString[6],
										ReceivedString[7]);
								#endif

								if (strcmp(ReceivedString[8], "END") == 0)
								{
									#ifdef DEBUG_UART_RX
									printf("END\r\n");
									#endif
									basic_meas_t * recvMeasData = (basic_meas_t *)calloc(sizeof(recvMeasData), sizeof(basic_meas_t));

									strcpy(recvMeasData->humidity, 		ReceivedString[2]);
									strcpy(recvMeasData->lux, 			ReceivedString[3]);
									strcpy(recvMeasData->temp_up, 		ReceivedString[4]);
									strcpy(recvMeasData->temp_middle,	ReceivedString[5]);
									strcpy(recvMeasData->temp_down, 	ReceivedString[6]);
									strcpy(recvMeasData->soil_moist, 	ReceivedString[7]);

									DataBase_InsertBasicMeas(recvMeasData);

									rxBuffIndex = 0;
									memset(rxBuffer, 0, RX_BUFF_SIZE);
								}
							}
							else if (strcmp(ReceivedString[1], "PHW") == 0)
							{
								//STA PHW waterPh END
								#ifdef DEBUG_UART_RX
								printf("PHW %s ", ReceivedString[2]);
								#endif
								if (strcmp(ReceivedString[3], "END") == 0)
								{
									#ifdef DEBUG_UART_RX
									printf("END\r\n");
									#endif
									ph_meas_t * recvPhMeas = (ph_meas_t *)calloc(sizeof(recvPhMeas), sizeof(ph_meas_t));
									strcpy(recvPhMeas->ph_water, ReceivedString[2]);

									DataBase_InsertPhMeas(recvPhMeas);

									rxBuffIndex = 0;
									memset(rxBuffer, 0, RX_BUFF_SIZE);
								}
							}
							else if (strcmp(ReceivedString[1], "PHS") == 0)
							{
								//STA PHS soilPh END
								#ifdef DEBUG_UART_RX
								printf("PHS %s ", ReceivedString[2]);
								#endif
								if (strcmp(ReceivedString[3], "END") == 0)
								{
								#ifdef DEBUG_UART_RX
									printf("END\r\n");
								#endif
									ph_meas_t * recvPhMeas = (ph_meas_t *)calloc(sizeof(recvPhMeas), sizeof(ph_meas_t));
									strcpy(recvPhMeas->ph_soil,  ReceivedString[2]);

									DataBase_InsertPhMeas(recvPhMeas);

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
}

void * uartRxThread(void * param)
{
	(void)param;
	while (true)
	{
		Uart_RxHandler();
	}

	pthread_exit(NULL);
}

void * uartTxThread(void * param)
{
	(void)param;
	while (true)
	{
		sleep(2);
#ifdef DEBUG_UART_TX
		printf("UART TX THREAD\r\n");
#endif
	}

	pthread_exit(NULL);
}
