/* Application example to detect button presses and releases on the PineNut
 * (https://pine64.com/product/pinenut-model01s-wifi-ble5-module/) */

extern "C" {
// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Standard input/output
#include <stdio.h>

// Hardware Abstraction Layers
#include <bl602_glb.h>
#include <bl_gpio.h>
#include <bl_irq.h>
#include <bl_timer.h>
#include <bl_uart.h>
#include <hal_gpio.h>
}

#include <etl/string.h>

/* GPIO pin of the button */
#define BUTTON_PIN 12

/* Variable to store the button press time */
static uint32_t begin = 0;

/* Function prototypes */
static void keepalive([[gnu::unused]] void *pvParameters);
static void handle_button_release([[gnu::unused]] void *arg);
static void handle_button_press([[gnu::unused]] void *arg);

/* Interrupt configuration to set after button press to detect release */
static auto interrupt_pos_pulse = gpio_ctx_t{
    .next = nullptr,                       /* No next interrupt */
    .gpio_handler = handle_button_release, /* Handle function */
    .arg = nullptr,                        /* Parameters to hand over */
    .gpioPin = BUTTON_PIN, /* Pin the interrupt should be configured for */
    .intCtrlMod = GPIO_INT_CONTROL_ASYNC, /* Interrupt mode */
    .intTrgMod =
        GLB_GPIO_INT_TRIG_POS_PULSE, /* Interrupt trigger (positive pulse)*/
};

/* Interrupt configuration to set after button release to detect press */
static auto interrupt_neg_pulse = gpio_ctx_t{
    .next = nullptr,                     /* No next interrupt */
    .gpio_handler = handle_button_press, /* Handle function */
    .arg = nullptr,                      /* Parameters to hand over */
    .gpioPin = BUTTON_PIN, /* Pin the interrupt should be configured for */
    .intCtrlMod = GPIO_INT_CONTROL_ASYNC, /* Interrupt mode */
    .intTrgMod =
        GLB_GPIO_INT_TRIG_NEG_PULSE, /* Interrupt trigger (negative pulse)*/
};

/* Button pressed -> negative pulse */
static void handle_button_press([[gnu::unused]] void *arg) {
  /* Mask pin to disable (and clear) interrupt */
  bl_gpio_intmask(BUTTON_PIN, 1);

  /* Get current time*/
  begin = bl_timer_now_us() / 1000;
  printf("Button pressed!\r\n");

  /* Set interrupt for button release*/
  bl_gpio_register(&interrupt_pos_pulse);
}

/* Button released -> positive pulse */
static void handle_button_release([[gnu::unused]] void *arg) {
  /* Mask pin to disable (and clear) interrupt */
  bl_gpio_intmask(BUTTON_PIN, 1);

  /* Compute button press time */
  auto end = bl_timer_now_us() / 1000;
  printf("Button released, was pressed for %ldms!\r\n", (end - begin));

  /* Set interrupt for button press */
  bl_gpio_register(&interrupt_neg_pulse);
}

extern "C" void bfl_main(void) {
  /* Initialize system */
  vInitializeBL602();

  /* Initialize UART
   * Ports: 16+7 (TX+RX)
   * Baudrate: 2 million
   */
  bl_uart_init(
      /* UART channel (0 = USB port)*/ 0,
      /* Transmission pin */ 16,
      /* Receive pin */ 7,
      /* Unused */ 255,
      /* Unused */ 255,
      /* Baud rate */ 2 * 1000 * 1000);

  /* Configure interrupt to detect button press */
  /* 1. Configure PIN 12 as input */
  bl_gpio_enable_input(BUTTON_PIN, /* Button pin */
                       1,          /* Enable pullup resistor */
                       0           /* No pulldown resistor */
  );

  /* 2. Set interrupt */
  bl_gpio_register(&interrupt_neg_pulse);

  /* Create and start task to keep the operating system alive */
  constexpr uint16_t STACK_SIZE = 128;
  constinit static StackType_t stack[STACK_SIZE]{};
  constinit static StaticTask_t task{};

  xTaskCreateStatic(keepalive, etl::string_view("keepalive").data(), STACK_SIZE,
                    nullptr, 15, stack, &task);

  /* Start task scheduler */
  vTaskStartScheduler();
}

/* Keep-alive task */
static void keepalive([[gnu::unused]] void *pvParameters) {
  while (1) {
    vTaskDelay(20000);
  }
  vTaskDelete(nullptr);
}
