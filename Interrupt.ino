#define TIMERIRQ_RATE   500000                          // in microseconds; should give 0.5Hz toggles

HardwareTimer timer(2);
//---------------
void setup_interrupt_check() {                          //
    timer.pause();                                      // Pause the timer while we're configuring it
    timer.setPeriod(TIMERIRQ_RATE);                     // Set up period in microseconds
    timer.setChannel1Mode(TIMER_OUTPUT_COMPARE);        // Set up an interrupt on channel 1
    timer.setCompare(TIMER_CH1, 1);                     // Interrupt 1 count after each update
    timer.attachCompare1Interrupt(interrupt_handler);   // Attach handler
    timer.refresh();                                    // Refresh the timer's count, prescale, and overflow
    timer.resume();                                     // Start the timer counting
}

//---------------
void interrupt_handler(void) {
#ifdef ENABLE_WWDG_COUNTER_CLEAR
  wwdg_counter = 0;
#endif

#ifdef DEBUG_TIMERIRQ_INDICATOR
  digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN)^1); // Blink
#endif
}
