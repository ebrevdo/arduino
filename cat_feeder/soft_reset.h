#include <avr/wdt.h>


// Function Implementation
void setup_soft_reset()
{
    MCUSR = 0;
    wdt_disable();

    return;
}

void soft_reset() {
    cli();                  // Clear interrupts
    wdt_enable(WDTO_15MS);      // Set the Watchdog to 15ms
    while(1);            // Enter an infinite loop
}
