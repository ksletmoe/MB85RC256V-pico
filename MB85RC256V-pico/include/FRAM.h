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
 * @file FRAM.h
 * @brief Driver library for MB85RC256V FRAM device on Raspberry Pi Pico
 *
 * This header file provides a C++ interface for communicating with the
 * MB85RC256V 256Kb FRAM (Ferroelectric Random Access Memory) device
 * over I2C protocol using the Raspberry Pi Pico SDK.
 *
 * The MB85RC256V is a non-volatile memory device that combines the
 * benefits of RAM (fast write speeds, high write endurance) with
 * non-volatile storage capabilities.
 *
 * @author Kyle Sletmoe
 * @version 1.0
 * @date 2025
 */

#pragma once

#include "hardware/i2c.h"

/** Default I2C address for MB85RC256V device */
#define MB85RC256V_DEFAULT_ADDRESS 0x50

/** Device ID for MB85RC256V FRAM */
#define MB85RC256V_DEVICE_ID 0x7C

/** Starting memory address of the FRAM */
#define MB85RC256V_MEM_START 0x00

/** Ending memory address of the FRAM (32KB - 1) */
#define MB85RC256V_MEM_END 0x7FFF

/** Total memory size of the FRAM in bytes (32KB) */
#define MB85RC256V_MEM_SIZE 0x8000

namespace MB85RC256V_PICO {

/**
 * @class FRAM
 * @brief Driver class for MB85RC256V FRAM device
 *
 * This class provides methods to communicate with the MB85RC256V FRAM device
 * over I2C, including reading, writing, and device identification functions.
 *
 * Key features:
 * - 256Kb (32KB) of non-volatile memory
 * - I2C interface communication
 * - High-speed write operations (no write delays)
 * - Virtually unlimited write endurance (10^14 write cycles)
 * - Data retention of 10 years at 85°C
 */
class FRAM {
private:
    /** I2C address of the FRAM device */
    uint8_t _address;

    /** I2C port instance (i2c0 or i2c1) */
    i2c_inst_t _i2c_port;

    /**
     * @brief Helper function to set memory address bytes for I2C communication
     * @param memAddress 16-bit memory address to convert
     * @param addrBuffer Buffer to store the address bytes (MSB first)
     */
    void setAddressBytes(uint16_t memAddress, uint8_t* addrBuffer);

public:
    /**
     * @brief Constructor for FRAM class
     * @param i2c_port I2C port instance (i2c0 or i2c1)
     * @param address I2C address of the device (default: 0x50)
     *
     * @note The I2C port must be initialized before creating a FRAM instance
     * @note Typical I2C speeds: 100kHz (standard), 400kHz (fast), 1MHz (fast+)
     */
    explicit FRAM(i2c_inst_t i2c_port, uint8_t address = MB85RC256V_DEFAULT_ADDRESS);

    /**
     * @brief Destructor (default)
     */
    ~FRAM() = default;

    /**
     * @brief Copy constructor (deleted)
     * @note Copying FRAM objects is not allowed to prevent I2C port conflicts
     */
    FRAM(const FRAM&) = delete;

    /**
     * @brief Copy assignment operator (deleted)
     * @note Copying FRAM objects is not allowed to prevent I2C port conflicts
     */
    FRAM& operator=(const FRAM&) = delete;

    /**
     * @brief Check if the FRAM device is connected and responding
     * @return true if device responds to I2C communication, false otherwise
     *
     * This function performs a simple I2C communication test to verify
     * the device is present and responding on the specified address.
     */
    bool isConnected();

    /**
     * @brief Get device identification information
     * @param manufacturerId Pointer to store manufacturer ID (Fujitsu: 0x00A)
     * @param productId Pointer to store product ID (MB85RC256V: 0x510)
     *
     * Reads the device ID register to verify the connected device is
     * indeed an MB85RC256V FRAM. This can be used for device validation.
     */
    void getDeviceId(uint16_t* manufacturerId, uint16_t* productId);

    /**
     * @brief Read data from FRAM memory
     * @param memAddress Starting memory address (0x0000 to 0x7FFF)
     * @param buffer Pointer to buffer where read data will be stored
     * @param length Number of bytes to read
     * @return true if read operation successful, false otherwise
     *
     * @note Reading beyond the memory boundary will wrap around to address 0x0000
     * @note Maximum single read operation depends on I2C buffer limitations
     */
    bool readData(uint16_t memAddress, uint8_t* buffer, size_t length);

    /**
     * @brief Write data to FRAM memory
     * @param memAddress Starting memory address (0x0000 to 0x7FFF)
     * @param data Pointer to data buffer to write
     * @param length Number of bytes to write
     * @return true if write operation successful, false otherwise
     *
     * @note Writing beyond the memory boundary will wrap around to address 0x0000
     * @note FRAM has no write delays - data is immediately available after write
     * @note Maximum single write operation depends on I2C buffer limitations
     */
    bool writeData(uint16_t memAddress, const uint8_t* data, size_t length);

    /**
     * @brief Clear entire FRAM memory with specified value
     * @param value Byte value to fill memory with (default: 0x00)
     * @return true if clear operation successful, false otherwise
     *
     * This function writes the specified value to all 32KB of FRAM memory,
     * effectively clearing or initializing the entire device.
     *
     * @warning This operation will erase all existing data in the FRAM
     */
    bool clear(uint8_t value = 0x00);
};

} // namespace MB85RC256V_PICO
