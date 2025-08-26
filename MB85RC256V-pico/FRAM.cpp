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

/**
 * @file FRAM.cpp
 * @brief Implementation of the MB85RC256V FRAM driver library
 *
 * This file contains the implementation of all methods for communicating
 * with the MB85RC256V FRAM device over I2C on the Raspberry Pi Pico.
 *
 * @author Kyle Sletmoe
 * @version 1.0
 * @date 2025
 */

#include <stdio.h>
#include <string.h>

#include "FRAM.h"
#include "pico/time.h"

/** Maximum buffer size for chunked write operations to prevent I2C buffer overflow */
#define MAX_BUFFER_SIZE 256

namespace MB85RC256V_PICO {

/**
 * @brief Constructor implementation
 *
 * Initializes the FRAM object with the specified I2C port and device address.
 * No actual I2C communication occurs in the constructor - call isConnected()
 * to verify device presence and functionality.
 *
 * @param i2c_port I2C port instance (i2c0 or i2c1)
 * @param address I2C address of the FRAM device
 */
FRAM::FRAM(i2c_inst_t i2c_port, uint8_t address) : _i2c_port(i2c_port), _address(address) {
    // Constructor only stores parameters - no I2C communication performed
}

/**
 * @brief Check if FRAM device is connected and responsive
 *
 * This function verifies the FRAM device presence by reading its device ID
 * and comparing it against expected values for the MB85RC256V chip.
 *
 * Expected values:
 * - Manufacturer ID: 0x00A (Fujitsu)
 * - Product ID: 0x510 (MB85RC256V)
 *
 * @return true if device is present and responds with correct ID, false otherwise
 *
 * @note Includes a 1-second delay to allow device power-up stabilization
 * @note Debug output is conditionally compiled based on MB85RC256V_FRAM_ENABLE_PRINTF
 */
bool FRAM::isConnected() {
#ifdef MB85RC256V_FRAM_ENABLE_PRINTF
    printf(
        "Initializing MB85RC256V device at address 0x%02X on I2C port %p\n", _address, _i2c_port);
#endif

    // Allow time for device power-up and stabilization
    sleep_ms(1000);

    uint16_t manufacturer_id = 0;
    uint16_t product_id = 0;

    // Read device identification
    getDeviceId(&manufacturer_id, &product_id);

    // Verify this is an MB85RC256V device
    if (manufacturer_id != 0x00A || product_id != 0x510) {
#ifdef MB85RC256V_FRAM_ENABLE_PRINTF
        printf("Failed to initialize MB85RC256V device: Invalid ID (Manuf: 0x%03X, Prod: "
               "0x%03X)\n",
               manufacturer_id,
               product_id);
#endif
        return false;
    }

    return true;
}

/**
 * @brief Read device identification information from FRAM
 *
 * This function reads the device ID register to retrieve manufacturer
 * and product identification codes. The ID register is accessed using
 * a special I2C address (0x7C) and magic value.
 *
 * The 3-byte response contains:
 * - Byte 0: Manufacturer ID upper 8 bits
 * - Byte 1: Manufacturer ID lower 4 bits + Product ID upper 4 bits
 * - Byte 2: Product ID lower 8 bits
 *
 * @param manufacturerId Pointer to store 12-bit manufacturer ID (expected: 0x00A)
 * @param productId Pointer to store 12-bit product ID (expected: 0x510)
 *
 * @note Uses dedicated device ID I2C address (0x7C), not the memory access address
 */
void FRAM::getDeviceId(uint16_t* manufacturerId, uint16_t* productId) {
    // Validate input pointers
    if (!manufacturerId || !productId) {
        return;
    }

    uint8_t reg = 0x50 << 1;     // Magic value to read device ID
    uint8_t readBuffer[3] = {0}; // 3-byte device ID response, initialized to zero

    // Write register address, then read 3 bytes of device ID
    int writeResult = i2c_write_blocking(&_i2c_port, MB85RC256V_DEVICE_ID, &reg, 1, true);
    if (writeResult != 1) {
        // I2C write failed, return zero values
        *manufacturerId = 0;
        *productId = 0;
        return;
    }

    int readResult = i2c_read_blocking(&_i2c_port, MB85RC256V_DEVICE_ID, readBuffer, 3, false);
    if (readResult != 3) {
        // I2C read failed, return zero values
        *manufacturerId = 0;
        *productId = 0;
        return;
    }

    // Parse manufacturer ID from bytes 0 and 1 (12 bits total)
    *manufacturerId = (readBuffer[0] << 4) + (readBuffer[1] >> 4);

    // Parse product ID from bytes 1 and 2 (12 bits total)
    *productId = ((readBuffer[1] & 0x0F) << 8) + readBuffer[2];
}

/**
 * @brief Convert 16-bit memory address to byte array for I2C transmission
 *
 * The MB85RC256V requires memory addresses to be sent as two bytes in
 * big-endian format (MSB first) for I2C memory access operations.
 *
 * @param memAddress 16-bit memory address (0x0000 to 0x7FFF)
 * @param addrBuffer 2-byte buffer to store address bytes [MSB, LSB]
 *
 * @note This is a helper function used internally by read/write operations
 */
void FRAM::setAddressBytes(uint16_t memAddress, uint8_t* addrBuffer) {
    // Validate input pointer
    if (!addrBuffer) {
        return;
    }

    addrBuffer[0] = (memAddress >> 8) & 0xFF; // High byte (MSB)
    addrBuffer[1] = memAddress & 0xFF;        // Low byte (LSB)
}

/**
 * @brief Read data from FRAM memory
 *
 * Performs a sequential read operation from the specified memory address.
 * The function first sends the memory address, then reads the requested
 * number of bytes from the FRAM device.
 *
 * @param memAddress Starting memory address (0x0000 to 0x7FFF)
 * @param buffer Pointer to buffer where read data will be stored
 * @param length Number of bytes to read
 * @return true if read operation successful, false if bounds check fails
 *
 * @note Validates that read operation stays within memory boundaries
 * @note Uses I2C repeated start condition for address write followed by data read
 */
bool FRAM::readData(uint16_t memAddress, uint8_t* buffer, size_t length) {
    // Validate input parameters
    if (!buffer || length == 0) {
        return false;
    }

    // Validate memory bounds with overflow protection
    if (length > MB85RC256V_MEM_SIZE || memAddress > MB85RC256V_MEM_END ||
        (memAddress + length - 1) > MB85RC256V_MEM_END) {
#ifdef MB85RC256V_FRAM_ENABLE_PRINTF
        printf("Read operation exceeds memory bounds\n");
#endif
        return false;
    }

    uint8_t addrBuffer[2];
    setAddressBytes(memAddress, addrBuffer);

    // Send memory address with repeated start, then read data
    int writeResult = i2c_write_blocking(&_i2c_port, _address, addrBuffer, 2, true);
    if (writeResult != 2) {
        return false;
    }

    int readResult = i2c_read_blocking(&_i2c_port, _address, buffer, length, false);
    return readResult == (int)length;
}

/**
 * @brief Write data to FRAM memory
 *
 * Performs a sequential write operation to the specified memory address.
 * Large writes are automatically chunked to prevent I2C buffer overflow.
 * Each chunk contains the memory address followed by the data bytes.
 *
 * @param memAddress Starting memory address (0x0000 to 0x7FFF)
 * @param data Pointer to data buffer to write
 * @param length Number of bytes to write
 * @return true if write operation successful, false if bounds check fails
 *
 * @note Validates that write operation stays within memory boundaries
 * @note Automatically chunks large writes into MAX_BUFFER_SIZE segments
 * @note No write delays needed - FRAM is immediately ready after write
 */
bool FRAM::writeData(uint16_t memAddress, const uint8_t* data, size_t length) {
    // Validate input parameters
    if (!data || length == 0) {
        return false;
    }

    // Validate memory bounds with overflow protection
    if (length > MB85RC256V_MEM_SIZE || memAddress > MB85RC256V_MEM_END ||
        (memAddress + length - 1) > MB85RC256V_MEM_END) {
#ifdef MB85RC256V_FRAM_ENABLE_PRINTF
        printf("Write operation exceeds memory bounds\n");
#endif
        return false;
    }

    size_t bytesWritten = 0;
    while (bytesWritten < length) {
        // Calculate chunk size to prevent buffer overflow
        size_t chunkSize =
            (length - bytesWritten > MAX_BUFFER_SIZE) ? MAX_BUFFER_SIZE : (length - bytesWritten);

        // Prepare write buffer: [address_high, address_low, data...]
        uint8_t writeBuffer[MAX_BUFFER_SIZE + 2]; // 2 bytes for address + data
        setAddressBytes(memAddress + bytesWritten, writeBuffer);
        memcpy(&writeBuffer[2], &data[bytesWritten], chunkSize);

        // Send address and data in single I2C transaction
        int writeResult =
            i2c_write_blocking(&_i2c_port, _address, writeBuffer, chunkSize + 2, false);
        if (writeResult != (int)(chunkSize + 2)) {
            // Write failed, return false
            return false;
        }

        bytesWritten += chunkSize;
    }

    return true;
}

/**
 * @brief Clear entire FRAM memory with specified value
 *
 * This function fills the entire 32KB FRAM memory space with a single
 * byte value. It uses chunked writes to avoid stack overflow issues
 * and writes the value across the entire memory range.
 *
 * @param value Byte value to fill memory with (default: 0x00)
 * @return true if clear operation successful, false otherwise
 *
 * @warning This operation will erase ALL existing data in the FRAM
 * @note Uses small buffer chunks to prevent stack overflow
 * @note Operation may take some time due to 32KB of data transfer
 */
bool FRAM::clear(uint8_t value) {
    // Use a reasonably sized buffer to avoid stack overflow
    uint8_t clearBuffer[MAX_BUFFER_SIZE];
    memset(clearBuffer, value, MAX_BUFFER_SIZE);

    // Write in chunks across the entire FRAM memory
    uint16_t address = 0;
    while (address < MB85RC256V_MEM_SIZE) {
        size_t remainingBytes = MB85RC256V_MEM_SIZE - address;
        size_t chunkSize = (remainingBytes < MAX_BUFFER_SIZE) ? remainingBytes : MAX_BUFFER_SIZE;

        if (!writeData(address, clearBuffer, chunkSize)) {
            return false;
        }

        // Safe increment with overflow check
        if (address > MB85RC256V_MEM_SIZE - chunkSize) {
            break; // Prevent overflow
        }
        address += chunkSize;
    }

    return true;
}
} // namespace MB85RC256V_PICO
