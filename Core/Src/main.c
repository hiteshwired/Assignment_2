/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : CPE 316 A2 - Matrix Keypad
  ******************************************************************************
  *
  * Keypad wiring assumed by this template:
  *
  *   LEDs:
  *     PC0 -> LED0
  *     PC1 -> LED1
  *     PC2 -> LED2
  *
  *   Keypad rows:
  *     PC4 -> Row 0
  *     PC5 -> Row 1
  *     PC6 -> Row 2
  *     PC7 -> Row 3
  *
  *   Keypad columns:
  *     PC8  -> Column 0
  *     PC9  -> Column 1
  *     PC10 -> Column 2
  *
  * Typical 4x3 keypad arrangement:
  *
  *              Col 0   Col 1   Col 2
  *             +-------+-------+-------+
  *     Row 0   |   1   |   2   |   3   |
  *     Row 1   |   4   |   5   |   6   |
  *     Row 2   |   7   |   8   |   9   |
  *     Row 3   |   *   |   0   |   #   |
  *             +-------+-------+-------+
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * read_keypad() returns this value when no key is pressed.
 *
 * int8_t is intentionally signed because -1 must be representable.
 */
#define NO_KEY              (-1)

/* Number of keypad rows and columns. */
#define NUM_OF_ROWS         4U
#define NUM_OF_COLS         3U

/* ------------------------------------------------------------------------- */
/* LED pins                                                                  */
/* ------------------------------------------------------------------------- */

#define LED0_PIN            0U
#define LED1_PIN            1U
#define LED2_PIN            2U

#define LED_PINS            ( \
                              (1UL << LED0_PIN) | \
                              (1UL << LED1_PIN) | \
                              (1UL << LED2_PIN)   \
                            )

/* ------------------------------------------------------------------------- */
/* Keypad row pins                                                           */
/* ------------------------------------------------------------------------- */

#define ROW0_PIN            4U
#define ROW1_PIN            5U
#define ROW2_PIN            6U
#define ROW3_PIN            7U

#define ROW_FIRST_PIN       ROW0_PIN

#define ROW_PINS            ( \
                              (1UL << ROW0_PIN) | \
                              (1UL << ROW1_PIN) | \
                              (1UL << ROW2_PIN) | \
                              (1UL << ROW3_PIN)   \
                            )

/* ------------------------------------------------------------------------- */
/* Keypad column pins                                                        */
/* ------------------------------------------------------------------------- */

#define COL0_PIN            8U
#define COL1_PIN            9U
#define COL2_PIN            10U

#define COL_FIRST_PIN       COL0_PIN

#define COL_PINS            ( \
                              (1UL << COL0_PIN) | \
                              (1UL << COL1_PIN) | \
                              (1UL << COL2_PIN)   \
                            )

/* USER CODE END PD */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

static void led_init(void);
static void keypad_init(void);

static uint8_t read_rows(void);
static int8_t row_bits_to_index(uint8_t row_bits);
static int8_t decode_key(uint8_t row, uint8_t column);

static int8_t read_keypad(void);
static void display_key(uint8_t key);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief Configure PC0-PC2 as LED outputs.
  *
  * GPIO configuration:
  *   MODER   = 01 -> general-purpose output
  *   OTYPER  = 0  -> push-pull
  *   PUPDR   = 00 -> no pull-up / pull-down
  *   OSPEEDR = 00 -> low speed
  */
static void led_init(void)
{
    /* Enable GPIOC peripheral clock. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    /*
     * MODER uses two bits per GPIO pin.
     *
     * PC0 -> bits 1:0
     * PC1 -> bits 3:2
     * PC2 -> bits 5:4
     *
     * Clear both bits first.
     */
    GPIOC->MODER &= ~(
        (3UL << (LED0_PIN * 2U)) |
        (3UL << (LED1_PIN * 2U)) |
        (3UL << (LED2_PIN * 2U))
    );

    /*
     * Output mode = 01.
     *
     * Set the lower bit of each MODER field.
     */
    GPIOC->MODER |= (
        (1UL << (LED0_PIN * 2U)) |
        (1UL << (LED1_PIN * 2U)) |
        (1UL << (LED2_PIN * 2U))
    );

    /* Push-pull outputs. */
    GPIOC->OTYPER &= ~LED_PINS;

    /* No pull-up / pull-down on LEDs. */
    GPIOC->PUPDR &= ~(
        (3UL << (LED0_PIN * 2U)) |
        (3UL << (LED1_PIN * 2U)) |
        (3UL << (LED2_PIN * 2U))
    );

    /* Low GPIO speed is sufficient for LEDs. */
    GPIOC->OSPEEDR &= ~(
        (3UL << (LED0_PIN * 2U)) |
        (3UL << (LED1_PIN * 2U)) |
        (3UL << (LED2_PIN * 2U))
    );

    /* Start with LEDs off. */
    GPIOC->BRR = LED_PINS;
}


/**
  * @brief Configure the GPIO pins used by the keypad.
  *
  * Rows PC4-PC7:
  *   input mode
  *   internal pull-down
  *
  * Columns PC8-PC10:
  *   general-purpose output
  *   push-pull
  *   no pull resistor
  *   initially LOW
  */
static void keypad_init(void)
{
    /* Enable GPIOC peripheral clock. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    /* --------------------------------------------------------------------- */
    /* Configure PC4-PC7 as inputs.                                          */
    /* --------------------------------------------------------------------- */

    /*
     * MODER = 00 for input mode.
     *
     * Clearing both mode bits is therefore enough.
     */
    GPIOC->MODER &= ~(
        (3UL << (ROW0_PIN * 2U)) |
        (3UL << (ROW1_PIN * 2U)) |
        (3UL << (ROW2_PIN * 2U)) |
        (3UL << (ROW3_PIN * 2U))
    );

    /*
     * First clear existing pull configurations.
     */
    GPIOC->PUPDR &= ~(
        (3UL << (ROW0_PIN * 2U)) |
        (3UL << (ROW1_PIN * 2U)) |
        (3UL << (ROW2_PIN * 2U)) |
        (3UL << (ROW3_PIN * 2U))
    );

    /*
     * PUPDR = 10 for pull-down.
     *
     * Because we drive columns HIGH, pressing a key connects
     * that HIGH column to one of these row inputs.
     */
    GPIOC->PUPDR |= (
        (2UL << (ROW0_PIN * 2U)) |
        (2UL << (ROW1_PIN * 2U)) |
        (2UL << (ROW2_PIN * 2U)) |
        (2UL << (ROW3_PIN * 2U))
    );

    /* --------------------------------------------------------------------- */
    /* Configure PC8-PC10 as outputs.                                        */
    /* --------------------------------------------------------------------- */

    /* Clear column MODER fields. */
    GPIOC->MODER &= ~(
        (3UL << (COL0_PIN * 2U)) |
        (3UL << (COL1_PIN * 2U)) |
        (3UL << (COL2_PIN * 2U))
    );

    /* MODER = 01 -> general-purpose output. */
    GPIOC->MODER |= (
        (1UL << (COL0_PIN * 2U)) |
        (1UL << (COL1_PIN * 2U)) |
        (1UL << (COL2_PIN * 2U))
    );

    /* Push-pull columns. */
    GPIOC->OTYPER &= ~COL_PINS;

    /* No pull-up / pull-down on output columns. */
    GPIOC->PUPDR &= ~(
        (3UL << (COL0_PIN * 2U)) |
        (3UL << (COL1_PIN * 2U)) |
        (3UL << (COL2_PIN * 2U))
    );

    /* Low speed is more than enough for human keypad scanning. */
    GPIOC->OSPEEDR &= ~(
        (3UL << (COL0_PIN * 2U)) |
        (3UL << (COL1_PIN * 2U)) |
        (3UL << (COL2_PIN * 2U))
    );

    /* Start with every column LOW. */
    GPIOC->BRR = COL_PINS;
}


/**
  * @brief Read the four keypad row inputs.
  *
  * @return Four-bit row value:
  *
  *         bit 0 -> Row 0 / PC4
  *         bit 1 -> Row 1 / PC5
  *         bit 2 -> Row 2 / PC6
  *         bit 3 -> Row 3 / PC7
  *
  * Examples:
  *
  *     0000 = no row active
  *     0001 = Row 0 active
  *     0010 = Row 1 active
  *     0100 = Row 2 active
  *     1000 = Row 3 active
  */
static uint8_t read_rows(void)
{
    uint32_t row_data;

    row_data = GPIOC->IDR & ROW_PINS;
    row_data >>= ROW_FIRST_PIN;

    return (uint8_t)row_data;
}


/**
  * @brief Convert the four-bit row pattern into a row index.
  *
  * @param row_bits Value returned by read_rows().
  *
  * @return 0-3 for one valid row, or NO_KEY for an invalid pattern.
  *
  * Multiple simultaneous row bits are treated as invalid for now.
  */
static int8_t row_bits_to_index(uint8_t row_bits)
{
    int8_t row = NO_KEY;

    if (row_bits == 0x01U)
    {
        row = 0;
    }
    else if (row_bits == 0x02U)
    {
        row = 1;
    }
    else if (row_bits == 0x04U)
    {
        row = 2;
    }
    else if (row_bits == 0x08U)
    {
        row = 3;
    }

    return row;
}


/**
  * @brief Convert a row and column into a keypad value.
  *
  * @param row     Row number 0-3.
  * @param column  Column number 0-2.
  *
  * @return Numeric key value or NO_KEY.
  *
  * The first three rows can be represented mathematically:
  *
  *     Row 0 -> 1, 2, 3
  *     Row 1 -> 4, 5, 6
  *     Row 2 -> 7, 8, 9
  *
  * TODO:
  * Decide how you want to represent the bottom row:
  *
  *             Col0   Col1   Col2
  *     Row 3     *      0      #
  *
  * The assignment lets you choose values for * and #.
  */
static int8_t decode_key(uint8_t row, uint8_t column)
{
    int8_t key = NO_KEY;

    /*
     * Rows 0-2 contain keys 1 through 9.
     */
    if ((row < 3U) && (column < NUM_OF_COLS))
    {
        key = (int8_t)((row * NUM_OF_COLS) + column + 1U);
    }
    else if (row == 3U)
    {
        /*
         * TODO: Finish this row yourself.
         *
         * Think about:
         *
         *   column == 0 -> *
         *   column == 1 -> 0
         *   column == 2 -> #
         *
         * At minimum, you should make column 1 return 0.
         *
         * Then decide what signed integer values you want to
         * assign to * and #.
         */
    }

    return key;
}


/**
  * @brief Poll and scan the 4x3 keypad.
  *
  * Stage 1:
  *   Drive all columns HIGH.
  *   If all row inputs remain LOW, no key is pressed.
  *
  * Stage 2:
  *   Drive one column HIGH at a time.
  *   Read the rows to determine the exact key.
  *
  * @return Key value, or NO_KEY if no valid key is detected.
  */
static int8_t read_keypad(void)
{
    uint8_t column;
    uint8_t row_bits;
    int8_t row;

    /* --------------------------------------------------------------------- */
    /* Stage 1: determine whether ANY key is pressed.                         */
    /* --------------------------------------------------------------------- */

    GPIOC->BSRR = COL_PINS;

    row_bits = read_rows();

    if (row_bits == 0U)
    {
        GPIOC->BRR = COL_PINS;
        return NO_KEY;
    }

    /* --------------------------------------------------------------------- */
    /* Stage 2: identify the active column.                                   */
    /* --------------------------------------------------------------------- */

    GPIOC->BRR = COL_PINS;

    for (column = 0U; column < NUM_OF_COLS; column++)
    {
        /*
         * Drive exactly one column HIGH.
         *
         * column 0 -> PC8
         * column 1 -> PC9
         * column 2 -> PC10
         */
        GPIOC->BSRR =
            (1UL << (COL_FIRST_PIN + column));

        row_bits = read_rows();

        if (row_bits != 0U)
        {
            row = row_bits_to_index(row_bits);

            /* Return outputs to inactive LOW state. */
            GPIOC->BRR = COL_PINS;

            if (row != NO_KEY)
            {
                return decode_key((uint8_t)row, column);
            }

            return NO_KEY;
        }

        /*
         * This column was not connected to an active row.
         * Turn it LOW before testing the next column.
         */
        GPIOC->BRR =
            (1UL << (COL_FIRST_PIN + column));
    }

    GPIOC->BRR = COL_PINS;

    return NO_KEY;
}


/**
  * @brief Display a keypad value using PC0-PC2.
  *
  * Three LEDs can directly represent binary values 0-7.
  *
  * The A2 instructions do not specify how 8, 9, *, and #
  * must be encoded on only three LEDs, so that decision
  * remains part of your implementation.
  */
static void display_key(uint8_t key)
{
    uint32_t led_value;

    if (key <= 7U)
    {
        /*
         * Since the LEDs are PC0-PC2, the low three bits
         * already line up with the three LED GPIO pins.
         */
        led_value = ((uint32_t)key & LED_PINS);

        /*
         * Clear only PC0-PC2.
         *
         * Do NOT write GPIOC->ODR = value here because GPIOC
         * also contains our keypad outputs.
         */
        GPIOC->BRR = LED_PINS;

        /* Set the required LED bits. */
        GPIOC->BSRR = led_value;
    }
    else
    {
        /*
         * TODO:
         *
         * Decide what the LEDs should do for:
         *
         *   8
         *   9
         *   *
         *   #
         *
         * Until you make that decision, values greater
         * than 7 leave the LEDs unchanged.
         */
    }
}

/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    int8_t key;

    /*
     * These two calls come from the ST-generated project.
     * The A2 GPIO code itself is register-level.
     */
    HAL_Init();

    SystemClock_Config();

    /*
     * We do NOT need MX_GPIO_Init() for our keypad because
     * the GPIO registers are configured directly below.
     */
    led_init();
    keypad_init();

    while (1)
    {
        key = read_keypad();

        /*
         * A2 specifically says that if no key is pressed,
         * the LEDs should retain their previous state.
         *
         * Therefore there is intentionally no "else".
         */
        if (key != NO_KEY)
        {
            display_key((uint8_t)key);
        }

        /*
         * TODO later:
         *
         * Once basic scanning works reliably, investigate
         * key release detection / switch debounce.
         *
         * Do not add that complexity until the basic
         * row/column scan works.
         */
    }
}


/**
  * @brief System Clock Configuration
  * @retval None
  *
  * This is the ST-generated clock function from the
  * project supplied in the prompt.
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /*
     * Configure the main internal regulator output voltage.
     */
    if (HAL_PWREx_ControlVoltageScaling(
        PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Configure HSI and PLL.
     */
    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;

    RCC_OscInitStruct.HSIState =
        RCC_HSI_ON;

    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_ON;

    RCC_OscInitStruct.PLL.PLLSource =
        RCC_PLLSOURCE_HSI;

    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 10;

    RCC_OscInitStruct.PLL.PLLP =
        RCC_PLLP_DIV7;

    RCC_OscInitStruct.PLL.PLLQ =
        RCC_PLLQ_DIV2;

    RCC_OscInitStruct.PLL.PLLR =
        RCC_PLLR_DIV2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Configure CPU, AHB, APB1 and APB2 clocks.
     */
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_PLLCLK;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
        &RCC_ClkInitStruct,
        FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
        /* Remain here after a fatal configuration error. */
    }
}


#ifdef USE_FULL_ASSERT

/**
  * @brief Reports the source file and line of an assert failure.
  * @param file Pointer to source file name.
  * @param line Source line number.
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /*
     * Optional debug reporting can be added here.
     */

    (void)file;
    (void)line;
}

#endif /* USE_FULL_ASSERT */