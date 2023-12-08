

#include "peripherals.hpp"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <time.h>


using namespace std;

enum ButtonState {
    Idle,
    ButtonPressed,
    Flashing
};

void setGpioValue(const char *gpioFile, const char *value) {
    FILE *gpioValueFile = fopen(gpioFile, "w");
    if (!gpioValueFile) {
        fprintf(stderr, "Unable to open %s\n", gpioFile);
        return;
    }

    fprintf(gpioValueFile, "%s", value);
    fclose(gpioValueFile);
}

void redOff() {
    setGpioValue(RED_LED_GPIO_VALUE, "0");
}

void redOn() {
    setGpioValue(RED_LED_GPIO_VALUE, "1");
}
void flashingRed(int ns){
    redOn();
    usleep(ns);
    redOff();
    usleep(ns);
}



void buzzerOn() {
    setGpioValue(BUZZER_GPIO_VALUE, "1");
}

void buzzerOff() {
    setGpioValue(BUZZER_GPIO_VALUE, "0");
}

void flashingBuzzer(int ns){
    for(int count = 0; count < 50; count++){
        buzzerOn();
        redOn();
        usleep(ns);
        buzzerOff();
        redOff();
        usleep(ns);
    }
    return;
}

void exportGpioPin(const string& gpioPin) {
    ofstream exportFile("/sys/class/gpio/export");
    if (!exportFile.is_open()) {
        exit(0);
    }
    exportFile << gpioPin;
    exportFile.close();
    sleep(1);
}
void exportAllGpioPin(){
    exportGpioPin("17"); //buzzer
    exportGpioPin("27"); //red led
    exportGpioPin("22"); //button
}

void openFiles(){
    // Open the direction files for writing
    FILE* buzzerDirectionFile = fopen(BUZZER_GPIO_DIRECTION, "w");
    FILE* buzzerValueFile = fopen(BUZZER_GPIO_VALUE, "w");
    FILE* redLedDirectionFile = fopen(RED_LED_GPIO_DIRECTION, "w");
    FILE* redLedValueFile = fopen(RED_LED_GPIO_VALUE, "w");
    FILE* buttonDirectionFile = fopen(BUTTON_GPIO_VALUE, "r");
    

    if (buzzerDirectionFile == NULL){
        printf("ERROR OPENING %s.", BUZZER_GPIO_DIRECTION);
        exit(0);
    }
    fprintf(buzzerDirectionFile, "out");
    fclose(buzzerDirectionFile);

    if (buzzerValueFile == NULL){
    printf("ERROR OPENING %s.", BUZZER_GPIO_VALUE);
    exit(0);
    }
    fprintf(buzzerValueFile, "out");
    fclose(buzzerValueFile);

    if (redLedDirectionFile == NULL){
        printf("ERROR OPENING %s.", RED_LED_GPIO_DIRECTION);
        exit(0);
    }
    fprintf(redLedDirectionFile, "out");
    fclose (redLedDirectionFile);
    
    if (redLedValueFile == NULL){
        printf("ERROR OPENING %s.", RED_LED_GPIO_VALUE);
        exit(0);
    }
    fprintf(redLedValueFile, "0");
    fclose (redLedValueFile);

    if (buttonDirectionFile == NULL){
        printf("ERROR OPENING %s.", BUTTON_GPIO_VALUE);
        exit(0);
    }
    return;
}

int returnButtonValue(){
    int value;
    FILE* buttonDirectionFile = fopen(BUTTON_GPIO_VALUE, "r");
    fscanf(buttonDirectionFile, "%d", &value);
    fclose(buttonDirectionFile);
    return value;
}




// int main() {
//     exportAllGpioPin();
//     openFiles();

//     ButtonState buttonState = Idle;
//     bool buttonPressedAgain = false;
//     cout << "starting! " << endl;
//     while (true) {
//         int buttonValue = returnButtonValue();

//         switch (buttonState) {
//             case Idle:
//                 if (buttonValue == 1) {
//                     buttonState = ButtonPressed;
//                     buttonPressedAgain = false;
//                 }
//                 break;

//             case ButtonPressed:
//                 if (buttonValue == 0) {
//                     buttonState = Flashing;
//                 }
//                 break;

//             case Flashing:
//                 // Flash LEDs and buzzer
//                 flashingRed(50000);
//                 flashingBuzzer(50000);

//                 // Check if the button is pressed again to stop flashing
//                 if (buttonValue == 1) {
//                     buttonPressedAgain = true;
//                 }

//                 // Check if the button is released to go back to Idle
//                 if (buttonValue == 0 && buttonPressedAgain) {
//                     buttonState = Idle;
//                 }
//                 break;
//         }

//         // Sleep for a short duration to control the rate of checking
//         usleep(5000);
//     }

//     return 0;
// }


