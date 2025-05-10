#ifndef W25Q128JV_H
#define W25Q128JV_H

#include "stm32f1xx_hal.h"
#include <cstdint>
#include <cstring>

class W25Q128JV {
public:
    // Constructor
    W25Q128JV(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
    
    // Initialization
    void init();
    
    // Basic operations
    void readData(uint32_t address, uint8_t *data, uint32_t length);
    void writeData(uint32_t address, uint8_t *data, uint32_t length);
    void eraseSector(uint32_t sector);
    void eraseChip();
    
    // Status operations
    uint8_t readStatusReg1();
    uint8_t readStatusReg2();
    uint8_t readStatusReg3();
    
    // Advanced features
    void sleep();
    void wakeup();
    void reset();
    
private:
    SPI_HandleTypeDef *_hspi;
    GPIO_TypeDef *_cs_port;
    uint16_t _cs_pin;
    
    void writeEnable();
    void waitForWriteComplete();
    void sendCommand(uint8_t cmd);
    void sendCommandWithAddress(uint8_t cmd, uint32_t address);
};

#endif // W25Q128JV_H
