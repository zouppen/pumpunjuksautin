// RS-485 interface

#define SERIAL_RX_LEN 50
#define SERIAL_TX_LEN 80

typedef struct {
	int good;         // Number of good frames
	int flip_timeout; // Number of missed flips
	int too_long;     // Number of too long frames
} serial_counter_t;

// Serial buffer which is sent on invocation of serial_tx_start().
extern char serial_tx[SERIAL_TX_LEN];

// Initialize serial port.
void serial_init(void);

// Is serial transmitter on?
bool serial_is_transmitting(void);

// Gets a message from serial receive buffer, if any.  The returned
// buffer is immutable. Buffer must be released after processing with
// serial_free_message()
uint8_t serial_get_message(uint8_t **const buf);

// Release receive buffer. This is important to do as soon as
// possible. If frame is not freed before back buffer is filled,
// error_flip_timeout occurs.
void serial_free_message(void);

// Start half-duplex transmission (disables rx).
void serial_tx_start(void);

// Get serial counters and zero them
serial_counter_t pull_serial_counters(void);
