# Arduino_STM32 WWDG+TIMER+FAULT combination for Auto_Reboot
We want to execute Auto Reset/Reboot when Fault occur for keep system work.\
This is one of solution for such needs.

## 1. Working Mechanism
- WWDG process always work at it's period.
- TIMER process always work and clear WWDG counter within expired time of WWDG.
- If TIMER interrupt process inhibited by FAULT or other cause, WWDG counter would expire and cause reset.
- Even if TIMER interrupt process not inhibited, all thread process stop except BUILTIN_LED featured blink. 
- Then, user would recognize something wrong and did reset manualy.

## 2. Sample Program
- We use WWDG and TIMER interrupt in user program.
- FAULT support must be prepared at develop environment of Arduino IDE 1.8.8 before.
- For check, prepare several type of FAULT cause.
- For check diference of behavior, there is need of execute on several type of CPU.

### `BluePill-WWDG-TIMER-FAULT-test.ino`
```
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
```
### `Interrupt.ino`
```
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
```
## 3. Execution Check
The result is not so good.
- I had been thinking Fault would mask all interrupt.
- But timer interrupt still work after Fault occur.
- Then checked `util.c` and found cause as below.
```
/* (Called from exc.S with global interrupts disabled.) */
__attribute__((noreturn)) void __error(void) {
    if (__lm_error) {
        __lm_error();
    }
    /* Reenable global interrupts */
    nvic_globalirq_enable();                                          <<<<<
	throb();
}
```
- The line re-enable globalirq which disabled previously in `exc.S` as below.
```
__default_exc:
    ldr r2, NVIC_CCR            @ Enable returning to thread mode even if there are
    mov r1 ,#1                  @ pending exceptions. See flag NONEBASETHRDENA.
    str r1, [r2]
    cpsid i                     @ Disable global interrupts           <<<<<
    ldr r2, SYSTICK_CSR         @ Disable systick handler
    mov r1, #0
    str r1, [r2]
    ldr r1, CPSR_MASK           @ Set default CPSR
    push {r1}
    ldr r1, TARGET_PC           @ Set target pc
    push {r1}
    sub sp, sp, #24             @ Don't care
    ldr r1, EXC_RETURN          @ Return to thread mode
    mov lr, r1
    bx lr                       @ Exception exit

.align 4
CPSR_MASK:     .word 0x61000000
EXC_RETURN:    .word 0xFFFFFFF9
TARGET_PC:     .word __error
NVIC_CCR:      .word 0xE000ED14    @ NVIC configuration control register
SYSTICK_CSR:   .word 0xE000E010    @ Systick control register
```
## 4. Solution
- Most simple way is comment out `nvic_globalirq_enable()`.
- But it is dangerous against such as operation for FAULT regsiters store to BACKUP RAM.
- Because, WWDG RESET occur timing is not solid and unknown.
- So, we had better to execute `nvic_globalirq_disenable()` after such as backup complete in `util.c`.
- `nvic_globalirq_disenable()` is defined as same as `nvic_globalirq_enable()` in "libmaple/nvic.h".
## 5. Execution Check II and Result
- Did above solution and checked.
- On BluePill(STM32F103C8T6,20k RAM,64k Flash), not cause WWDG RESET, work Timer IRQ usual.
- On STM32F103RET6(64k RAM, 512k Flash), cause WWDG RESET.
- There left need of future research and consideration about chip internal structure/circuit difference against globalirq of BluePill.
