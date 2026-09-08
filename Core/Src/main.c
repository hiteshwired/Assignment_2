/**
 * CPE 316 - Assignment 2: Keypad
 *
 * Reads a 4x3 matrix keypad using GPIO polling.
 * Displays a detected key value using the LEDs from A1.
 *
 * LEDs:
 *   PC0 - LED bit 0
 *   PC1 - LED bit 1
 *   PC2 - LED bit 2
 *
 * Keypad:
 *   PC4-PC7  - Rows 0-3
 *   PC8-PC10 - Columns 0-2
 */

#include "main.h"


/* Keypad dimensions */
#define NUM_OF_ROWS        4U
#define NUM_OF_COLS        3U

/* Return value when no key is pressed */
#define NO_KEY             (-1)

/* GPIO ports */
#define LED_PORT           GPIOC
#define KEYPAD_PORT        GPIOC
#define ROW_PORT           GPIOC
#define COL_PORT           GPIOC

/* A1 LEDs: PC0-PC2 */
#define LED_PINS           (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2)

/* Keypad rows: PC4-PC7 */
#define ROW_PINS           (GPIO_PIN_4 | GPIO_PIN_5 | \
                            GPIO_PIN_6 | GPIO_PIN_7)

#define ROW_FIRST_PIN      4U

/* Keypad columns: PC8-PC10 */
#define COL_PINS           (GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10)

#define COL_FIRST_PIN      8U

/* Optional values for the two non-numeric keys */
#define STAR_KEY           10
#define POUND_KEY          11

static const int8_t key_map[NUM_OF_ROWS][NUM_OF_COLS] = {
    {1, 2, 3}, 
    {4, 5, 6}, 
    {7, 8, 9}, 
    {STAR_KEY, 0, POUND_KEY}
};

void SystemClock_Config(void);

static void gpio_init(void);
static uint8_t read_rows(void);
static int8_t row_to_index(uint8_t row_bits);
static int8_t read_keypad(void);
static void display_key(uint8_t key);


/**
 * Main program.
 */
int main(void)
{
    int8_t key;

    HAL_Init();
    SystemClock_Config();

    gpio_init();

    while (1)
    {
        key = read_keypad();

        if (key != NO_KEY)
        {
            display_key((uint8_t)key);
        }
    }
}


/**
 * Configure LEDs and keypad GPIO pins.
 */
static void gpio_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

    /* PC0-PC2: LED outputs */
    LED_PORT->MODER &= ~(GPIO_MODER_MODE0 |
                         GPIO_MODER_MODE1 |
                         GPIO_MODER_MODE2);

    LED_PORT->MODER |= (GPIO_MODER_MODE0_0 |
                        GPIO_MODER_MODE1_0 |
                        GPIO_MODER_MODE2_0);

    LED_PORT->OTYPER &= ~(GPIO_OTYPER_OT0 |
                          GPIO_OTYPER_OT1 |
                          GPIO_OTYPER_OT2);

    LED_PORT->PUPDR &= ~(GPIO_PUPDR_PUPD0 |
                         GPIO_PUPDR_PUPD1 |
                         GPIO_PUPDR_PUPD2);

    LED_PORT->BRR = LED_PINS;


    /* PC4-PC7: keypad row inputs */
    ROW_PORT->MODER &= ~(GPIO_MODER_MODE4 |
                         GPIO_MODER_MODE5 |
                         GPIO_MODER_MODE6 |
                         GPIO_MODER_MODE7);

    ROW_PORT->PUPDR &= ~(GPIO_PUPDR_PUPD4 |
                         GPIO_PUPDR_PUPD5 |
                         GPIO_PUPDR_PUPD6 |
                         GPIO_PUPDR_PUPD7);

    /*
     * Pull-down = 10 in each PUPDR field.
     * Set the upper bit of each two-bit field.
     */
    ROW_PORT->PUPDR |= (GPIO_PUPDR_PUPD4_1 |
                        GPIO_PUPDR_PUPD5_1 |
                        GPIO_PUPDR_PUPD6_1 |
                        GPIO_PUPDR_PUPD7_1);


    /* PC8-PC10: keypad column outputs */
    COL_PORT->MODER &= ~(GPIO_MODER_MODE8 |
                         GPIO_MODER_MODE9 |
                         GPIO_MODER_MODE10);

    COL_PORT->MODER |= (GPIO_MODER_MODE8_0 |
                        GPIO_MODER_MODE9_0 |
                        GPIO_MODER_MODE10_0);

    COL_PORT->OTYPER &= ~(GPIO_OTYPER_OT8 |
                          GPIO_OTYPER_OT9 |
                          GPIO_OTYPER_OT10);

    COL_PORT->PUPDR &= ~(GPIO_PUPDR_PUPD8 |
                         GPIO_PUPDR_PUPD9 |
                         GPIO_PUPDR_PUPD10);

    /* Columns start low */
    COL_PORT->BRR = COL_PINS;
}


/**
 * Read PC4-PC7 and shift the rows down to bits 0-3.
 */
static uint8_t read_rows(void)
{
    uint8_t rows;

    rows = (uint8_t)((ROW_PORT->IDR & ROW_PINS) >>
                     ROW_FIRST_PIN);

    return rows;
}


/**
 * Convert a one-hot row reading into row number 0-3.
 */
static int8_t row_to_index(uint8_t row_bits)
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
 * Poll the keypad and return the detected key.
 *
 * Returns NO_KEY when no button is pressed.
 */
static int8_t read_keypad(void)
{
    uint8_t column;
    uint8_t row_bits;
    int8_t row;

    /* Stage 1: drive all columns high */
    COL_PORT->BSRR = COL_PINS;

    row_bits = read_rows();

    if (row_bits == 0U)
    {
        COL_PORT->BRR = COL_PINS;
        return NO_KEY;
    }

    /* Stage 2: test one column at a time */
    COL_PORT->BRR = COL_PINS;

    for (column = 0U; column < NUM_OF_COLS; column++)
    {
        COL_PORT->BSRR =
            (1UL << (COL_FIRST_PIN + column));

        row_bits = read_rows();

        COL_PORT->BRR =
            (1UL << (COL_FIRST_PIN + column));

        if (row_bits != 0U)
        {
            row = row_to_index(row_bits);

            if (row != NO_KEY)
            {
              return key_map[row][column];
            }
        }
    }

    return NO_KEY;
}


/**
 * Display a key value on the three A1 LEDs.
 */
static void display_key(uint8_t key)
{
    uint8_t led_value;

    /*
     * TODO:
     * Decide how the assignment should represent keys
     * greater than 7 on only three A1 LEDs.
     *
     * For initial testing, values 0-7 can be displayed
     * directly.
     */

    if (key <= 7U)
    {
        led_value = key;

        LED_PORT->BRR = LED_PINS;
        LED_PORT->BSRR = led_value;
    }
}


/**
 * Configure the system clock.
 *
 * This is ST-generated code and can remain unchanged.
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    if (HAL_PWREx_ControlVoltageScaling(
            PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        Error_Handler();
    }

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
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

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

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                            FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
 * Stop execution if clock configuration fails.
 */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}