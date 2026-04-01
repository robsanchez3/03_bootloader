#include "boot_display.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Display geometry: only the left section of the physical panel is used. */
#define LCD_WIDTH      267U
#define LCD_HEIGHT     480U

/* Text layout. */
#define CHAR_W         8U
#define CHAR_H         16U
#define TEXT_X_OFFSET  16U
#define TEXT_COLS      ((LCD_WIDTH - TEXT_X_OFFSET) / CHAR_W)
#define TEXT_ROWS      (LCD_HEIGHT / CHAR_H)
#define LOG_ROWS       (TEXT_ROWS - 2U)

/* RGB565 palette tuned for the bootloader text UI. */
#define COLOR_BG       0x0000U
#define COLOR_FG       0xA534U
#define COLOR_BLUE     0x051FU
#define COLOR_GREEN    0x7F2FU
#define COLOR_RED      0xD104U

static LTDC_HandleTypeDef hltdc;
static TIM_HandleTypeDef htim3;

static uint16_t framebuf[LCD_WIDTH * LCD_HEIGHT];
static char log_lines[LOG_ROWS][TEXT_COLS + 1U];
static BootDisplayColor_t log_colors[LOG_ROWS];
static uint32_t log_count;
static char progress_label_cache[TEXT_COLS + 1U];
static uint32_t progress_percent_cache;
static uint8_t progress_cache_valid;

typedef enum
{
    BOOT_DISPLAY_LINE_DRAW = 0,
    BOOT_DISPLAY_LINE_CLEAR_AND_DRAW
} BootDisplayLineMode_t;

static void BootDisplay_GPIO_Init(void);
static void BootDisplay_TIM3_Init(void);
static void BootDisplay_LTDC_Init(void);
static void BootDisplay_PutChar(uint32_t x, uint32_t y, char c, uint16_t fg_color);
static void BootDisplay_DrawPixel(uint32_t x, uint32_t y, uint16_t color);
static void BootDisplay_ClearLine(uint32_t row);
static void BootDisplay_RenderLine(uint32_t row, const char *text, BootDisplayColor_t color, BootDisplayLineMode_t mode);
static void BootDisplay_ScrollLogArea(void);
static void BootDisplay_CopyText(char *dst, const char *src);
static uint16_t BootDisplay_ResolveColor(BootDisplayColor_t color);
/* 8x16 VGA-style font: 13 rows of glyph + 3 rows blank (built-in line spacing). */
static const uint8_t font_space[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const uint8_t font_percent[16] = {0x00,0x00,0x00,0x62,0x64,0x08,0x08,0x10,0x10,0x26,0x46,0x00,0x00,0x00,0x00,0x00};
static const uint8_t font_hash[16] = {0x00,0x00,0x00,0x24,0x24,0x7E,0x24,0x24,0x7E,0x24,0x24,0x00,0x00,0x00,0x00,0x00};
static const uint8_t font_dot[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00};
static const uint8_t font_colon[16] = {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00};
static const uint8_t font_dash[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t font_lbracket[16] = {0x00,0x00,0x1E,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1E,0x00,0x00,0x00,0x00};
static const uint8_t font_rbracket[16] = {0x00,0x00,0x78,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x78,0x00,0x00,0x00,0x00};
static const uint8_t font_0[16] = {0x00,0x00,0x3C,0x42,0x46,0x4A,0x4A,0x52,0x52,0x62,0x42,0x3C,0x00,0x00,0x00,0x00};
static const uint8_t font_1[16] = {0x00,0x00,0x08,0x18,0x38,0x08,0x08,0x08,0x08,0x08,0x08,0x3E,0x00,0x00,0x00,0x00};
static const uint8_t font_2[16] = {0x00,0x00,0x3C,0x42,0x42,0x02,0x04,0x08,0x10,0x20,0x40,0x7E,0x00,0x00,0x00,0x00};
static const uint8_t font_3[16] = {0x00,0x00,0x3C,0x42,0x02,0x02,0x1C,0x02,0x02,0x02,0x42,0x3C,0x00,0x00,0x00,0x00};
static const uint8_t font_4[16] = {0x00,0x00,0x04,0x0C,0x14,0x24,0x44,0x44,0x7E,0x04,0x04,0x04,0x00,0x00,0x00,0x00};
static const uint8_t font_5[16] = {0x00,0x00,0x7E,0x40,0x40,0x40,0x7C,0x02,0x02,0x02,0x42,0x3C,0x00,0x00,0x00,0x00};
static const uint8_t font_6[16] = {0x00,0x00,0x1C,0x20,0x40,0x40,0x7C,0x42,0x42,0x42,0x42,0x3C,0x00,0x00,0x00,0x00};
static const uint8_t font_7[16] = {0x00,0x00,0x7E,0x42,0x02,0x04,0x04,0x08,0x08,0x10,0x10,0x10,0x00,0x00,0x00,0x00};
static const uint8_t font_8[16] = {0x00,0x00,0x3C,0x42,0x42,0x42,0x3C,0x42,0x42,0x42,0x42,0x3C,0x00,0x00,0x00,0x00};
static const uint8_t font_9[16] = {0x00,0x00,0x3C,0x42,0x42,0x42,0x42,0x3E,0x02,0x02,0x04,0x38,0x00,0x00,0x00,0x00};
static const uint8_t font_A[16] = {0x00,0x00,0x18,0x24,0x42,0x42,0x42,0x7E,0x42,0x42,0x42,0x42,0x00,0x00,0x00,0x00};
static const uint8_t font_B[16] = {0x00,0x00,0x7C,0x42,0x42,0x42,0x7C,0x42,0x42,0x42,0x42,0x7C,0x00,0x00,0x00,0x00};
static const uint8_t font_C[16] = {0x00,0x00,0x3C,0x42,0x40,0x40,0x40,0x40,0x40,0x40,0x42,0x3C,0x00,0x00,0x00,0x00};
static const uint8_t font_D[16] = {0x00,0x00,0x78,0x44,0x42,0x42,0x42,0x42,0x42,0x42,0x44,0x78,0x00,0x00,0x00,0x00};
static const uint8_t font_E[16] = {0x00,0x00,0x7E,0x40,0x40,0x40,0x7C,0x40,0x40,0x40,0x40,0x7E,0x00,0x00,0x00,0x00};
static const uint8_t font_F[16] = {0x00,0x00,0x7E,0x40,0x40,0x40,0x7C,0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00};
static const uint8_t font_G[16] = {0x00,0x00,0x3C,0x42,0x40,0x40,0x40,0x4E,0x42,0x42,0x42,0x3C,0x00,0x00,0x00,0x00};
static const uint8_t font_H[16] = {0x00,0x00,0x42,0x42,0x42,0x42,0x7E,0x42,0x42,0x42,0x42,0x42,0x00,0x00,0x00,0x00};
static const uint8_t font_I[16] = {0x00,0x00,0x3E,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x3E,0x00,0x00,0x00,0x00};
static const uint8_t font_J[16] = {0x00,0x00,0x1E,0x04,0x04,0x04,0x04,0x04,0x04,0x44,0x44,0x38,0x00,0x00,0x00,0x00};
static const uint8_t font_K[16] = {0x00,0x00,0x42,0x44,0x48,0x50,0x60,0x50,0x48,0x44,0x42,0x42,0x00,0x00,0x00,0x00};
static const uint8_t font_L[16] = {0x00,0x00,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x7E,0x00,0x00,0x00,0x00};
static const uint8_t font_M[16] = {0x00,0x00,0x42,0x66,0x5A,0x5A,0x42,0x42,0x42,0x42,0x42,0x42,0x00,0x00,0x00,0x00};
static const uint8_t font_N[16] = {0x00,0x00,0x42,0x62,0x52,0x52,0x4A,0x4A,0x46,0x46,0x42,0x42,0x00,0x00,0x00,0x00};
static const uint8_t font_O[16] = {0x00,0x00,0x3C,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3C,0x00,0x00,0x00,0x00};
static const uint8_t font_P[16] = {0x00,0x00,0x7C,0x42,0x42,0x42,0x7C,0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00};
static const uint8_t font_R[16] = {0x00,0x00,0x7C,0x42,0x42,0x42,0x7C,0x48,0x44,0x44,0x42,0x42,0x00,0x00,0x00,0x00};
static const uint8_t font_S[16] = {0x00,0x00,0x3C,0x42,0x40,0x40,0x3C,0x02,0x02,0x02,0x42,0x3C,0x00,0x00,0x00,0x00};
static const uint8_t font_T[16] = {0x00,0x00,0x7F,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x00,0x00,0x00,0x00};
static const uint8_t font_U[16] = {0x00,0x00,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3C,0x00,0x00,0x00,0x00};
static const uint8_t font_V[16] = {0x00,0x00,0x42,0x42,0x42,0x42,0x42,0x24,0x24,0x24,0x18,0x18,0x00,0x00,0x00,0x00};
static const uint8_t font_W[16] = {0x00,0x00,0x42,0x42,0x42,0x42,0x42,0x5A,0x5A,0x5A,0x66,0x42,0x00,0x00,0x00,0x00};
static const uint8_t font_X[16] = {0x00,0x00,0x42,0x42,0x24,0x24,0x18,0x24,0x24,0x42,0x42,0x42,0x00,0x00,0x00,0x00};
static const uint8_t font_Y[16] = {0x00,0x00,0x42,0x42,0x24,0x24,0x18,0x08,0x08,0x08,0x08,0x08,0x00,0x00,0x00,0x00};
static const uint8_t font_Z[16] = {0x00,0x00,0x7E,0x02,0x04,0x08,0x10,0x10,0x20,0x40,0x40,0x7E,0x00,0x00,0x00,0x00};

static const uint8_t *BootDisplay_GetGlyph(char c)
{
    switch (c)
    {
        case '%': return font_percent;
        case '#': return font_hash;
        case '.': return font_dot;
        case ':': return font_colon;
        case '-': return font_dash;
        case '[': return font_lbracket;
        case ']': return font_rbracket;
        case '0': return font_0;
        case '1': return font_1;
        case '2': return font_2;
        case '3': return font_3;
        case '4': return font_4;
        case '5': return font_5;
        case '6': return font_6;
        case '7': return font_7;
        case '8': return font_8;
        case '9': return font_9;
        case 'H': return font_H;
        case 'O': return font_O;
        case 'L': return font_L;
        case 'A': return font_A;
        case 'B': return font_B;
        case 'C': return font_C;
        case 'M': return font_M;
        case 'E': return font_E;
        case 'F': return font_F;
        case 'G': return font_G;
        case 'I': return font_I;
        case 'J': return font_J;
        case 'K': return font_K;
        case 'U': return font_U;
        case 'N': return font_N;
        case 'D': return font_D;
        case 'P': return font_P;
        case 'R': return font_R;
        case 'S': return font_S;
        case 'T': return font_T;
        case 'V': return font_V;
        case 'W': return font_W;
        case 'Y': return font_Y;
        case 'X': return font_X;
        case 'Z': return font_Z;
        case ' ': return font_space;
        default:  return font_space;
    }
}


void BootDisplay_Init(void)
{
    BootDisplay_GPIO_Init();
    BootDisplay_TIM3_Init();
    BootDisplay_LTDC_Init();

    HAL_GPIO_WritePin(LCD_CTRL_GPIO_Port, LCD_CTRL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 150U);

    BootDisplay_Clear();
}

void BootDisplay_Clear(void)
{
    uint32_t i;

    for (i = 0; i < (LCD_WIDTH * LCD_HEIGHT); i++)
    {
        framebuf[i] = COLOR_BG;
    }

    memset(log_lines, 0, sizeof(log_lines));
    memset(log_colors, 0, sizeof(log_colors));
    log_count = 0U;
    memset(progress_label_cache, 0, sizeof(progress_label_cache));
    progress_percent_cache = 0U;
    progress_cache_valid = 0U;
}

void BootDisplay_WriteLine(uint32_t row, const char *text)
{
    BootDisplay_RenderLine(row, text, BOOT_DISPLAY_COLOR_NORMAL, BOOT_DISPLAY_LINE_DRAW);
}

void BootDisplay_Log(const char *text)
{
    BootDisplay_LogColor(text, BOOT_DISPLAY_COLOR_NORMAL);
}

void BootDisplay_LogColor(const char *text, BootDisplayColor_t color)
{
    uint32_t i;

    if (log_count < LOG_ROWS)
    {
        BootDisplay_CopyText(log_lines[log_count], text);
        log_colors[log_count] = color;
        BootDisplay_RenderLine(log_count,
                               log_lines[log_count],
                               log_colors[log_count],
                               BOOT_DISPLAY_LINE_CLEAR_AND_DRAW);
        log_count++;
    }
    else
    {
        BootDisplay_ScrollLogArea();
        for (i = 1U; i < LOG_ROWS; i++)
        {
            memcpy(log_lines[i - 1U], log_lines[i], TEXT_COLS + 1U);
            log_colors[i - 1U] = log_colors[i];
        }
        BootDisplay_CopyText(log_lines[LOG_ROWS - 1U], text);
        log_colors[LOG_ROWS - 1U] = color;
        BootDisplay_RenderLine(LOG_ROWS - 1U,
                               log_lines[LOG_ROWS - 1U],
                               log_colors[LOG_ROWS - 1U],
                               BOOT_DISPLAY_LINE_CLEAR_AND_DRAW);
    }
}

void BootDisplay_ShowStatus(const char *line0, const char *line1)
{
    BootDisplay_Log(line0);
    BootDisplay_Log(line1);
}

void BootDisplay_ShowProgress(const char *label, uint32_t current, uint32_t total)
{
    char line[40];
    char bar[17];
    uint32_t i;
    uint32_t percent = 0U;
    uint32_t filled = 0U;

    if (total != 0U)
    {
        percent = (current * 100U) / total;
    }

    if ((progress_cache_valid != 0U) &&
        (progress_percent_cache == percent) &&
        (strncmp(progress_label_cache, label, TEXT_COLS) == 0))
    {
        return;
    }

    filled = percent / 10U;
    for (i = 0U; i < 10U; i++)
    {
        bar[i] = (i < filled) ? '#' : '.';
    }
    bar[10] = '\0';

    (void)snprintf(line, sizeof(line), "[%s] %3lu%%", bar, (unsigned long)percent);

    if ((progress_cache_valid == 0U) ||
        (strncmp(progress_label_cache, label, TEXT_COLS) != 0))
    {
        BootDisplay_RenderLine(TEXT_ROWS - 2U,
                               label,
                               BOOT_DISPLAY_COLOR_NORMAL,
                               BOOT_DISPLAY_LINE_CLEAR_AND_DRAW);
        BootDisplay_CopyText(progress_label_cache, label);
    }

    BootDisplay_RenderLine(TEXT_ROWS - 1U,
                           line,
                           BOOT_DISPLAY_COLOR_NORMAL,
                           BOOT_DISPLAY_LINE_DRAW);
    progress_percent_cache = percent;
    progress_cache_valid = 1U;
}

/* Clear the two bottom rows reserved for progress title and bar. */
void BootDisplay_ClearProgress(void)
{
    BootDisplay_ClearLine(TEXT_ROWS - 2U);
    BootDisplay_ClearLine(TEXT_ROWS - 1U);
    memset(progress_label_cache, 0, sizeof(progress_label_cache));
    progress_percent_cache = 0U;
    progress_cache_valid = 0U;
}

static void BootDisplay_PutChar(uint32_t x, uint32_t y, char c, uint16_t fg_color)
{
    uint32_t r;
    uint32_t col;
    const uint8_t *glyph = BootDisplay_GetGlyph(c);

    for (r = 0; r < CHAR_H; r++)
    {
        uint8_t bits = glyph[r];

        for (col = 0; col < CHAR_W; col++)
        {
            uint16_t color = (bits & (1U << (7U - col))) ? fg_color : COLOR_BG;
            BootDisplay_DrawPixel(x + col, y + r, color);
        }
    }
}

static void BootDisplay_DrawPixel(uint32_t x, uint32_t y, uint16_t color)
{
    if ((x < LCD_WIDTH) && (y < LCD_HEIGHT))
    {
        framebuf[(y * LCD_WIDTH) + x] = color;
    }
}

static void BootDisplay_ClearLine(uint32_t row)
{
    uint32_t y;
    uint32_t x;
    uint32_t y0 = row * CHAR_H;

    for (y = 0U; y < CHAR_H; y++)
    {
        for (x = 0U; x < LCD_WIDTH; x++)
        {
            BootDisplay_DrawPixel(x, y0 + y, COLOR_BG);
        }
    }
}

static void BootDisplay_RenderLine(uint32_t row, const char *text, BootDisplayColor_t color, BootDisplayLineMode_t mode)
{
    uint32_t x;
    uint32_t y;
    uint16_t fg_color;

    if (mode == BOOT_DISPLAY_LINE_CLEAR_AND_DRAW)
    {
        BootDisplay_ClearLine(row);
    }

    x = 0U;
    y = row * CHAR_H;
    fg_color = BootDisplay_ResolveColor(color);

    while ((text[x] != '\0') && (x < TEXT_COLS))
    {
        BootDisplay_PutChar(TEXT_X_OFFSET + (x * CHAR_W), y, text[x], fg_color);
        x++;
    }
}

/* Scroll only the framebuffer area used by the log, then blank the last row. */
static void BootDisplay_ScrollLogArea(void)
{
    uint32_t src_row_start = CHAR_H;
    uint32_t rows_to_move = (LOG_ROWS - 1U) * CHAR_H;
    uint32_t bytes_per_row = LCD_WIDTH * sizeof(framebuf[0]);
    uint32_t blank_row = (LOG_ROWS - 1U);

    memmove(&framebuf[0],
            &framebuf[src_row_start * LCD_WIDTH],
            rows_to_move * bytes_per_row);

    BootDisplay_ClearLine(blank_row);
}

/* Copy and clamp a text line so it always fits the visible text width. */
static void BootDisplay_CopyText(char *dst, const char *src)
{
    uint32_t i = 0U;

    while ((i < TEXT_COLS) && (src[i] != '\0'))
    {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

/* Convert logical bootloader colors to the RGB565 palette used by the panel. */
static uint16_t BootDisplay_ResolveColor(BootDisplayColor_t color)
{
    switch (color)
    {
        case BOOT_DISPLAY_COLOR_BLUE:
            return COLOR_BLUE;
        case BOOT_DISPLAY_COLOR_GREEN:
            return COLOR_GREEN;
        case BOOT_DISPLAY_COLOR_RED:
            return COLOR_RED;
        case BOOT_DISPLAY_COLOR_NORMAL:
        default:
            return COLOR_FG;
    }
}

static void BootDisplay_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    GPIO_InitStruct.Pin = LCD_RESET_Pin | LCD_CTRL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LCD_BL_PWM_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

static void BootDisplay_TIM3_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM3_CLK_ENABLE();

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 2499;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 199;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim3);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 150;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3);
}

static void BootDisplay_LTDC_Init(void)
{
    LTDC_LayerCfgTypeDef pLayerCfg = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInit.LtdcClockSelection = RCC_LTDCCLKSOURCE_PLL3;
    PeriphClkInit.PLL3.PLL3Source = RCC_PLLSOURCE_HSE;
    PeriphClkInit.PLL3.PLL3M = 5;
    PeriphClkInit.PLL3.PLL3N = 49;
    PeriphClkInit.PLL3.PLL3P = 2;
    PeriphClkInit.PLL3.PLL3Q = 5;
    PeriphClkInit.PLL3.PLL3R = 9;
    PeriphClkInit.PLL3.PLL3RGE = RCC_PLLVCIRANGE_0;
    PeriphClkInit.PLL3.PLL3FRACN = 0;
    PeriphClkInit.PLL3.PLL3ClockOut = RCC_PLL3_DIVR;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    __HAL_RCC_LTDC_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_LTDC;

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_8 |
                          GPIO_PIN_13 | GPIO_PIN_7 | GPIO_PIN_15 | GPIO_PIN_12 |
                          GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_14;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_15 |
                          GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_8 | GPIO_PIN_13 |
                          GPIO_PIN_14 | GPIO_PIN_10 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_15 |
                          GPIO_PIN_14;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    hltdc.Instance = LTDC;
    hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IIPC;
    hltdc.Init.HorizontalSync = 2;
    hltdc.Init.VerticalSync = 1;
    hltdc.Init.AccumulatedHBP = 15;
    hltdc.Init.AccumulatedVBP = 4;
    hltdc.Init.AccumulatedActiveW = 815;
    hltdc.Init.AccumulatedActiveH = 484;
    hltdc.Init.TotalWidth = 859;
    hltdc.Init.TotalHeigh = 528;
    hltdc.Init.Backcolor.Blue = 0;
    hltdc.Init.Backcolor.Green = 0;
    hltdc.Init.Backcolor.Red = 0;
    HAL_LTDC_Init(&hltdc);

    pLayerCfg.WindowX0 = 0;
    pLayerCfg.WindowX1 = LCD_WIDTH;
    pLayerCfg.WindowY0 = 0;
    pLayerCfg.WindowY1 = LCD_HEIGHT;
    pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    pLayerCfg.Alpha = 255;
    pLayerCfg.Alpha0 = 0;
    pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
    pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
    pLayerCfg.FBStartAdress = (uint32_t)framebuf;
    pLayerCfg.ImageWidth = LCD_WIDTH;
    pLayerCfg.ImageHeight = LCD_HEIGHT;
    pLayerCfg.Backcolor.Blue = 0;
    pLayerCfg.Backcolor.Green = 0;
    pLayerCfg.Backcolor.Red = 0;
    HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0);
}
