/*******************************************************************************
* Copyright (C) 2019 Maxim Integrated Products, Inc., All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
*******************************************************************************
*/
#include "max31875.h"
#include "max31875_c.h"
#include "nrf_drv_twi.h"
#include "app_config.h"
#include "sdk_config.h"
#include "nrf_log.h"

/******************************************************************************/
/*  C version for MAX31875 driver                                             */
/******************************************************************************/

/* @var max31875_write_address, max31875_read_address
 * @brief I2C address
 */
uint8_t max31875_write_address, max31875_read_address;
/* @var max31875_write_address, max31875_read_address
 * @brief max31875_extended_format
 */
uint32_t max31875_extended_format;


#define TWI_INSTANCE_ID     0

static volatile bool m_twi_xfer_done = false;

static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
	  switch (p_event->type)
    {
        
            case NRF_DRV_TWI_EVT_DONE:
            m_twi_xfer_done = true;
            break;
        default:
            break;
    }
}

void twi_master_init(void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
       .scl                = TWI_SCL_M,  
       .sda                = TWI_SDA_M,  
       .frequency          = NRF_DRV_TWI_FREQ_100K, 
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH, 
       .clear_bus_init     = false
    };
    
    err_code = nrf_drv_twi_init(&m_twi, &twi_config, twi_handler, NULL);
	
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_twi_enable(&m_twi);
}


//int max31875_init(uint8_t slave_address)
//{
//    max31875_write_address = (slave_address <<1);
//    max31875_read_address = ((slave_address << 1) | 1);
//    max31875_extended_format = 0;
//    return MAX31875_NO_ERROR;
//}
 

/******************************************************************************/
int max31875_read_reg(uint16_t *value, uint8_t reg) 
{
    ret_code_t err_code;
    uint8_t data[2] = {0, 0};
    max31875_raw_data tmp;
     
    if (reg <= MAX31875_REG_MAX) {
        /* write to the Register Select */
        err_code = nrf_drv_twi_tx(&m_twi, MAX31875_I2C_SLAVE_ADR_R0, &reg, 1, true);
      //  ret = i2c_bus.write(max31875_write_address, &reg, 1, true);
        /* read the two bytes of data */
        if (err_code == NRF_SUCCESS) {  
            err_code = nrf_drv_twi_rx(&m_twi, MAX31875_I2C_SLAVE_ADR_R0, data, 2);
            if (err_code == NRF_SUCCESS) {
                tmp.msb = data[0];
                tmp.lsb = data[1];
                *value = tmp.uwrd;
                return MAX31875_NO_ERROR;
            } else {
                //SEGGER_RTT_printf(0,"failed to read data");
            }
        } else {                
            //SEGGER_RTT_printf(0,"failed to write data");
        }
    } else {
        //SEGGER_RTT_printf(0,"%s: register address is not correct");
    }                
    return MAX31875_ERROR;
}

float max31875_read_reg_as_temperature(uint8_t reg)
{
    max31875_raw_data tmp;
    float temperature;
    if (reg == MAX31875_REG_TEMPERATURE ||
        reg == MAX31875_REG_THYST_LOW_TRIP || reg == MAX31875_REG_TOS_HIGH_TRIP) {
        max31875_read_reg(&tmp.uwrd, reg);
        temperature = (float)tmp.magnitude_bits;
        if (max31875_extended_format)
            temperature *= MAX31875_CF_EXTENDED_FORMAT;
        else
            temperature *= MAX31875_CF_NORMAL_FORMAT;
        if (tmp.sign_bit)
            temperature = -temperature;
        return temperature;

        return temperature;
    } else {
        //SEGGER_RTT_printf(0,"%s: register is invalid, %d r\n", __func__, reg);
        return 0;
    }
}


/******************************************************************************/
int max31875_write_reg(uint16_t value, uint8_t reg) 
{
    ret_code_t err_code;
    uint8_t cmd[3];
    max31875_raw_data tmp;

    if (reg >= MAX31875_REG_CONFIGURATION && reg <= MAX31875_REG_MAX) {
        cmd[0] = reg;
        tmp.uwrd = value;
        cmd[1] = tmp.msb;
        cmd[2] = tmp.lsb;
        err_code = nrf_drv_twi_tx(&m_twi, MAX31875_I2C_SLAVE_ADR_R0, cmd, 1, true);

        if (err_code == NRF_SUCCESS) {
            if (MAX31875_REG_CONFIGURATION == reg) {
                max31875_extended_format = 0;
                if (tmp.uwrd & MAX31875_CFG_EXTENDED_FORMAT)
                    max31875_extended_format = 1;
            }
            return MAX31875_NO_ERROR;
        } else {
            //SEGGER_RTT_printf(0,"%s: I2C write error %d\r\n",__func__, err_code);
            return MAX31875_ERROR;
        }
    } else {
        //SEGGER_RTT_printf(0,"%s: register value invalid %x\r\n",__func__, reg);
        return MAX31875_ERROR;
    }
}


int max31875_write_cfg(uint16_t cfg)
{
    return max31875_write_reg(cfg, MAX31875_REG_CONFIGURATION);
}


int max31875_write_trip_low(float temperature)
{
    max31875_raw_data raw;
    raw.uwrd = 0;
    if (temperature < 0) {
        raw.sign_bit = 1;
        temperature = -temperature;
    }
    if (max31875_extended_format)
        temperature /= MAX31875_CF_EXTENDED_FORMAT;
    else
        temperature /= MAX31875_CF_NORMAL_FORMAT;
    raw.magnitude_bits = uint16_t(temperature);
    return max31875_write_reg(raw.uwrd, MAX31875_REG_THYST_LOW_TRIP);
}


int max31875_write_trip_high(float temperature)
{
    max31875_raw_data raw;
    raw.uwrd = 0;
    if (temperature < 0) {
        raw.sign_bit = 1;
        temperature = -temperature;
    }
    if (max31875_extended_format)
        temperature /= MAX31875_CF_EXTENDED_FORMAT;
    else
        temperature /= MAX31875_CF_NORMAL_FORMAT;
    raw.magnitude_bits = uint16_t(temperature);
    return max31875_write_reg(raw.uwrd, MAX31875_REG_TOS_HIGH_TRIP);
}


float max31875_celsius_to_fahrenheit(float temp_c)
{
    float temp_f;
    temp_f = ((temp_c * 9)/5) + 32;
    return temp_f;
}