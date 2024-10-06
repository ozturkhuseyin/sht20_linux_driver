#include "sht20_driver.h"


int sht20_read_temperature(struct i2c_client *client, int *buff)
{
    /* i2c tranfer status */
    int status = 0;
    
    /* Store temperature */
    int temperature = 0;

    uint8_t tmp[3] = {0};
    uint8_t buf[3] = {0};

    /* If send fault, status less than zero. */
    status = i2c_smbus_write_byte(client, SHT20_TEMPERATURE_NO_HOLD_ADDRESS);
    if(status < 0) {
        return -1;
    }
    
    /**
     *  Follow spec.
     *  14 bits must be delay 85 ms.
     */
    msleep(85);

    /* If receive fault, status less than zero. */
    status = i2c_master_recv(client, tmp, 3);
    if(status < 0) {
        return -1;
    }

    temperature = (tmp[0] << 8) | (tmp[1] &0xFC);
    temperature = -4685 + ((17572 * temperature) >> 16);
    buf[0] = temperature >> 8;
    buf[1] = temperature & 0xFF;

    *buff = temperature;
    
    //pr_info("sht20_read_temperature read %d", temperature);

    return 0;   
}

int sht20_read_hum(struct i2c_client *client, int *buff)
{



    return 0;
}