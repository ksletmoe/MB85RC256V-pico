# MB85RC256V FRAM Driver for Raspberry Pi Pico

A C++ driver library for the **MB85RC256V** 256Kb FRAM (Ferroelectric Random Access Memory) device, specifically designed for the Raspberry Pi Pico using the Pico SDK.

FRAM is a type of non-volatile memory that combines the best characteristics of both RAM and traditional non-volatile storage like EEPROM or Flash.

### Key Characteristics:

- **Non-volatile** - Data persists without power, like EEPROM/Flash
- **Fast writes** - No write delays required (unlike EEPROM which needs ~5ms per write)
- **High endurance** - Virtually unlimited write cycles (10^14 vs ~100,000 for EEPROM)
- **Instant availability** - Data is immediately available after write operations
- **Low power** - Very efficient power consumption

## Hardware Requirements

- Raspberry Pi Pico, Pico W, Pico 2, or Pico 2 W
- MB85RC256V FRAM breakout board or chip
- Pull-up resistors for I2C lines (if not using a breakout board that already includes them; typically 4.7kΩ)

## Known Compatible MB85RC256V Breakout Boards

- [Adafruit I2C Non-Volatile FRAM Breakout - 256Kbit / 32KByte](https://www.adafruit.com/product/1895?srsltid=AfmBOoqG-FudoLtrgJe3XMj_-YABf4_md2Ko17dxmfprnyuTW5hswxYd)
- [HiLetgo MB85RC256V FRAM Breakout Board](https://www.amazon.com/dp/B0CF4L3XVF?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_1)

## Wiring

| MB85RC256V Pin | Pico Pin | Description |
|----------------|----------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| SDA | GPIO 4 (I2C0 SDA) | I2C Data line |
| SCL | GPIO 5 (I2C0 CLK) | I2C Clock line |
| WP | GND or 3.3V | Write protect (connect to GND for normal operation) |

*Note: You can use any GPIO pins configured for I2C. The example above uses I2C0 with default pins.*

## Installation

### Method 1: Git Submodule (Recommended)

Add this library as a git submodule to your Raspberry Pi Pico project:

```bash
# Navigate to your project directory
cd your-pico-project

# Add as a submodule
git submodule add https://github.com/ksletmoe/MB85RC256V-pico.git lib/MB85RC256V-pico

# Initialize and update the submodule
git submodule update --init --recursive
```

### Method 2: Direct Clone

```bash
# Clone into your project's lib directory
git clone https://github.com/ksletmoe/MB85RC256V-pico.git lib/MB85RC256V-pico
```

## CMake Integration

Add the following to your project's `CMakeLists.txt`:

```cmake
add_subdirectory(lib/MB85RC256V-pico/MB85RC256V-pico build)

target_link_libraries(your_project_name
        MB85RC256V-pico)
```

## Usage Example

Here's a complete example showing how to use the FRAM library:

```cpp
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "FRAM.h"

using namespace MB85RC256V_PICO;

int main() {
    // Initialize stdio
    stdio_init_all();

    // Initialize I2C0 at 400kHz
    i2c_init(i2c0, 400000);
    gpio_set_function(4, GPIO_FUNC_I2C);  // SDA
    gpio_set_function(5, GPIO_FUNC_I2C);  // SCL
    gpio_pull_up(4);
    gpio_pull_up(5);

    // Create FRAM instance
    FRAM fram(*i2c0);  // Uses default address 0x50

    // Check if device is connected
    if (!fram.isConnected()) {
        printf("FRAM device not found!\n");
        return -1;
    }

    printf("FRAM device connected successfully!\n");

    // Write some data
    const char* message = "Hello, FRAM!";
    if (fram.writeData(0x0000, (uint8_t*)message, strlen(message))) {
        printf("Data written successfully\n");
    }

    // Read the data back
    char readBuffer[32];
    if (fram.readData(0x0000, (uint8_t*)readBuffer, strlen(message))) {
        readBuffer[strlen(message)] = '\0';  // Null terminate
        printf("Data read: %s\n", readBuffer);
    }

    // Get device ID
    uint16_t manufacturerId, productId;
    fram.getDeviceId(&manufacturerId, &productId);
    printf("Manufacturer ID: 0x%03X, Product ID: 0x%03X\n",
           manufacturerId, productId);

    // Clear a portion of memory
    if (fram.writeData(0x1000, (uint8_t*)"Test data for clearing", 22)) {
        printf("Test data written at 0x1000\n");

        // Clear the entire FRAM (optional - this erases everything!)
        // fram.clear(0x00);
    }
}
```

## API Reference

### Constructor

```cpp
explicit FRAM(i2c_inst_t i2c_port, uint8_t address = MB85RC256V_DEFAULT_ADDRESS);
```

### Methods

#### `bool isConnected()`
Checks if the FRAM device is connected and responds with the correct device ID.

#### `void getDeviceId(uint16_t* manufacturerId, uint16_t* productId)`
Retrieves the manufacturer and product IDs from the device.
- Expected manufacturer ID: `0x00A` (Fujitsu)
- Expected product ID: `0x510` (MB85RC256V)

#### `bool readData(uint16_t memAddress, uint8_t* buffer, size_t length)`
Reads data from FRAM memory.
- `memAddress`: Starting address (0x0000 to 0x7FFF)
- `buffer`: Buffer to store read data
- `length`: Number of bytes to read
- Returns: `true` on success, `false` on error

#### `bool writeData(uint16_t memAddress, const uint8_t* data, size_t length)`
Writes data to FRAM memory.
- `memAddress`: Starting address (0x0000 to 0x7FFF)
- `data`: Data buffer to write
- `length`: Number of bytes to write
- Returns: `true` on success, `false` on error

#### `bool clear(uint8_t value = 0x00)`
Clears the entire FRAM memory with the specified value.
- `value`: Byte value to fill memory with (default: 0x00)
- Returns: `true` on success, `false` on error
- ⚠️ **Warning**: This erases ALL data in the FRAM

## Memory Layout

The MB85RC256V provides:
- **Total memory**: 32,768 bytes (32KB)
- **Address range**: 0x0000 to 0x7FFF
- **Organization**: 32K × 8 bits
- **I2C address**: 0x50 (default, configurable via hardware pins)

## Debug Output

Enable debug output by defining `MB85RC256V_FRAM_ENABLE_PRINTF` in your build:

```cmake
target_compile_definitions(your_project_name PRIVATE MB85RC256V_FRAM_ENABLE_PRINTF)
```

## Troubleshooting

### Device Not Found
- Check wiring connections
- Verify I2C pull-up resistors are present
- Ensure power supply is stable (3.3V)
- Verify I2C pins are correctly configured

### Write/Read Failures
- Check memory address bounds (0x0000 to 0x7FFF)
- Ensure I2C bus speed is appropriate (try 100kHz if 400kHz fails)
- Verify device isn't write-protected (WP pin)

### Build Errors
- Ensure Pico SDK is properly installed and `PICO_SDK_PATH` is set
- Verify CMakeLists.txt includes all required libraries
- Check that the submodule is properly initialized

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Resources

- [MB85RC256V datasheet](https://www.fujitsu.com/uk/Images/MB85RC256V-20171207.pdf)
