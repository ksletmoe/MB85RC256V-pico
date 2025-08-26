/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2025, Kyle Sletmoe
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FRAM.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#define I2C_BAUDRATE 400000 // 400kHz
#define I2C_PORT i2c0

// using GPIO 4 and 5 for I2C -- note GPIO pin numbers != physical pin numbers.
#define I2C_SDA 4
#define I2C_SCL 5

void i2cInit() {
    printf("Initializing I2C on port %p, SDA pin %d and SLC pin %d\n", I2C_PORT, I2C_SDA, I2C_SCL);

    i2c_init(I2C_PORT, I2C_BAUDRATE); // Initialize I2C at 400kHz

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

int main() {
    stdio_init_all();

    i2cInit();

    MB85RC256V_PICO::FRAM fram(*I2C_PORT);

    if (!fram.isConnected()) {
        while (true) {
            printf("FRAM not detected. Please check connections.\n");
            sleep_ms(2000);
        }
    }

    uint8_t readByte;
    fram.readData(0x0000, &readByte, 1);

    uint8_t writeByte = readByte + 1;
    fram.writeData(0x0000, &writeByte, 1);

    // you can reset the board with a debug probe and verify that the old value is retained and
    // then updated with the new value. on the next reboot, the new value should be retained.
    while (true) {
        printf("Read byte: 0x%02X, Wrote byte: 0x%02X\n", readByte, writeByte);
        sleep_ms(2000);
    }
}
