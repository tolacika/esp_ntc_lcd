#ifndef LCD_H
#define LCD_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

// I2C configuration
#define I2C_MASTER_NUM        I2C_NUM_0
#define I2C_MASTER_SDA_IO     14
#define I2C_MASTER_SCL_IO     15
#define I2C_MASTER_FREQ_HZ    100000
#define LCD_I2C_ADDRESS       0x27    // Adjust to your PCF8574 address

// LCD commands
#define WRITE_BIT           I2C_MASTER_WRITE

#define LCD_BACKLIGHT (1 << 3) // Backlight bit
#define LCD_ENABLE (1 << 2)   // Enable bit
#define LCD_ENABLE_OFF (0 << 2) // Enable off
#define LCD_RW (1 << 1)      // Read/Write bit
#define LCD_RW_WRITE (0 << 1) // Write mode
#define LCD_RW_READ (1 << 1)  // Read mode
#define LCD_RS (1 << 0)      // Register Select bit
#define LCD_RS_CMD (0 << 0)    // Command mode
#define LCD_RS_DATA (1 << 0)   // Data mode
#define LCD_DB7 (1 << 7) // Data bit 7
#define LCD_DB6 (1 << 6) // Data bit 6
#define LCD_DB5 (1 << 5) // Data bit 5
#define LCD_DB4 (1 << 4) // Data bit 4

#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_ROW_OFFSET {0x00, 0x40, 0x14, 0x54} // Row offsets for 20x4 LCD
#define LCD_BUFFER_SIZE (LCD_COLS * LCD_ROWS)

/**
 * @brief Initialize the I2C master.
 */
void i2c_initialize(void);

/**
 * @brief Initialize the LCD.
 */
void lcd_initialize(void);

/**
 * @brief Set the cursor position on the LCD.
 * @param col Column index (0-based).
 * @param row Row index (0-based).
 */
void lcd_set_cursor_position(uint8_t col, uint8_t row);

/**
 * @brief Update the cursor position in the buffer.
 * @param col Column index (0-based).
 * @param row Row index (0-based).
 */
void lcd_set_cursor(uint8_t col, uint8_t row);

/**
 * @brief Write a single character to the buffer.
 * @param c Character to write.
 */
void lcd_write_character(char c);

/**
 * @brief Write a string to the buffer.
 * @param str Null-terminated string to write.
 */
void lcd_write_text(const char *str);

/**
 * @brief Copy data to the LCD buffer at a specific position.
 * @param data Data to copy.
 * @param size Size of the data.
 * @param col Column index (0-based).
 * @param row Row index (0-based).
 */
void lcd_copy_to_buffer(const char *data, size_t size, int8_t col, int8_t row);

/**
 * @brief Clear the LCD buffer.
 */
void lcd_clear_buffer(void);

/**
 * @brief Render the buffer content to the LCD.
 */
void lcd_render(void);

/**
 * @brief Control the LCD backlight.
 * @param state True to turn on, false to turn off.
 */
void lcd_toggle_backlight(bool state);

/**
 * @brief Format temperature as a string.
 * @param temp Temperature value.
 * @param buffer Buffer to store the formatted string.
 * @param buffer_size Size of the buffer.
 */
void lcd_format_temperature(float temp, char *buffer, size_t buffer_size);

/**
 * @brief Display an array of temperatures on the LCD.
 * @param temps Array of temperature values.
 * @param size Number of temperatures.
 */
void lcd_display_temperatures(float *temps, size_t size);

/**
 * @brief Periodically update the LCD with temperature data.
 * @param pvParameter Task parameter (unused).
 */
void lcd_update_task(void *pvParameter);

#endif // LCD_H