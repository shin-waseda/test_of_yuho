#include "main.h"

extern SPI_HandleTypeDef hspi2;

void ICM_Write(uint8_t reg, uint8_t data) {
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
    uint8_t buf[2] = {reg & 0x7F, data};
    HAL_SPI_Transmit(&hspi2, buf, 2, 10);
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

uint8_t ICM_Read(uint8_t reg) {
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
    uint8_t tx = reg | 0x80;
    uint8_t rx = 0;
    HAL_SPI_Transmit(&hspi2, &tx, 1, 10);
    HAL_SPI_Receive(&hspi2, &rx, 1, 10);
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
    return rx;
}

