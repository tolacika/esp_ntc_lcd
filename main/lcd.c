#include "lcd.h"
#include "ntc_adc.h"
#include <string.h>

static const char *TAG = "I2C_LCD";
static i2c_master_dev_handle_t i2c_device_handle = NULL;
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static uint8_t lcd_backlight_status = LCD_BACKLIGHT;

static char lcd_buffer[LCD_BUFFER_SIZE]; // 80-byte buffer for the LCD
static uint8_t cursor_col = 0;
static uint8_t cursor_row = 0;

static esp_err_t i2c_send_with_toggle(uint8_t data) {
    // Helper function to toggle the enable bit
    uint8_t data_with_enable = data | LCD_ENABLE;
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_device_handle, &data_with_enable, 1, -1));
    vTaskDelay(pdMS_TO_TICKS(1));

    data_with_enable &= ~LCD_ENABLE;
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_device_handle, &data_with_enable, 1, -1));
    vTaskDelay(pdMS_TO_TICKS(1));

    return ESP_OK;
}

static esp_err_t i2c_send_4bit_data(uint8_t data, uint8_t rs) {
    // Send a byte of data to the LCD in 4-bit mode
    uint8_t nibbles[2] = {
        (data & 0xF0) | rs | lcd_backlight_status | LCD_RW_WRITE,
        ((data << 4) & 0xF0) | rs | lcd_backlight_status | LCD_RW_WRITE
    };
    ESP_ERROR_CHECK(i2c_send_with_toggle(nibbles[0]));
    ESP_ERROR_CHECK(i2c_send_with_toggle(nibbles[1]));

    return ESP_OK;
}

void i2c_initialize(void) {
    // Initialize the I2C master
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));
    ESP_LOGI(TAG, "I2C bus initialized");

    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_I2C_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &i2c_device_config, &i2c_device_handle));
    ESP_LOGI(TAG, "I2C device added");
    vTaskDelay(pdMS_TO_TICKS(50));  // Wait for LCD to power up
    ESP_LOGI(TAG, "I2C device initialized");
}

void lcd_initialize(void) {
    // Initialize the LCD
    uint8_t init_8bit_commands[] = {
        lcd_backlight_status | LCD_ENABLE_OFF | LCD_RW_WRITE | LCD_RS_CMD,
        0b00110000 | lcd_backlight_status | LCD_RW_WRITE | LCD_RS_CMD,
        0b00110000 | lcd_backlight_status | LCD_RW_WRITE | LCD_RS_CMD,
        0b00110000 | lcd_backlight_status | LCD_RW_WRITE | LCD_RS_CMD,
        0b00100000 | lcd_backlight_status | LCD_RW_WRITE | LCD_RS_CMD,
    };

    uint8_t init_commands[] = {
        0b00101000, // Function set: 4-bit mode, 2 lines, 5x8 dots
        0b00001100, // Display control: display on, cursor off, blink off
        0b00000001, // Clear display
        0b00000110, // Entry mode set: increment cursor, no shift
        0b00000010, // Set cursor to home position
        0b10000000  // Set cursor to first line
    };

    for (uint8_t i = 0; i < sizeof(init_8bit_commands); i++) {
        ESP_ERROR_CHECK(i2c_send_with_toggle(init_8bit_commands[i]));
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    for (uint8_t i = 0; i < sizeof(init_commands); i++) {
        ESP_ERROR_CHECK(i2c_send_4bit_data(init_commands[i], LCD_RS_CMD));
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void lcd_set_cursor_position(uint8_t col, uint8_t row) {
    // Set the cursor position on the LCD
    if (col >= LCD_COLS) col = LCD_COLS - 1;
    if (row >= LCD_ROWS) row = LCD_ROWS - 1;

    static const uint8_t row_offsets[] = LCD_ROW_OFFSET;
    uint8_t data = 0x80 | (col + row_offsets[row]);
    ESP_ERROR_CHECK(i2c_send_4bit_data(data, LCD_RS_CMD));
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    // Update the cursor position in the buffer
    if (col < LCD_COLS && row < LCD_ROWS) {
        cursor_col = col;
        cursor_row = row;
    }
}

void lcd_write_character(char c) {
    // Write a single character to the buffer
    if (cursor_col < LCD_COLS && cursor_row < LCD_ROWS) {
        lcd_buffer[cursor_row * LCD_COLS + cursor_col] = c;
        cursor_col++;
        if (cursor_col >= LCD_COLS) {
            cursor_col = 0;
            cursor_row = (cursor_row + 1) % LCD_ROWS;
        }
    }
}

void lcd_write_text(const char *str) {
    // Write a string to the buffer
    while (*str) {
        lcd_write_character(*str);
        str++;
    }
}

void lcd_copy_to_buffer(const char *data, size_t size, int8_t col, int8_t row) {
    // Copy data to the LCD buffer at a specific position
    if (col < 0 || col >= LCD_COLS || row < 0 || row >= LCD_ROWS) {
        return;
    }
    if (size > LCD_COLS - col) {
        size = LCD_COLS - col;
    }

    for (size_t i = 0; i < size; i++) {
        lcd_buffer[(row * LCD_COLS) + col + i] =
            (data[i] < 128) ? (char)data[i] : ' ';
    }
    lcd_set_cursor(col + size, row);
}

void lcd_clear_buffer(void) {
    // Clear the LCD buffer
    for (int i = 0; i < LCD_BUFFER_SIZE; i++) {
        lcd_buffer[i] = ' ';
    }
    lcd_set_cursor(0, 0);
}

void lcd_render(void) {
    // Render the buffer content to the LCD
    for (uint8_t row = 0; row < LCD_ROWS; row++) {
        lcd_set_cursor_position(0, row);
        for (uint8_t col = 0; col < LCD_COLS; col++) {
            ESP_ERROR_CHECK(i2c_send_4bit_data(lcd_buffer[row * LCD_COLS + col], LCD_RS_DATA));
        }
    }
}

void lcd_toggle_backlight(bool state) {
    // Control the LCD backlight
    if (state) {
        lcd_backlight_status |= LCD_BACKLIGHT;
    } else {
        lcd_backlight_status &= ~LCD_BACKLIGHT;
    }
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_device_handle, &lcd_backlight_status, 1, -1));
}

void lcd_format_temperature(float temp, char *buffer, size_t buffer_size) {
    // Format temperature as a string
    if (buffer_size < 5) {
        return; // Buffer too small
    }
    if (temp < 0) {
        buffer[0] = '-';
        temp = -temp;
    } else {
        buffer[0] = temp >= 100 ? ((int)(temp / 100) % 10) + '0' : ' ';
    }
    buffer[1] = temp < 10 ? ' ' : (int)(temp / 10) % 10 + '0';
    buffer[2] = (int)temp % 10 + '0';
    buffer[3] = '.';
    buffer[4] = (int)(temp * 10) % 10 + '0';
}

void lcd_display_temperatures(float *temps, size_t size) {
    if (size != 6) {
        return; // Not enough space for 6 temperatures
    }
    lcd_clear_buffer();

    char bgBuffer[] = "T1:     C  T4:     C";
    lcd_set_cursor(0, 0);
    lcd_copy_to_buffer(bgBuffer, strlen(bgBuffer), 0, 0);
    bgBuffer[1] = '2';
    bgBuffer[12] = '5';
    lcd_set_cursor(0, 1);
    lcd_copy_to_buffer(bgBuffer, strlen(bgBuffer), 0, 1);
    bgBuffer[1] = '3';
    bgBuffer[12] = '6';
    lcd_set_cursor(0, 2);
    lcd_copy_to_buffer(bgBuffer, strlen(bgBuffer), 0, 2);
    char avgBuffer[] = "     C<     C<     C";
    lcd_set_cursor(0, 3);
    lcd_copy_to_buffer(avgBuffer, strlen(avgBuffer), 0, 3);

    char buffer[6] = {0};
    float min_temp = 200.0;
    float max_temp = -20.0;
    float avg_temp = 0.0;

    for (int i = 0; i < size; i++) {
        if (temps[i] < min_temp) {
            min_temp = temps[i];
        }
        if (temps[i] > max_temp) {
            max_temp = temps[i];
        }
        avg_temp += temps[i];
        lcd_set_cursor(i < 3 ? 3 : 14, i % 3);
        lcd_format_temperature(temps[i], buffer, sizeof(buffer));
        lcd_write_text(buffer);
    }
    avg_temp /= size;
    lcd_set_cursor(0, 3);
    lcd_format_temperature(min_temp, buffer, sizeof(buffer));
    lcd_write_text(buffer);
    
    lcd_set_cursor(7, 3);
    lcd_format_temperature(avg_temp, buffer, sizeof(buffer));
    lcd_write_text(buffer);

    lcd_set_cursor(14, 3);
    lcd_format_temperature(max_temp, buffer, sizeof(buffer));
    lcd_write_text(buffer);

    lcd_render(); // Draw the buffer to the LCD
}

void lcd_update_task(void *pvParameter) {
    // Periodically update the LCD with temperature data
    float temps[6];
    while (1)
    {
        for (int i = 0; i < 6; i++) {
            temps[i] = ntc_adc_raw_to_temperature(ntc_get_channel_data(i));
        }
        lcd_display_temperatures(temps, 6);
        vTaskDelay(pdMS_TO_TICKS(500)); // Update every second
    }
}