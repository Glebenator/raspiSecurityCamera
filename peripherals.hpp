#ifndef PERIPHERALS
#define PERIPHERALS

#include <fstream>
#include <unistd.h>


using namespace std;

#define BUZZER_GPIO_DIRECTION "/sys/class/gpio/gpio17/direction"
#define BUZZER_GPIO_VALUE "/sys/class/gpio/gpio17/value"
#define RED_LED_GPIO_DIRECTION "/sys/class/gpio/gpio27/direction"
#define RED_LED_GPIO_VALUE "/sys/class/gpio/gpio27/value"
#define BUTTON_GPIO_VALUE "/sys/class/gpio/gpio22/value"


void setGpioValue(const char *gpioFile, const char *value);
void redOff();
void redOn();
void flashingRed(int ns);
void buzzerOn();
void buzzerOff();
void flashingBuzzer(int ns);
void exportGpioPin(const string &gpioPin);
void exportAllGpioPin();
void openFiles();
int returnButtonValue();

#endif // PERIPHERALS
