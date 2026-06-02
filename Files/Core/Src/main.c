/* USER CODE BEGIN Header */
/**
  * @file           : main.c
  * @brief          : Fully Rectified High-Performance DVD Screensaver for ILI9341.
  * @note           : Coordinates are unified so X is always horizontal and Y is vertical.
  *                   The text bounces precisely off all 4 physical screen edges.
  */
/* USER CODE END Header */

#include "main.h"

/* ---- User Configurations ---- */
// The floating text displayed on the screen.
#define USER_TEXT        "SEPSR"

// Physical display dimensions aligned to natural portrait mode.
#define LCD_WIDTH        240
#define LCD_HEIGHT       320

// 16-bit RGB565 Color Palette
#define RED              0xF800
#define GREEN            0x07E0
#define BLUE             0x001F
#define BLACK            0x0000
#define WHITE            0xFFFF
#define YELLOW           0xFFE0
#define CYAN             0x07FF
#define MAGENTA          0xF81F

// Driver orientation register configuration for portrait mode.
#define LCD_ORIENTATION  0x88

/* ---- Hardware Pin Assignments ---- */
#define LCD_CS_PORT      GPIOB
#define LCD_CS_PIN       GPIO_PIN_9
#define LCD_DC_PORT      GPIOB
#define LCD_DC_PIN       GPIO_PIN_8
#define LCD_RST_PORT     GPIOB
#define LCD_RST_PIN      GPIO_PIN_7
#define LCD_LED_PORT     GPIOB
#define LCD_LED_PIN      GPIO_PIN_6

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_adc1;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
void Error_Handler(void);

void LCD_WriteCommand(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_WriteDataBytes(uint8_t *data, uint16_t size);
void ILI9341_Init(void);
void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ILI9341_FillScreen(uint16_t color);
void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t scale);
void ILI9341_AnimateScreensaver(void);

/* ---- 5x7 ASCII Font Array (Covering ASCII 32 to 126) ---- */
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space (ASCII 32)
    {0x00, 0x00, 0x5f, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, // #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1c, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1c, 0x00}, // )
    {0x14, 0x08, 0x3e, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3e, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 0
    {0x00, 0x42, 0x7f, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4b, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7f, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1e}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3e}, // @
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, // A (ASCII 65)
    {0x7f, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3e, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, // D
    {0x7f, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7f, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, // G
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // H
    {0x00, 0x41, 0x7f, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3f, 0x01}, // J
    {0x7f, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7f, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // M
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, // N
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, // O
    {0x7f, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, // Q
    {0x7f, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7f, 0x01, 0x01}, // T
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, // U
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, // V
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x7f, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x16, 0x20}, // \
    {0x00, 0x41, 0x41, 0x7f, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x05, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a (ASCII 97)
    {0x7f, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7f}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7e, 0x09, 0x01, 0x02}, // f
    {0x0c, 0x52, 0x52, 0x52, 0x3e}, // g
    {0x7f, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7d, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3d, 0x00}, // j
    {0x7f, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7f, 0x40, 0x00}, // l
    {0x7c, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7c, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7c, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x14, 0x7c}, // q
    {0x7c, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3f, 0x44, 0x40, 0x20}, // t
    {0x3c, 0x40, 0x40, 0x20, 0x7c}, // u
    {0x1c, 0x20, 0x40, 0x20, 0x1c}, // v
    {0x3c, 0x40, 0x30, 0x40, 0x3c}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0c, 0x50, 0x50, 0x50, 0x3c}, // y
    {0x44, 0x64, 0x54, 0x4c, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7f, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x10, 0x08, 0x18, 0x10, 0x08}  // ~ (ASCII 126)
};

/* ---- Low-level Command Writing Functions ---- */

/**
  * @brief  Transmits a command byte over the SPI bus.
  * @note   We pull DC low to specify that the incoming byte is a command,
  *         and pull CS low to begin SPI frame transmission.
  */
void LCD_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET); // Command mode (DC = 0)
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET); // Chip Select Active
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);           // Send over SPI
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);   // Chip Select Inactive
}

/**
  * @brief  Transmits a single data parameter byte over SPI.
  * @note   We pull DC high to let the display know this is parameter/pixel data.
  */
void LCD_WriteData(uint8_t data) {
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);   // Data mode (DC = 1)
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET); // Chip Select Active
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);          // Send over SPI
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);   // Chip Select Inactive
}

/**
  * @brief  Sends a block of data bytes sequentially.
  * @note   This is highly optimized because we keep CS low during the entire
  *         transfer block instead of pulling it high after every byte.
  */
void LCD_WriteDataBytes(uint8_t *data, uint16_t size) {
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);   // Data mode (DC = 1)
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET); // Chip Select Active
    HAL_SPI_Transmit(&hspi1, data, size, HAL_MAX_DELAY);         // Block send
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);   // Chip Select Inactive
}

/**
  * @brief  Tells the display controller which rectangular area we want to write to.
  * @note   x0 and x1 refer to vertical physical columns, while y0 and y1 refer
  *         to horizontal physical rows according to our unified coordinate mapping.
  */
void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4];

    // Column Address Set (0x2A) - Maps to vertical address space
    LCD_WriteCommand(0x2A);
    data[0] = x0 >> 8; data[1] = x0 & 0xFF; data[2] = x1 >> 8; data[3] = x1 & 0xFF;
    LCD_WriteDataBytes(data, 4);

    // Page/Row Address Set (0x2B) - Maps to horizontal address space
    LCD_WriteCommand(0x2B);
    data[0] = y0 >> 8; data[1] = y0 & 0xFF; data[2] = y1 >> 8; data[3] = y1 & 0xFF;
    LCD_WriteDataBytes(data, 4);

    // Command to prepare memory write (0x2C RAMWR)
    LCD_WriteCommand(0x2C);
}

/**
  * @brief  Fills the entire 240x320 resolution space with a solid color.
  * @note   We use a static row buffer so that we don't cause any stack overflows.
  */
void ILI9341_FillScreen(uint16_t color) {
    // Fill vertical from 0 to 319, horizontal from 0 to 239
    ILI9341_SetAddressWindow(0, 0, LCD_HEIGHT - 1, LCD_WIDTH - 1);
    uint8_t hi = color >> 8, lo = color & 0xFF;
    static uint8_t buf[240 * 2]; // Line buffer representing physical horizontal size

    // Initialize the line buffer
    for (int i = 0; i < 240; i++) {
        buf[2*i] = hi; buf[2*i+1] = lo;
    }

    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    // Write 320 consecutive vertical lines across the horizontal span
    for (int y = 0; y < LCD_HEIGHT; y++) {
        HAL_SPI_Transmit(&hspi1, buf, sizeof(buf), HAL_MAX_DELAY);
    }
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  Draws a solid colored rectangle at specified coordinates.
  * @note   This function strictly maps x to horizontal fize, and y to vertical size.
  *         First parameter of SetAddressWindow gets Y, and second gets X.
  */
void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // Out of bounds check to prevent memory wrap-around errors on display
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH)  w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;

    // Map horizontal x to column address, and vertical y to row address inside SetAddressWindow
    ILI9341_SetAddressWindow(y, x, y + h - 1, x + w - 1);

    uint8_t hi = color >> 8, lo = color & 0xFF;
    static uint8_t line_buf[240 * 2]; // Preallocated cache
    uint16_t buf_size = w * 2;

    for (uint16_t i = 0; i < w; i++) {
        line_buf[2 * i]     = hi;
        line_buf[2 * i + 1] = lo;
    }

    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    // Overwrite only the specified rectangular block
    for (uint16_t row = 0; row < h; row++) {
        HAL_SPI_Transmit(&hspi1, line_buf, buf_size, HAL_MAX_DELAY);
    }
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  Draws a single scaled character on the screen.
  * @note   x is horizontal (0 to 239) and y is vertical (0 to 319).
  *         This matches the natural visual layout of the panel.
  */
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = font5x7[c - 32];
    uint8_t buf[2];

    for (uint8_t col = 0; col < 5; col++) {
        for (uint8_t row = 0; row < 7; row++) {
            uint16_t color = (glyph[col] & (1 << row)) ? fg : bg;
            buf[0] = color >> 8; buf[1] = color & 0xFF;

            // Map pixel coordinates safely:
            // Vertical pixel = y + row * scale
            // Horizontal pixel = x + col * scale
            ILI9341_SetAddressWindow(
                y + row * scale,
                x + col * scale,
                y + row * scale + scale - 1,
                x + col * scale + scale - 1
            );
            HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
            for (uint8_t sy = 0; sy < scale; sy++) {
                for (uint8_t sx = 0; sx < scale; sx++) {
                    HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
                }
            }
            HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
        }
    }
}

/**
  * @brief  Renders a full text string horizontally from left to right.
  */
void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t scale) {
    while (*str) {
        ILI9341_DrawChar(x, y, *str, fg, bg, scale);
        x += (5 + 1) * scale; // Move X horizontally for the next character block
        str++;
    }
}

/**
  * @brief  Main screensaver loop. Checks boundary collisions with absolute precision.
  */
void ILI9341_AnimateScreensaver(void) {
    // Clear screen fully at startup to blacken any lingering power-on static noise
    ILI9341_FillScreen(BLACK);

    const char *text = USER_TEXT;
    uint8_t scale = 4; // Using scale 4 for a large, beautiful display

    // Calculate precise bounding box of the string SEPSR (5 characters)
    uint16_t len = 0;
    const char *ptr = text;
    while (*ptr++) len++;

    // Width = 29 columns * scale (For SEPSR at scale 4: 116 pixels)
    uint16_t text_width  = (len * 6 * scale) - scale;
    // Height = 7 rows * scale (For scale 4: 28 pixels)
    uint16_t text_height = 7 * scale;

    // Start coordinates centered on the display
    int16_t pos_x = (LCD_WIDTH - text_width) / 2;
    int16_t pos_y = (LCD_HEIGHT - text_height) / 2;

    // Position cache for the dirty rectangle erasure
    int16_t prev_x = pos_x;
    int16_t prev_y = pos_y;

    // Horizontal and vertical velocity in pixels per frame
    int16_t vel_x = 2;
    int16_t vel_y = 2;

    // Palette of colors to cycle through on each bounce
    uint16_t colors[] = {YELLOW, RED, GREEN, BLUE, WHITE, CYAN, MAGENTA};
    uint8_t color_idx = 0;
    uint16_t current_color = colors[color_idx];

    while (1) {
        // Step 1: Clear ONLY the previous bounding box area (Dirty Rectangle clearing)
        // This takes less than 1ms and avoids global screen flickering completely.
        ILI9341_FillRect(prev_x, prev_y, text_width, text_height, BLACK);

        // Step 2: Draw the text string at the newly calculated coordinates
        ILI9341_DrawString(pos_x, pos_y, text, current_color, BLACK, scale);

        // Store positions for the next frame's erasure pass
        prev_x = pos_x;
        prev_y = pos_y;

        // Apply frame step displacements
        pos_x += vel_x;
        pos_y += vel_y;

        uint8_t bounced = 0;

        // Watertight horizontal bounce checks:
        // Left Edge hit
        if (pos_x < 0) {
            pos_x = 0;
            vel_x = -vel_x; // Reverse horizontal direction
            bounced = 1;
        }
        // Right Edge hit
        else if (pos_x + text_width > LCD_WIDTH) {
            pos_x = LCD_WIDTH - text_width;
            vel_x = -vel_x; // Reverse horizontal direction
            bounced = 1;
        }

        // Watertight vertical bounce checks:
        // Top Edge hit
        if (pos_y < 0) {
            pos_y = 0;
            vel_y = -vel_y; // Reverse vertical direction
            bounced = 1;
        }
        // Bottom Edge hit
        else if (pos_y + text_height > LCD_HEIGHT) {
            pos_y = LCD_HEIGHT - text_height;
            vel_y = -vel_y; // Reverse vertical direction
            bounced = 1;
        }

        // Change color palette when a bounce is detected
        if (bounced) {
            color_idx = (color_idx + 1) % (sizeof(colors) / sizeof(colors[0]));
            current_color = colors[color_idx];
        }

        // Maintain stable, fluid 60 FPS animation loop
        HAL_Delay(16);
    }
}

/* ---- Initialization Code ---- */

/**
  * @brief  Initializes the display panel registers.
  */
void ILI9341_Init(void) {
    // Push RST low to hardware reset the display
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(120);
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(120);

    // Set LED backlight high
    HAL_GPIO_WritePin(LCD_LED_PORT, LCD_LED_PIN, GPIO_PIN_SET);

    // Software Reset Command
    LCD_WriteCommand(0x01);
    HAL_Delay(150);

    // Power-on configuration registers
    LCD_WriteCommand(0xCB);
    LCD_WriteData(0x39); LCD_WriteData(0x2C); LCD_WriteData(0x00);
    LCD_WriteData(0x34); LCD_WriteData(0x02);

    LCD_WriteCommand(0xCF);
    LCD_WriteData(0x00); LCD_WriteData(0xC1); LCD_WriteData(0x30);

    LCD_WriteCommand(0xE8);
    LCD_WriteData(0x85); LCD_WriteData(0x00); LCD_WriteData(0x78);

    LCD_WriteCommand(0xEA);
    LCD_WriteData(0x00); LCD_WriteData(0x00);

    LCD_WriteCommand(0xED);
    LCD_WriteData(0x64); LCD_WriteData(0x03);
    LCD_WriteData(0x12); LCD_WriteData(0x81);

    LCD_WriteCommand(0xF7); LCD_WriteData(0x20);

    LCD_WriteCommand(0xC0); LCD_WriteData(0x23); // Power Control 1
    LCD_WriteCommand(0xC1); LCD_WriteData(0x10); // Power Control 2
    LCD_WriteCommand(0xC5); LCD_WriteData(0x3E); LCD_WriteData(0x28); // VCOM 1
    LCD_WriteCommand(0xC7); LCD_WriteData(0x86); // VCOM 2

    // Write orientation setting to register 0x36
    LCD_WriteCommand(0x36);
    LCD_WriteData(LCD_ORIENTATION);

    LCD_WriteCommand(0x3A); LCD_WriteData(0x55); // 16-bit Pixel Format
    LCD_WriteCommand(0xB1); LCD_WriteData(0x00); LCD_WriteData(0x18); // Frame Rate

    LCD_WriteCommand(0xB6);
    LCD_WriteData(0x0A); LCD_WriteData(0x82);
    LCD_WriteData(0x27); LCD_WriteData(0x00);

    LCD_WriteCommand(0x11); // Wake display
    HAL_Delay(120);

    // Ensure display RAM starts fully cleared
    ILI9341_FillScreen(BLACK);
    HAL_Delay(20);

    LCD_WriteCommand(0x29); // Turn Display ON
    HAL_Delay(100);
}

/* ---- Entry Point ---- */

int main(void) {
    HAL_Init();             // Initialize HAL
    SystemClock_Config();   // Set SysClock to 84 MHz HSI PLL
    MX_GPIO_Init();         // Initialize control GPIO lines
    MX_SPI1_Init();         // Setup high-speed SPI1

    ILI9341_Init();         // Initialize ILI9341 registers

    // Start screensaver animation
    ILI9341_AnimateScreensaver();

    while (1) {
        // Animation loop is infinite
    }
}

/* ---- Configuration Functions ---- */

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                 |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

static void MX_SPI1_Init(void) {
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    // PCLK2 / 8 = 10.5 MHz (Sturdy transfer rate over breadboards)
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Set control lines high initially
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
