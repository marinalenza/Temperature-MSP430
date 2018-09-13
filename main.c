/* Temperature Sensor MSP430
 * using internal sensor of the experiment board
 * temperature >= 25 -> turn on red LED
 * temperature < 25 -> turn off green LED*/

#include <msp430.h> 
#include <intrinsics.h>

volatile int adcBuffer;
volatile float temp, temp2;

// Calibration Constants
const unsigned int *ADC_Temperatura30 = (unsigned int *)0x1A1A;
const unsigned int *ADC_Temperatura85 = (unsigned int *)0x1A1C;

#define red_LED   BIT0 //Red LED P1.0
#define grn_LED   BIT7 //Green LED P4.7

#define calibration 0 //temperature offset

enum _convDone{
    IDLE = 0,
    CONVDONE
} convDone;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    //LEDs as outputs
    P1DIR |= red_LED;
    P4DIR |= grn_LED;
    //LEDs begin low
    P1OUT = 0;
    P4OUT = 0;


    //Configuração do Clock
    UCSCTL0 = 0x0000;           // Set lowest possible DCOx, MODx
                                // These are controlled by FLL
    UCSCTL1 = DCORSEL_5;        // Select DCO range to 16MHz
    UCSCTL2 |= 243;             // DCOCLK = 2* (N+1) * 32.768kHz = 16MHz
    UCSCTL3 = SELREF_0;         // Set DCO FLL reference = XT1
    UCSCTL4 = SELA_0 | SELS_3 | SELM_3 ;    // Set ACLK = XT1,
                                            // SMCLK = DCOCLK, MCLK = DCOCLK
    UCSCTL5 = DIVPA_0 | DIVA_0 | DIVS_4 | DIVM_0;   // Set SMCLK, divide by 16
    __delay_cycles(250000);     // Wait for oscillator to stabilize

    // Configure Voltage Reference
    REFCTL0 = REFMSTR | REFON;  // 1,5V voltage reference.

    // Configure ADC

    ADC12CTL0 &= ~ADC12ENC;
    ADC12CTL0 |= ADC12SHT0_8 | ADC12ON;
    ADC12CTL1 |= ADC12SSEL_1 | ADC12SHP;    // ADC Clock = ACLK = 32768Hz
    ADC12CTL2 |= ADC12RES_2;    // 12 bit resolution
    ADC12MCTL0 = ADC12EOS | ADC12SREF_1 | ADC12INCH_10; // Vref-0V, temperature sensor
    ADC12IE = ADC12IE0;         // Enable interupts

    const float delta = 55.0f/(*ADC_Temperatura85 - *ADC_Temperatura30);
    const float cte   = 30.0f - (delta * (*ADC_Temperatura30));



    while(1){
        // Start conversion
        ADC12CTL0 &= ~ADC12SC;
        ADC12CTL0 |= ADC12ENC | ADC12SC;
        __low_power_mode_0();

        // Calculate temperature
        temp = delta * (float)adcBuffer + cte - calibration;


        if(temp >= 25){
            P1OUT |= red_LED;    // red LED on, green LED off
            P4OUT &= ~grn_LED;
        }
        else{
            P1OUT &= ~red_LED;  // green LED on, red LED off
            P4OUT |= grn_LED;
        }

        __delay_cycles(16000000); //wait one second

    }
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void){
    adcBuffer = ADC12MEM0;
    convDone = CONVDONE;
    __low_power_mode_off_on_exit();
}


