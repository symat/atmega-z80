#include "common.h"

#ifndef __LIB_USART_H
#define __LIB_USART_H


void setup_usart();


void setup_usart_timeout_timer();
void start_usart_timeout_timer(uint16_t bytes_expected);

uint8_t validate_usart_communication();

void usart_send_8(uint8_t data);

// length is one smaller than the actual length, enabling us to send 1..256 bytes
void usart_send_block(uint8_t* bytes, uint8_t length);

uint8_t usart_recv_8();
uint16_t usart_recv_16();

// fetching a block, validating the checksum.
// returning the actual block size in received_size
// both received_size and max_size is one less than the actual sizes (allowing to receive 1..256 bytes)
uint8_t fetch_block_from_usart(uint8_t* read_buffer, uint8_t* received_size, uint8_t max_size);

void flush_receive_buffer();

#endif