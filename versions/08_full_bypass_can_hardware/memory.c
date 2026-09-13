#include <stddef.h>
#include <stdint.h>

/* 裸机固件不链接C库，提供libcanard和DSDL生成代码需要的内存函数。 */
void *memset(void *destination, int value, size_t length)
{
    uint8_t *output = (uint8_t *)destination;
    while (length-- > 0U) {
        *output++ = (uint8_t)value;
    }
    return destination;
}

void *memcpy(void *destination, const void *source, size_t length)
{
    uint8_t *output = (uint8_t *)destination;
    const uint8_t *input = (const uint8_t *)source;
    while (length-- > 0U) {
        *output++ = *input++;
    }
    return destination;
}
