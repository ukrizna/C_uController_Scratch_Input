#include <inttypes.h>

#include <avr/io.h>

#include <avr/interrupt.h>

#include <stdio.h> 

#include "uart.h"

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, 

_FDEV_SETUP_RW); 

#define t1 2

#define t2 1500

#define t2Circle 3000

#define t3 750

#define t4 12000

#define AVG_LEN 200

#define MOVING_LEN 60

#define THRESHOLD 400

#define PAST_LEN 6

#define LOG_LEN 900 

volatile char time1;

volatile int time2, time3, time4;

int mic[AVG_LEN + 1], moving = 0, avg = 0, upper = 0, lower = 0,

peakWidth[PAST_LEN], valleyWidth[PAST_LEN], peakSlope[PAST_LEN],

peakHeight[PAST_LEN], peakCount = 0, index = 0, begin = 0, isUpper = 0,

signalWidth = 0, tPeakWidth = 0, tValleyWidth = 0, tSignalWidth = 0,

tPeakSlope = 0, k = 0;

char circleMode = 0, tapCircle = 0; 

ISR (TIMER0_COMPA_vect) {

    if (time1 > 0) time1--;

    if (time2 > 0) time2--;

    if (time3 > 0) time3--;

    if (time4 > 0) time4--;

} 



void capture() {

// Captures the ADC value and calculates two different moving averages.

// Moving is a moving average of the past 60 data points. Moving simply

// smooths out the sometimes erratic signal from the analog filter.

// Avg is a moving average of the past 200 data points. Avg is used to

// calculate the hysteresis values upper and lower. 

    mic[index] = ADCH;

    ADCSRA |= (1 << ADSC); 

    if (index >= MOVING_LEN)

        moving = moving + (mic[index] * 10 / MOVING_LEN) - (mic[index - MOVING_LEN] * 10 / MOVING_LEN);

    else

        moving = moving + (mic[index] * 10 / MOVING_LEN) - (mic[AVG_LEN + 1 - 


MOVING_LEN + index] * 10 / MOVING_LEN); 


    if (index == AVG_LEN)

        avg = avg + (mic[AVG_LEN] * 10 / AVG_LEN) - (mic[0] * 10 / AVG_LEN);

    else

        avg = avg + (mic[index] * 10 / AVG_LEN) - (mic[index + 1] * 10 / AVG_LEN); 
        

upper = 12 * avg / 10;

lower = 8 * avg / 10;


} 

void extract() {

// Extracts features from the incoming signal, such as peak heights, peak

// slope, peak width, valley width, signal width, and peak count. This is 

// done without holding any past data points. Past data points are only

// kept for moving averages.

//

// Peaks count is amplitude independent. This is done by hysteresis of

// moving. Whenever moving is greater than upper, a peak is counted only if

// moving has been smaller than lower previously isUpper is used to keep

// the hysteresis state. 

    if (moving > THRESHOLD) {

        if (circleMode) time2 = t2Circle;

        else time2 = t2; 

        if (!isUpper && moving > upper) {

           isUpper = 1;

           if (peakCount == 0) tSignalWidth = 0;

           else valleyWidth[(peakCount - 1) % PAST_LEN] = tValleyWidth;

           peakCount++;

           tPeakWidth = 0;

           tPeakSlope = 0;

       }

    }


    if (isUpper) {

        if (peakHeight[(peakCount - 1) % PAST_LEN] < moving) {

            peakHeight[(peakCount - 1) % PAST_LEN] = moving;

            peakSlope[(peakCount - 1) % PAST_LEN] = tPeakSlope;

        } 

        if (moving < lower) {

            isUpper = 0;

            tValleyWidth = 0;

            peakWidth[(peakCount - 1) % PAST_LEN] = tPeakWidth;

        }

    }

} 

void selector() {

// Attempts to map the attributes extracted from the signal to a gesture. 

    int tempWDiff, tempHDiff, antiTap = 0;

    signalWidth = tSignalWidth; 

// Often times when starting a gesture, the initial finger placement creates

// an unwanted tap with RC decay. The following anti-tap filters out most

// unwanted intial taps.

    if (peakCount > 1 && peakWidth[0] < 200 && peakWidth[1] > 185) {

         antiTap = 1;

         peakCount--;

    } 

    if (peakCount == 1) {

         if (peakWidth[0 + antiTap] < 200) {

         tapCircle = 1;

         time4 = t4;

     }

     else if (peakWidth[0 + antiTap] > 230 && peakWidth[0 + antiTap] < 1000)

          fprintf(stdout, "1\n");

      } 

     else if (peakCount == 2) {

         tempHDiff = peakHeight[0] - peakHeight[1];

        if (tempHDiff < 0) 
            tempHDiff = -tempHDiff; 

        if (tempHDiff < peakHeight[0] / 10 && peakWidth[0] < 200 && peakWidth[1] < 185)

            fprintf(stdout, "D\n");

        else if (peakWidth[0 + antiTap] > 220 && peakWidth[1 + antiTap] < 300 && valleyWidth[0 + antiTap] > 250 && peakSlope[1 + antiTap] < 300)

            fprintf(stdout, "2\n");

        else if (peakWidth[0 + antiTap] > 220 && peakWidth[1 + antiTap] > 240 && 

            valleyWidth[0 + antiTap] > 250)

            fprintf(stdout, "3\n");

    } 

    else if (peakCount == 3) {

        tempHDiff = peakHeight[0 + antiTap] - peakHeight[1 + antiTap];

        if (tempHDiff < 0) 
            tempHDiff = -tempHDiff; 

        if (tempHDiff < peakHeight[0] / 10 && peakWidth[0] < 200 && peakWidth[1] < 185 && peakWidth[2] < 185)

            fprintf(stdout, "X\n");

        else if (tempHDiff > (2 * peakHeight[0 + antiTap] / 5) && peakWidth[0 + antiTap] > 220 && peakWidth[2 + antiTap] > 240) {

            tempHDiff = peakHeight[2 + antiTap] - peakHeight[1 + antiTap];

        if (tempHDiff < 0) tempHDiff = -tempHDiff; 

        if (tempHDiff > (2 * peakHeight[2 + antiTap] / 5))

            fprintf(stdout, "3\n");

        }

    }

} 

void reset() {

// Resets the all signal characteristic variables for the next

// gesture input. 

    for (int i = 0; i < PAST_LEN; i++) {

        peakHeight[i] = 0;

        peakWidth[i] = 0;

        peakSlope[i] = 0;

        valleyWidth[i] = 0;

    } 

    circleMode = 0;

    peakCount = 0;

    isUpper = 0; 

} 


int main() {

// Resets variables for the initial gesture.

    int rPeakCount = 0;
    
    for (int i = 0; i < AVG_LEN; i++) mic[i] = 0;

        time1 = 0;

        reset(); 

// Starts Timer0 and Timer0 interrupts for task scheduling.

        TCCR0A = (1 << WGM01);

        TCCR0B = 3;

        OCR0A = 60;

        TIMSK0 = (1 << OCIE0A); 

        // Readies the ADC.

        ADMUX = (1 << REFS1) | (1 << REFS0) | (1<<ADLAR);

        ADCSRA = ((1 << ADEN) | (1 << ADSC)) + 7; 

        // Readies the UART.

        uart_init();

        stdout = stdin = stderr = &uart_str;

        fprintf(stdout, "UART Initialized\n"); 

        sei(); 

    while (1) {
    
    // Time1 executes approximately every 0.5 ms. Time1 captures the ADC

    // signal and extracts characteristics from the signal.

        if (time1 == 0) {

            capture();

             extract(); 

            if (tPeakWidth < 10000) tPeakWidth++;

            if (tSignalWidth < 10000) tSignalWidth++;

            if (tValleyWidth < 10000) tValleyWidth++;
    
            if (tPeakSlope < 10000) tPeakSlope++;

            index++;

            index = index % (AVG_LEN + 1);

            time1 = t1;

         } 

    // Time4 executes approximately 2.88 s after a circle or tap is

    // detected. Time4 is a timeout counter for tap circle. Time4 is only

    // reset when a circle gesture is detected or when there is a

    // singular tap.

        if (time4 == 0 && tapCircle == 1)

            tapCircle = 0; 

    // Time3 executes approximately every 180 ms. Time3 looks for circle

    // gestures. Circle gestures are detected in real time rather than after

    // the gesture is finished.

        if (time3 == 0 && peakCount > 3) {

            if (peakWidth[(peakCount - 1) % PAST_LEN] > 240) {

                circleMode = 1;

                time4 = t4; 

                if (rPeakCount < peakCount) {

                    if (tapCircle) fprintf(stdout, "C");

                    else fprintf(stdout, "W");

                }

            } 

            rPeakCount = peakCount;

            time3 = t3;

        } 

// Time2 executes approximately 360 ms after a gesture is done. Time2

// is a timeout counter for all gestures except for circles. After

// a gesture falls below the threshold value, the software waits 360 ms

// to make sure the gesture is finished and not just temporarily below

// the threshold.

        if (time2 == 0 && peakCount > 0 && peakWidth[0] > 0) {

            selector();

            reset();
        
        }

    }

}
