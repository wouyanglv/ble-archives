#ifndef APP_CONFIG_H__
#define APP_CONFIG_H__


#include <stdint.h>



/*
 * DEV0 = EEG
 * DEV1 = EMG
 */
#define NUM_MAX30003_DEVS          2


#ifdef NRF52_DK

    /* SPI */
    #define MAX30003_SPI_MOSI_PIN     27
    #define MAX30003_SPI_MISO_PIN     29
    #define MAX30003_SPI_SCK_PIN      26

    #define MAX30003_SPI_DEV0_SS_PIN  28
    #define MAX30003_SPI_DEV1_SS_PIN  30

    /* INTB */
    #define MAX30003_SPI_DEV0_INTB_PIN  25
    #define MAX30003_SPI_DEV1_INTB_PIN  31

#else

    /* SPI */
    #define MAX30003_SPI_MOSI_PIN     6
    #define MAX30003_SPI_MISO_PIN     5
    #define MAX30003_SPI_SCK_PIN      8

    #define MAX30003_SPI_DEV0_SS_PIN  15
    #define MAX30003_SPI_DEV1_SS_PIN  14

    /* INTB */
    #define MAX30003_SPI_DEV0_INTB_PIN  3
    #define MAX30003_SPI_DEV1_INTB_PIN  12

#endif






#endif // APP_CONFIG_H__