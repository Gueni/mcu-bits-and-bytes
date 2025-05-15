#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

volatile int wdt_flag = 1;
volatile int tempo = 0;

/**
 * Watchdog Interrupt Service Routine
 * Executed on watchdog timeout (no reset, just interrupt)
 */
ISR(WDT_vect)
{
    tempo++;
}

/**
 * Enters power-down sleep mode and wakes up via WDT
 */
void enterSleep(void)
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    sleep_mode(); // MCU sleeps here, wakes on WDT interrupt

    sleep_disable();           // Disable sleep after waking
    power_all_enable();        // Re-enable all peripherals
}

/**
 * Setup serial and configure Watchdog Timer
 */
void setup()
{
    Serial.begin(9600);
    Serial.println("Initializing...");
    delay(100); // Allow time for serial output

    // Clear watchdog reset flag
    MCUSR &= ~(1 << WDRF);

    // Enable watchdog configuration changes
    WDTCSR |= (1 << WDCE) | (1 << WDE);

    // Set WDT timeout to approx 2 seconds
    WDTCSR = (1 << WDP3) | (1 << WDP0); // 2s prescaler

    // Enable WDT interrupt only (no reset)
    WDTCSR |= _BV(WDIE);

    Serial.println("Initialization complete.");
    delay(100);
}

/**
 * Main loop: MCU sleeps and wakes to blink PWM output
 */
void loop()
{
    enterSleep();
    analogWrite(6, 50); // Output PWM to pin 6 after wake-up
    delay(2000);        // Wait 2s before next sleep
}
