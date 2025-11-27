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
- FAULT support must be prepared as develop environment of Arduino IDE 1.8.8 before.
- For check, prepare several type of FAULT cause.
- For check diference of behavior, there is need of execute on several type of CPU.
