//
// Created by Hessian on 2023/3/29.
//

#include "utils.h"
#include <Wire.h>


int i2cScan(TwoWire *twoWire, char *result)
{
    byte error, address;
    int nDevices;

    char * pbuf = result;

    nDevices = 0;
    for(address = 1; address < 127; address++ )
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        twoWire->beginTransmission(address);
        error = twoWire->endTransmission();

        if (error == 0)
        {
            pbuf += sprintf(pbuf, "0x");
            if (address<16)
                pbuf += sprintf(pbuf, "0");
            pbuf += sprintf(pbuf, "%x found!\n", address);

            nDevices++;
        }
        else if (error==4)
        {
            pbuf += sprintf(pbuf, "0x");
            if (address<16)
                pbuf += sprintf(pbuf, "0");
            pbuf += sprintf(pbuf, "%x unknown!\n", address);
        }
    }
    if (nDevices == 0)
        pbuf += sprintf(pbuf, "No I2C devices found !\n");
    else
        pbuf += sprintf(pbuf, "done !\n");

    return pbuf - result;
}
