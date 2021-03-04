/******************************************************************************

  DucatiPanigaleCanBus: monitor CANBUS messages and report them to HLT
  More info at https://github.com/renatobo/DucatiPanigaleCanBus
  Renato Bonomini https://github.com/renatobo

******************************************************************************/

// Define PINS used for interfaces here

/* ------- Status LED --------- */
// Define LED_STATUS to the pin used to show status of the system:
// - 1 sec frequency on/off: regular activity
// - fading light: CAN connection error, waiting to retry
// Standard configuration: we flip status of LED_BUILTIN to show the main loop is active
// You can decide to have the 
#ifdef ARDUINO_LOLIN_D32_PRO
#define LED_STATUS GPIO_NUM_5
#else
#define LED_STATUS GPIO_NUM_2
#endif

/* ------- CANBUS transceiver RX and TX pins --------- */
// You can use any PIN not otherwise available, there is no dedicated TWAI set of pins 
// cfr https://arduino.stackexchange.com/questions/81209/what-are-the-correct-pins-for-twai-can-on-esp32
// Override custom PINS from command line if needed via -DTX_GPIO_NUM <yourpin> and -DRX_GPIO_NUM <yourpin>
#ifndef TX_GPIO_NUM
#define TX_GPIO_NUM GPIO_NUM_21
#endif
#ifndef RX_GPIO_NUM
#define RX_GPIO_NUM GPIO_NUM_22
#endif

/* end of file */