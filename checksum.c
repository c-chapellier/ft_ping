#include "ft_ping.h"

uint16_t checksum(uint16_t *data, int data_len)
{
    uint32_t sum = 0;
    uint16_t checksum = 0;

    for (int i = 0; i < (data_len + 1) / 2; ++i)
    {
        sum += data[i];
        if (sum > UINT16_MAX)
        {
            sum -= UINT16_MAX + 1;
            ++sum;
        }
    }
    checksum = sum;
    return (~checksum);
}
