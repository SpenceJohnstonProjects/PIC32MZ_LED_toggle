//Project: LED interrupt toggle
//Description: blinking light periodically using interrupts
//Author: Spence Johnston
//Date: 10/1/2022

// PIC32MZ2048ECG144, EFM144 or or EFG144 based HMZ144 board Configuration Bit Settings
// DEVCFG2
#if defined(__32MZ2048EFG144__) || defined(__32MZ2048EFM144__)
#pragma config FPLLIDIV = DIV_4         // System PLL Input Divider (4x Divider) for 24MHz clock (Rev C1 board w EFG) 24MHz/4 = 6MHz
                                        // also 24MHz clock rev C board w EFM (weird - went back to C. rev D also is EFM but with Osc)
#pragma config UPLLFSEL = FREQ_24MHZ    // USB PLL Input Frequency Selection (USB PLL input is 24 MHz)
#else
#pragma config FPLLIDIV = DIV_2         // System PLL Input Divider (2x Divider) for 12 MHz crystal (Rev B and C boards w ECG) 12MHz/2 = 6MHz
#pragma config UPLLEN = OFF             // USB PLL Enable (USB PLL is disabled)
#endif
#pragma config FPLLRNG = RANGE_5_10_MHZ // System PLL Input Range (5-10 MHz Input)
#pragma config FPLLICLK = PLL_POSC      // System PLL Input Clock Selection (POSC is input to the System PLL)
#pragma config FPLLMULT = MUL_112       // System PLL Multiplier (PLL Multiply by 112) 6MHz * 112 = 672MHz
#pragma config FPLLODIV = DIV_8         // System PLL Output Clock Divider (8x Divider) 672MHz / 2 = 84MHz

// DEVCFG1
#pragma config FNOSC = SPLL             // Oscillator Selection Bits (Primary Osc (HS,EC))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disable SOSC)
#if defined(__32MZ2048EFG144__)
#pragma config POSCMOD = EC             // Primary Oscillator Configuration EC - External clock osc
                                        // Rev C1 board w EFG uses an Oscillator (Rev D boards too))
#else
#pragma config POSCMOD = HS             // Primary Oscillator Configuration HS - Crystal osc
                                        // Rev B and C (w ECG or EFM) use Crystals
#endif
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disabled, FSCM Disabled)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled)
#pragma config FDMTEN = OFF             // Deadman Timer Enable (Deadman Timer is disabled)
#pragma config DMTINTV = WIN_127_128    // Default DMT Count Window Interval (Window/Interval value is 127/128 counter value)
#pragma config DMTCNT = DMT31           // Max Deadman Timer count = 2^31

// DEVCFG0
#pragma config JTAGEN = OFF             // JTAG Enable (JTAG Disabled)
#pragma config ICESEL = ICS_PGx2        // ICD/ICE is on PGEC2/PGED2 pins (not default)


#include <xc.h>

//interrupt handlers

//multi-vector mode. Not using shadow registers
void __attribute__ ((at_vector(_TIMER_2_VECTOR), interrupt(IPL4SOFT), nomips16)) timer2_i_handler()
{
    IFS0CLR = _IFS0_T2IF_MASK; //atomically clear interrupt flag
    LATHINV = _LATH_LATH2_MASK; //toggle LED
}

//if using single vector mode. need to configure main for this.
/*
void __attribute__ ((at_vector(0), interrupt(RIPL), nomips16)) timer2_i_handler()
{    
    if(_IFS0_T2IF_MASK)
    {
        IFS0CLR = _IFS0_T2IF_MASK; //atomically clear interrupt flag
        LATHINV = _LATH_LATH2_MASK;//toggle LED
    }
}
*/


int main() {
    //configure output at H2
    TRISH &= ~_LATH_LATH2_MASK;
    
    //configure waitstate and prefen (predictive prefetch)
    PRECON = (1 & _PRECON_PFMWS_MASK) | ((2 << _PRECON_PREFEN_POSITION) & _PRECON_PREFEN_MASK);
    
    
    //setup PB clock 3 to 21MHz, ie div by 4
    //do this before interrupts are enabled
    SYSKEY = 0; // Ensure lock
    SYSKEY = 0xAA996655; // Write Key 1
    SYSKEY = 0x556699AA; // Write Key 2
    PB3DIV = _PB3DIV_ON_MASK | 3 & _PB3DIV_PBDIV_MASK; // 0 = div by 1, 1 = div by 2, 2 = div by 3 etc up to 128
    SYSKEY = 0; // Re lock
    
    //configure clock
    // Set T2CON for a prescale value of 5 (1:32) along with setting the T32 bit
    // configuring it for 32 bit mode all other bits including TON are 0 (so it will also turn T2 off)
    //      prescale 1:32                                            32 bit mode                              
    T2CON = ((7 << _T2CON_TCKPS_POSITION) & _T2CON_TCKPS_MASK) & ~_T2CON_T32_MASK;
    TMR2 = 0x0; //clear timer register
    PR2 = 0xFF; //load period register
    IEC0bits.T2IE = 0; //disable timer2 interrupts
    IFS0bits.T2IF = 0; // clear interrupt flag for t2
    INTCONbits.MVEC = 1; //configure multi-vectored interrupts    
    IPC2bits.T2IP = 4;//t2 interrupt priority 4
    IEC0bits.T2IE = 1;//enable timer2 interrupt
    T2CONSET = _T2CON_ON_MASK; //start timer2
    
    asm volatile("ei"); //globally enable interrupts
    
    while ( 1 )//infinite loop so the program never stops.
    {}
    
     return 0;
}