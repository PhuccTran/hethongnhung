#include "ssd1306.h"

#define SSD1306_CONTROL_COMMAND  0x00U
#define SSD1306_CONTROL_DATA     0x40U
#define SSD1306_I2C_TIMEOUT      100U
#define SSD1306_CHAR_WIDTH       6U
#define SSD1306_CHAR_HEIGHT      8U

static I2C_HandleTypeDef *ssd1306_i2c;
static uint8_t ssd1306_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8U];
static uint8_t cursor_x;
static uint8_t cursor_y;
static uint8_t text_size = 1U;

static const uint8_t font_digits[10][5] =
{
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00,tatic const uint8_t font_upper[26][5] =
{
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
  {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
  {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
  {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
  {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
  {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
  {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63},
  {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}
}; 0x42, 0x7F, 0x40, 0x00},
  {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
  {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
  {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}
};

static const uint8_t font_upper[26][5] =
{
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
  {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
  {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
  {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
  {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
  {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
  {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63},
  {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}
};

static const uint8_t font_colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};

static HAL_StatusTypeDef ssd1306_send_command(uint8_t command)
{
  uint8_t data[2] = {SSD1306_CONTROL_COMMAND, command};

  return HAL_I2C_Master_Transmit(ssd1306_i2c, SSD1306_I2C_ADDR, data,
                                 sizeof(data), SSD1306_I2C_TIMEOUT);
}

HAL_StatusTypeDef SSD1306_Init(I2C_HandleTypeDef *hi2c)
{
  static const uint8_t init_commands[] =
  {
    0xAE, 0x20, 0x00, 0xB0, 0xC8, 0x00, 0x10, 0x40,
    0x81, 0x7F, 0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3,
    0x00, 0xD5, 0x80, 0xD9, 0xF1, 0xDA, 0x12, 0xDB,
    0x40, 0x8D, 0x14, 0xAF
  };
  HAL_StatusTypeDef status;

  if (hi2c == NULL)
  {
    return HAL_ERROR;
  }

  ssd1306_i2c = hi2c;
  status = HAL_I2C_IsDeviceReady(ssd1306_i2c, SSD1306_I2C_ADDR, 2,
                                 SSD1306_I2C_TIMEOUT);
  if (status != HAL_OK)
  {
    return status;
  }

  for (uint32_t index = 0; index < sizeof(init_commands); index++)
  {
    status = ssd1306_send_command(init_commands[index]);
    if (status != HAL_OK)
    {
      return status;
    }
  }

  SSD1306_Fill(SSD1306_COLOR_BLACK);
  cursor_x = 0;
  cursor_y = 0;
  return SSD1306_UpdateScreen();
}

HAL_StatusTypeDef SSD1306_UpdateScreen(void)
{
  uint8_t data[SSD1306_WIDTH + 1U];

  if (ssd1306_i2c == NULL)
  {
    return HAL_ERROR;
  }

  data[0] = SSD1306_CONTROL_DATA;
  for (uint8_t page = 0; page < SSD1306_HEIGHT / 8U; page++)
  {
    if (ssd1306_send_command(0xB0U + page) != HAL_OK ||
        ssd1306_send_command(0x00U) != HAL_OK ||
        ssd1306_send_command(0x10U) != HAL_OK)
    {
      return HAL_ERROR;
    }

    for (uint16_t column = 0; column < SSD1306_WIDTH; column++)
    {
      data[column + 1U] = ssd1306_buffer[(page * SSD1306_WIDTH) + column];
    }
    if (HAL_I2C_Master_Transmit(ssd1306_i2c, SSD1306_I2C_ADDR, data,
                               sizeof(data), SSD1306_I2C_TIMEOUT) != HAL_OK)
    {
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef SSD1306_SetContrast(uint8_t contrast)
{
  HAL_StatusTypeDef status = ssd1306_send_command(0x81U);
  if (status == HAL_OK)
  {
    status = ssd1306_send_command(contrast);
  }
  return status;
}

void SSD1306_Fill(SSD1306_Color color)
{
  uint8_t value = (color == SSD1306_COLOR_BLACK) ? 0x00U : 0xFFU;
  for (uint16_t index = 0; index < sizeof(ssd1306_buffer); index++)
  {
    ssd1306_buffer[index] = value;
  }
}

void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_Color color)
{
  if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
  {
    return;
  }

  if (color == SSD1306_COLOR_WHITE)
  {
    ssd1306_buffer[x + (y / 8U) * SSD1306_WIDTH] |= (uint8_t)(1U << (y % 8U));
  }
  else
  {
    ssd1306_buffer[x + (y / 8U) * SSD1306_WIDTH] &= (uint8_t)~(1U << (y % 8U));
  }
}

void SSD1306_SetCursor(uint8_t x, uint8_t y)
{
  cursor_x = x;
  cursor_y = y;
}

void SSD1306_SetTextSize(uint8_t size)
{
  if (size < 1U)
  {
    size = 1U;
  }
  if (size > 4U)
  {
    size = 4U;
  }
  text_size = size;
}

char SSD1306_WriteChar(char character)
{
  const uint8_t *glyph = NULL;

  if (character >= '0' && character <= '9')
  {
    glyph = font_digits[(uint8_t)character - (uint8_t)'0'];
  }
  else if (character >= 'a' && character <= 'z')
  {
    character = (char)(character - ('a' - 'A'));
  }
  if (character >= 'A' && character <= 'Z')
  {
    glyph = font_upper[(uint8_t)character - (uint8_t)'A'];
  }
  else if (character == ':')
  {
    glyph = font_colon;
  }

  if (character == ' ')
  {
    if (cursor_x + (SSD1306_CHAR_WIDTH * text_size) > SSD1306_WIDTH)
    {
      cursor_x = 0;
      cursor_y += SSD1306_CHAR_HEIGHT * text_size;
    }
    if (cursor_y + (SSD1306_CHAR_HEIGHT * text_size) > SSD1306_HEIGHT)
    {
      return 0;
    }
    cursor_x += SSD1306_CHAR_WIDTH * text_size;
    return character;
  }
  if (glyph == NULL)
  {
    return 0;
  }

  if (cursor_x + (SSD1306_CHAR_WIDTH * text_size) > SSD1306_WIDTH)
  {
    cursor_x = 0;
    cursor_y += SSD1306_CHAR_HEIGHT * text_size;
  }
  if (cursor_y + (SSD1306_CHAR_HEIGHT * text_size) > SSD1306_HEIGHT)
  {
    return 0;
  }

  for (uint8_t column = 0; column < 5U; column++)
  {
    for (uint8_t row = 0; row < 7U; row++)
    {
      SSD1306_Color color = (glyph[column] & (1U << row)) ?
                            SSD1306_COLOR_WHITE : SSD1306_COLOR_BLACK;
      for (uint8_t width = 0; width < text_size; width++)
      {
        for (uint8_t height = 0; height < text_size; height++)
        {
          SSD1306_DrawPixel((uint8_t)(cursor_x + column * text_size + width),
                            (uint8_t)(cursor_y + row * text_size + height), color);
        }
      }
    }
  }
  cursor_x += SSD1306_CHAR_WIDTH * text_size;
  return character;
}

char *SSD1306_WriteString(char *string)
{
  while (*string != '\0')
  {
    if (SSD1306_WriteChar(*string++) == 0)
    {
      return string;
    }
  }
  return string;
}