
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/delay.h>

#ifndef __COMMON_H
#define __COMMON_H


#define USART_BAUD_RATE 9600

#define PORT_DATA_BUS PORTA
#define DDR_DATA_BUS DDRA
#define PIN_DATA_BUS PINA

#define PORT_ADDRESS_BUS PORTB
#define DDR_ADDRESS_BUS DDRB
#define PIN_ADDRESS_BUS PINB

#define PORT_SIG1 PORTC
#define DDR_SIG1 DDRC
#define PIN_SIG1 PINC

#define PORT_SIG2 PORTD
#define DDR_SIG2 DDRD
#define PIN_SIG2 PIND

#define BUSRQ_SIG1 PC3
#define BUSACK_SIG1 PC4
#define WAIT_SIG1 PC5
#define WAIT_RESET_SIG1 PC6
#define CHANGE_BANK_ADDR_SIG1 PC7

#define USER_LED_SWITCH_SIG2 PD2
#define RD_SIG2 PD3
#define WR_SIG2 PD4
#define MREQ_SIG2 PD5
#define RESET_SIG2 PD6

#define MEM_OE_SIG2 RD_SIG2
#define MEM_WE_SIG2 WR_SIG2
#define MEM_CE_SIG2 MREQ_SIG2

#define IO_PORT_CONSOLE 0
#define IO_PORT_USER_LED_SWITCH 1
#define IO_PORT_MEMORY_BANK 2

#define IO_USER_DISABLE_LED 0
#define IO_USER_ENABLE_LED 1
#define IO_USER_SELECT_LED_MODE 2
#define IO_USER_SELECT_SWITCH_MODE 3

#define CMD_LOAD_Z80_BINARY_FROM_USART 1
#define CMD_ECHO 2
#define CMD_UPLOAD_CODE_TO_RAM 3
#define CMD_DOWNLOAD_FIRST_16_BYTE_FROM_RAM 4

#define ERR_OK 0
#define ERR_TIMEOUT 1
#define ERR_INVALID_SIZE 2
#define ERR_CHECKSUM_FAILED 3
#define ERR_UNKNOWN_COMMAND 4
#define ERR_USART_FRAME_ERROR 5
#define ERR_USART_BUFFER_OVERFLOW 6

#define INLINE static __attribute__((always_inline)) inline

#endif