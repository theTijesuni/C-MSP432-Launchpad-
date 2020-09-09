#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <stdlib.h>
Graphics_Context g_sContext;

volatile uint32_t flag = 0;
//volatile intwait = 0;
volatile int8_t flagUp = 0;
volatile int8_t flagDown = 0;

void LCD_init(){
    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    /* Initializes graphics context */
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(&g_sContext);

}

void SysTick_Handler(void){
    flag = 1;
}
void PORT5_IRQHandler(void){
    GPIO_clearInterruptFlag(GPIO_PORT_P5, GPIO_PIN1);//reset flag to enable future interrupts
//    wait = 1; //depends on implementation
    flagUp = 1; //used in main to check interrupt took place
}

void PORT3_IRQHandler(void){
    GPIO_clearInterruptFlag(GPIO_PORT_P3, GPIO_PIN5);//reset flag to enable future interrupts
//    wait = 1; //depends on implementation
    flagDown = 1; //used in main to check interrupt took place
}

void erase(int32_t x, int32_t y, int32_t radius){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    //paint dot white to "erase"
    Graphics_fillCircle(&g_sContext, x, y, radius);
    //set dot radius
    Graphics_setForegroundColor
    (&g_sContext, GRAPHICS_COLOR_RED);
    //set dot color again
}

void draw (int32_t x, int32_t y, int32_t radius){
    Graphics_fillCircle(&g_sContext, x, y, radius);
    //paint red dot
}

void erase_paddle( int32_t px, int32_t py, int32_t paddle_length) {
    // draw over old paddle with background color to erase it
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawLineV(&g_sContext, px, py, py + paddle_length - 1);
    // restore the foreground color setting
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
}

void draw_paddle(int32_t px, int32_t py, int32_t paddle_length) {
    Graphics_drawLineV(&g_sContext, px, py, py + paddle_length - 1);
}

int main(void){
    int32_t px = 3, paddle_length = 32;
    int32_t pvy = 16;
    int32_t py = 48;
    uint32_t x=65, y =70 ;
    int32_t vx=1, vy = 3;
    const int32_t radius = 3;
    uint32_t bounce = 0;
    int32_t period = 100000;

    WDT_A_hold(WDT_A_BASE);
    GPIO_setAsOutputPin (GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 |GPIO_PIN2 );


    LCD_init();
//    SysTick initialization
    SysTick_setPeriod(period);
    SysTick-> VAL = 0;
    SysTick_enableModule();
    SysTick_enableInterrupt();

//    Port 5 init / Top
    /* Setup button as an input */
    GPIO_setAsInputPin(GPIO_PORT_P5, GPIO_PIN1);
    /* Clear any existing interrupts, housekeeping */
    GPIO_clearInterruptFlag(GPIO_PORT_P5, GPIO_PIN1);
    /* Set interrupt to trigger on falling edge of input */
    GPIO_interruptEdgeSelect(GPIO_PORT_P5, GPIO_PIN1,GPIO_HIGH_TO_LOW_TRANSITION);
    /* Enable interrupt on port X, pin Y */
    GPIO_enableInterrupt(GPIO_PORT_P5, GPIO_PIN1);
    Interrupt_enableInterrupt(INT_PORT5);
    /* Enable Interrupts globally for the entire chip */
    Interrupt_enableMaster();

//    Port 3 init / Bottom Button
    GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN5);
    /* Clear any existing interrupts, housekeeping */
    GPIO_clearInterruptFlag(GPIO_PORT_P3, GPIO_PIN5);
    /* Set interrupt to trigger on falling edge of input */
    GPIO_interruptEdgeSelect(GPIO_PORT_P3, GPIO_PIN5,GPIO_HIGH_TO_LOW_TRANSITION);
    /* Enable interrupt on port X, pin Y */
    GPIO_enableInterrupt(GPIO_PORT_P3, GPIO_PIN5);
    Interrupt_enableInterrupt(INT_PORT3);
    /* Enable Interrupts globally for the entire chip */
    Interrupt_enableMaster();

    while(1){
        draw(x,y,radius);
        if(flag == 1){
            flag = 0;
            if (x > 128 - radius){
                vx = -vx;
            }
            if (y > 128 - radius){
                vy = -vy;
            }
            erase(x,y,radius);
            x = x + vx ;
            y = y + vy ;
            draw(x,y,radius);
            draw_paddle(px,py,paddle_length);

            if(flagUp == 1){
                //check if up button was pushed and if paddle is not in the top position
                //reset flag
                //erase previous paddle
                 erase_paddle(px,py,paddle_length);
                 flagUp = 0;
                 if(py - pvy > 0){ //check if there is room to move paddle up half length
                     py -= pvy;
                 }
                 else{
                     py = 1; //since there is no room, move paddle
                 }
                 draw_paddle(px,py,paddle_length);
                 //draw paddle at new position, paddle y-coord changed at up button ISR
             }

            if(flagDown == 1){ //check if down button was pushed and if there is room to move paddle down
                flagDown = 0;
                erase_paddle(px,py,paddle_length);
                if(py + paddle_length + pvy < 127){ //check if there is room to move paddle down
                    py += pvy ; //change sign of interrupt decrement to move paddle down
                }
                else{ //
                    py = 126 - paddle_length ; //since there is no room, keep paddle in place
                }
                draw_paddle(px,py,paddle_length);
                //draw paddle at new position
             }
        }
//        if(){//check if next dot x-position is in contact with paddle and dot y-position is below paddle top and bottom edges
//        //change x-direction

        if(x == 3 + radius){
            if( py <= y && y <= py + paddle_length){
                //        //increment bounce counter
                GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1 );
                GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN2);
                bounce = bounce + 1;
                erase(x,y,radius);
                vx = -vx;
                x = x + vx;
                draw(x,y,radius);

                ////        if(**){ //check SystTick period
                ////        **//decrease SysTick period to accelerate dot
                ////        SysTick_setPeriod(period);
                //        }
                //        }
                if(period > 50000){
                    period = period - 10000;
                    SysTick_setPeriod(period);
                }
            }else{
            }
        }else{
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1 | GPIO_PIN0 | GPIO_PIN2 );

            if (x < 3 + radius){
               GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);

               erase(x,y,radius);
               erase_paddle(px,py,paddle_length);

               break;

            }
        }
    }

    //// necessary to use the sprintf() function.
    char print_string[20] = "";
    // string to hold text
    /* Prints ‘Hello World!’ centered in the screen */

    Graphics_drawStringCentered(
        &g_sContext,// global variable
        (int8_t *) "Game Over!",// Text to output, can be a variable.
        AUTO_STRING_LENGTH,
        64,// x-position to center around
        30,// y-position to center around
        OPAQUE_TEXT
        );
    // construct output string
        sprintf(print_string, "Score = %d", bounce);
    /* Prints x = 1 on the left of the screen */

    Graphics_drawString(
        &g_sContext,// global variable
        (int8_t *) print_string, // Text to output, can be a variable.
        AUTO_STRING_LENGTH,
        5,// x-position, top-left of string
        40, // y-position, top-left of string
        OPAQUE_TEXT
        );

}
