#include <stddef.h>
#include <stdint.h>

/* 裸机不链接C库，仅提供canard_stm32底层需要的memset。 */
void *memset(void *destination, int value, size_t length)
{
    uint8_t *output = (uint8_t *)destination;
    while (length-- > 0U) {
        *output++ = (uint8_t)value;
    }
    return destination;
}
