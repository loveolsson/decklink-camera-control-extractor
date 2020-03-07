#include "esp32_defines.h"

static const gpio_num_t dipPins[] = {DIP_PIN_1, DIP_PIN_2, DIP_PIN_3, DIP_PIN_4};

static void Pin(gpio_num_t pin, gpio_mode_t dir, gpio_pullup_t pullup, uint32_t state)
{
  uint64_t bitMask = ((uint64_t)1) << pin;

  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = dir;
  io_conf.pin_bit_mask = bitMask;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = pullup;

  ESP_ERROR_CHECK(gpio_config(&io_conf));

  if (dir == GPIO_MODE_OUTPUT)
  {
    gpio_set_level(pin, state);
  }
}

void SetupPins()
{
  for (int i = 0; i < NUM(dipPins); ++i)
  {
    Pin(dipPins[i], GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, 0);
  }

  Pin(RS422_DE, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, 0);
  Pin(RS422_RE, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, 1);

  Pin(TWI_EN, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, 1);
  Pin(LED_ERR, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, 1);

  Pin(OPTO_O1, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, 1);
  Pin(OPTO_O2, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, 1);
}

uint8_t ReadDips()
{
  uint8_t res = 0;

  for (int i = 0; i < NUM(dipPins); ++i)
  {
    res |= (!gpio_get_level(dipPins[i])) << i;
  }

  return res;
}
