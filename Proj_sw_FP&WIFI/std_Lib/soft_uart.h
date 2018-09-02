
#ifndef	__SOFT_UART_H
#define	__SOFT_UART_H

#include	"config.h"

void TxSend(u8 dat);
void LogString(unsigned char code *puts);
void LogDats(u8 *dats, u8 datsLen);

#endif
