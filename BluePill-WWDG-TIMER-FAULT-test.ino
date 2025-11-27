#define LED_BUILTIN PC13

#define ENABLE_WWDG_COUNTER_CLEAR   // 1: for inhibit WWDG reset occur. when need to check, comment out.
//#define DEBUG_WWDGIRQ_INDICATOR     // 2: for check WWDIRQ callback by BUILTIN_LED fast blink
//#define DEBUG_TIMERIRQ_INDICATOR    // 3: for check TIMERIRQ callback by BUILTIN_LED slow blink
#define DEBUG_FORCE_FAULT           // 4: for force cause FAULT, enable one of below
#define DEBUG_BUS_ERROR             //   a) force cause BUS error
//#define DEBUG_ADR_ERROR             //   b) force cause ADDRESS error
//#define DEBUG_ZERO_DEVIDE           //   c) Coretex-M3 can't execute but Coretex-M4 can.
//#define DEBUG_UNDEFINED_OPECODE     //   d) force cause UNDEFINED OPERATION code error

#include "libmaple/wwdg.h"

#define WWDG_NEW_COUNTER    127       // min 65(0x41) to max 127(0x7F) 
//#define DEBUG_CHK_WWDG_RESET        // Display message if WWDG_RESET occur or not at boot time. 
                                      // if comment out, display all caues.
int wwdg_counter;
//---------------
void WWDG_IRQHandler(void) {
  wwdg_counter++;  
  if (wwdg_counter < 10) {
    wwdg_counter_reload(WWDG_NEW_COUNTER); 
    wwdg_ewi_flag_clear();
  }
#ifdef DEBUG_WWDGIRQ_INDICATOR
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN)^1); // Blink
#endif
}

//---------------
void setup() {
    pinMode(LED_BUILTIN,OUTPUT);
    Serial.begin(9600);
    while (!Serial) { delay(100); }   // For ensure display message, wait until detect (USB) Serial connection.
                                      // At stand-alone use with no connection, it might be not desireable.
    Serial.println("BluePill-WWDG-TIMER-FAULT-test");

#ifdef DEBUG_CHK_WWDG_RESET
    if (check_wwdg_reset_occur()) {   // only check WWDGRSTF flag at start-up
        Serial.println("*** Recover from WWDG RESET ***");
    }
#else
    // Check all cause of RESET
int cause = get_all_reset_cause();    // cause = RCC->CSR >> 26; 
    if (cause &  1) { Serial.println("*** PIN  RESET occur ***"); }
    if (cause &  2) { Serial.println("*** POR  RESET occur ***"); }
    if (cause &  4) { Serial.println("*** SFT  RESET occur ***"); }
    if (cause &  8) { Serial.println("*** IWDG RESET occur ***"); }
    if (cause & 16) { Serial.println("*** WWDG RESET occur ***"); }
    if (cause & 32) { Serial.println("*** LPWR RESET occur ***"); }
#endif

    setup_interrupt_check();                // for TimerIRQ

    // WWDG setup 
    wwdg_attach_callback(WWDG_IRQHandler);  // attach interrupt service function
    wwdg_init(127, 127, 3);                 // wwdg window=127, counter=127, devider=2^3=8. this parameter setting means max 57.3mS period
    nvic_irq_set_priority(NVIC_WWDG, 2);    // set priority 2. (*)NVIC_WWDG is defined at "nvic.h"
    nvic_irq_enable(NVIC_WWDG);             // interrupt enable. this function is defined as 'static inline' at "nvic.h"
    wwdg_ewi_flag_clear();                  // pre-clear WWDG EWI flag
    wwdg_ewi_enable();                      // set WWDG EWI interrupt enable flag
    wwdg_counter = 0;

//-----------------------------
#ifdef  DEBUG_FORCE_FAULT
uint32_t adr, offset, data;

 #ifdef DEBUG_BUS_ERROR
    // Bus Error check
    for (uint8_t i=0; i<16; i++) {
        delay(100);
        adr = i * 0x10000000;
        Serial.print("Address = 0x"); Serial.print(adr,HEX);
        data = *(__IO uint32_t *) adr;
        Serial.print(" : Data = 0x"); Serial.println(data,HEX);
    }
 #endif

 #ifdef DEBUG_ADR_ERROR
    // Address Error check
    for (uint8_t i=0; i<16; i++) {
        delay(100);
        adr = i * 0x10000000 + 1;
        Serial.print("Address = 0x"); Serial.print(adr,HEX);
        data = *(__IO uint32_t *) adr;
        Serial.print(" : Data = 0x"); Serial.println(data,HEX);
    }
 #endif

 #ifdef DEBUG_ZERO_DEVIDE
    Serial.println("*** STM32F103 can't execute 'Zero Devide'.");
    //asm volatile ("sdiv r0,r1,r2"); // only for Coretex-M4, Coretex-M3 do not have this instruction
    //asm volatile ("udiv r0.r1.r2"); // only for Coretex-M4, Coretex-M3 do not have this instruction
 #endif

 #ifdef DEBUG_UNDEFINED_OPECODE
    asm volatile (".byte 0xFF, 0xFF, 0xFF, 0xFF");
 #endif

#endif
}

//--------------- 
void loop() {
    Serial.print(".");
    delay(500);
}
