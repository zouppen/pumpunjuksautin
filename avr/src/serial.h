// -*- mode: c; c-file-style: "linux" -*-

#define SERIAL_TX_LEN 40

// Serial buffer which is sent on invocation of serial_tx_start().
extern char serial_tx[SERIAL_TX_LEN];

// Initialize serial port.
void serial_init(void);

// Gets a message from serial receive buffer, if any.
char const *serial_pull_message(void);

// Start half-duplex transmission (disables rx).
void serial_tx_start(void);

