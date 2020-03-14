#pragma once

#include <pthread.h>
#include <stdint.h>
//#include <endian.h>

#include <BMDSDIControl.h>

#ifdef ARDUINO_ARCH_ESP32
#include "driver/i2c.h"
#endif

#define I2C_FREQ_HZ 400000  //!< I2C master clock frequency

#if 0
template <typename T>
static inline T EndianSwap(T& in) {
  T res;

  for (int i = 0; i < sizeof(T); ++i) {
    ((uint8_t*)&res)[i] = ((uint8_t*)&in)[sizeof(T) - i - 1];
  }

  return res;
}
#else
#define EndianSwap(T) (T)
#endif

static bool I2CInitialized = false;

static void
RunInit()
{
    i2c_config_t conf;
    conf.mode          = I2C_MODE_MASTER;
    conf.sda_io_num    = TWI_DA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num    = TWI_CL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed =
        I2C_FREQ_HZ;  //I2C frequency is the clock speed for a complete high low clock sequence
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));

    int timeout;
    i2c_get_timeout(I2C_NUM_0, &timeout);
    i2c_set_timeout(I2C_NUM_0, timeout * 100);
    I2CInitialized = true;
}

template <typename T>
class I2CCustom : public T
{
public:
    I2CCustom(int wireAddress)
        : m_wireAddress(wireAddress)
    {
    }

    // Inherits from T
    virtual void begin();
    virtual uint32_t regRead32(uint16_t address) const;
    virtual void regWrite32(uint16_t address, uint32_t value) const;
    virtual uint16_t regRead16(uint16_t address) const;
    virtual void regWrite16(uint16_t address, uint16_t value) const;
    virtual uint8_t regRead8(uint16_t address) const;
    virtual void regWrite8(uint16_t address, uint8_t value) const;
    virtual void regRead(uint16_t address, uint8_t values[], int length) const;
    virtual void regWrite(uint16_t address, const uint8_t values[], int length) const;

private:
    int m_wireAddress;
};

template <typename T>
void
I2CCustom<T>::begin()
{
    if (!I2CInitialized) {
        RunInit();
    }

    T::begin();
}

template <typename T>
uint32_t
I2CCustom<T>::regRead32(uint16_t address) const
{
    uint32_t reg;
    regRead(address, (uint8_t *)&reg, sizeof(uint32_t));
    return EndianSwap(reg);
}

template <typename T>
void
I2CCustom<T>::regWrite32(uint16_t address, uint32_t value) const
{
    auto data = EndianSwap(value);
    regWrite(address, (uint8_t *)&data, sizeof(uint32_t));
}

template <typename T>
uint16_t
I2CCustom<T>::regRead16(uint16_t address) const
{
    uint16_t reg;
    regRead(address, (uint8_t *)&reg, sizeof(uint16_t));
    return EndianSwap(reg);
}

template <typename T>
void
I2CCustom<T>::regWrite16(uint16_t address, uint16_t value) const
{
    auto data = EndianSwap(value);
    regWrite(address, (uint8_t *)&data, 2);
}

template <typename T>
uint8_t
I2CCustom<T>::regRead8(uint16_t address) const
{
    uint8_t res;
    regRead(address, &res, sizeof(uint8_t));
    return res;
}

template <typename T>
void
I2CCustom<T>::regWrite8(uint16_t address, uint8_t value) const
{
    regWrite(address, &value, sizeof(uint8_t));
}

template <typename T>
void
I2CCustom<T>::regRead(uint16_t address, uint8_t values[], int length) const
{
    if (length <= 0) {
        return;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Start the message and write the address
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (m_wireAddress << 1) | I2C_MASTER_WRITE, true));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, address & 0xFF, true));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (address >> 8) & 0xFF, true));

    // Restart the message and do the read
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (m_wireAddress << 1) | I2C_MASTER_READ, true));
    ESP_ERROR_CHECK(i2c_master_read(cmd, values, length, I2C_MASTER_LAST_NACK));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    //Send queued commands
    i2c_master_cmd_begin(
        I2C_NUM_0, cmd,
        (1000 /
         portTICK_RATE_MS));  // Not error checked since timeout is a valid outcome during startup
    i2c_cmd_link_delete(cmd);
}

template <typename T>
void
I2CCustom<T>::regWrite(uint16_t address, const uint8_t values[], int length) const
{
    if (length <= 0) {
        return;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Start the message and write the address and data
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (m_wireAddress << 1) | I2C_MASTER_WRITE, true));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, address & 0xFF, true));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (address >> 8) & 0xFF, true));
    ESP_ERROR_CHECK(i2c_master_write(cmd, (uint8_t *)values, length, true));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    //Send queued commands
    i2c_master_cmd_begin(
        I2C_NUM_0, cmd,
        (1000 /
         portTICK_RATE_MS));  // Not error checked since timeout is a valid outcome during startup
    i2c_cmd_link_delete(cmd);
}
