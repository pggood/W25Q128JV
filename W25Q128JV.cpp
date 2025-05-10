#include "W25Q128JV.h"

W25Q128JV::W25Q128JV(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) 
    : _hspi(hspi), _cs_port(cs_port), _cs_pin(cs_pin) {
}

void W25Q128JV::init() {
    // Initialize CS pin
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    
    // Reset the device
    reset();
    
    // Read manufacturer and device ID to verify communication
    uint8_t cmd = 0x90; // Read Manufacturer/Device ID
    uint8_t dummy = 0x00;
    uint8_t id[2] = {0};
    
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(_hspi, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(_hspi, &dummy, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(_hspi, &dummy, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(_hspi, id, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    
    // Check if we got the expected IDs (0xEF for Winbond, 0x4018 for W25Q128JV)
    if (id[0] != 0xEF || id[1] != 0x40) {
        Error_Handler();
    }
}

void W25Q128JV::readData(uint32_t address, uint8_t *data, uint32_t length) {
    uint8_t cmd[4] = {0x03, // Read Data command
                     (uint8_t)(address >> 16),
                     (uint8_t)(address >> 8),
                     (uint8_t)address};
    
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(_hspi, cmd, 4, HAL_MAX_DELAY);
    HAL_SPI_Receive(_hspi, data, length, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
}

void W25Q128JV::writeEnable() {
    uint8_t cmd = 0x06; // Write Enable command
    
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(_hspi, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
}

uint8_t W25Q128JV::readStatusReg1() {
    uint8_t cmd = 0x05; // Read Status Register 1 command
    uint8_t status;
    
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(_hspi, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(_hspi, &status, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    
    return status;
}

uint8_t W25Q128JV::readStatusReg2() {
    uint8_t cmd[2] = {0x35, 0x00}; // Read Status Register 2 command
    
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(_hspi, cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(_hspi, cmd+1, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    
    return cmd[1];
}

uint8_t W25Q128JV::readStatusReg3() {
    uint8_t cmd[2] = {0x15, 0x00}; // Read Status Register 3 command
    
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(_hspi, cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(_hspi, cmd+1, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    
    return cmd[1];
}

void W25Q128JV::waitForWriteComplete() {
    while (readStatusReg1() & 0x01); // Wait while BUSY bit is set
}

void W25Q128JV::writeData(uint32_t address, uint8_t *data, uint32_t length) {
    uint8_t cmd[4];
    uint32_t chunkSize;
    uint32_t offset = 0;
    
    while (length > 0) {
        writeEnable();
        
        chunkSize = (length > 256) ? 256 : length;
        
        cmd[0] = 0x02; // Page Program command
        cmd[1] = (uint8_t)((address + offset) >> 16);
        cmd[2] = (uint8_t)((address + offset) >> 8);
        cmd[3] = (uint8_t)(address + offset);
        
        HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
        HAL_SPI_Transmit(_hspi, cmd, 4, HAL_MAX_DELAY);
        HAL_SPI_Transmit(_hspi, data + offset, chunkSize, HAL_MAX_DELAY);
        HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
        
        waitForWriteComplete();
        
        offset += chunkSize;
        length -= chunkSize;
    }
}

void W25Q128JV::eraseSector(uint32_t sector) {
    uint32_t address = sector * 4096; // 4KB sectors
    uint8_t cmd[4] = {0x20, // Sector Erase command
                     (uint8_t)(address >> 16),
                     (uint8_t)(address >> 8),
                     (uint8_t)address};
    
    writeEnable();
    
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(_hspi, cmd, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    
    waitForWriteComplete();
}

void W25Q128JV::eraseChip() {
    uint8_t cmd = 0xC7; // Chip Erase command
    
    writeEnable();
    
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(_hspi, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    
    waitForWriteComplete();
}

void W25Q128JV::sleep() {
    uint8_t cmd = 0xB9; // Deep Power Down command
    sendCommand(cmd);
}

void W25Q128JV::wakeup() {
    uint8_t cmd = 0xAB; // Release from Deep Power Down command
    sendCommand(cmd);
    HAL_Delay(3); // Wait for wakeup time
}

void W25Q128JV::reset() {
    uint8_t reset_enable = 0x66;
    uint8_t reset = 0x99;
    
    sendCommand(reset_enable);
    sendCommand(reset);
    HAL_Delay(1); // Wait for reset to complete
}

void W25Q128JV::sendCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(_hspi, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
}

void W25Q128JV::sendCommandWithAddress(uint8_t cmd, uint32_t address) {
    uint8_t buf[4] = {cmd,
                     (uint8_t)(address >> 16),
                     (uint8_t)(address >> 8),
                     (uint8_t)address};
    
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(_hspi, buf, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
}
