/*
 * Outputs: TIOCM_DTR(4), TIOCM_RTS(7)
 * Inputs:  TIOCM_DSR(6), TIOCM_CTS(8), TIOCM_DCD(1)
 * Useless: TIOCM_RNG(9)
 *
 * TIOCMIWAIT on TIOCM_RNG only returns on the 1->0 transition
 */
#define SERIO_OUT TIOCM_DTR
#define SERIO_IN  TIOCM_DSR

typedef struct {
	struct timeval tv;
	bool on;
} __attribute__((__packed__)) pulse_t;

#define xerror(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
#define cerror(msg, expr) do { if (expr) xerror(msg); } while(0)
