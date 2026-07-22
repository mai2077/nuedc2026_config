#ifndef BSP_IIC_H_
#define BSP_IIC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    IIC_OK = 0x00U,
    IIC_ERR = 0x01U,
    IIC_BUSY = 0x02U,
    IIC_TIMEOUT = 0x03U
} IIC_Status_t;

/* One register address plus at most seven data bytes fits the 8-byte FIFO. */
#define BSP_IIC_MAX_REGISTER_WRITE (7U)

typedef struct
{
    IIC_Status_t (*write)(uint16_t DevAddress,
                          uint8_t *pData,
                          uint16_t Size,
                          unsigned long Timeout);
    IIC_Status_t (*read)(uint16_t DevAddress,
                         uint8_t *pData,
                         uint16_t Size,
                         unsigned long Timeout);
    IIC_Status_t (*write_reg)(uint16_t DevAddress,
                              uint16_t MemAddress,
                              uint8_t *pData,
                              uint16_t Size,
                              unsigned long Timeout);
    IIC_Status_t (*read_reg)(uint16_t DevAddress,
                             uint16_t MemAddress,
                             uint8_t *pData,
                             uint16_t Size,
                             unsigned long Timeout);
    void (*delay_ms)(uint16_t ms);
} IICInterface_t;

typedef IICInterface_t *pIICInterface_t;

extern IICInterface_t UserII2Dev;

#ifdef __cplusplus
}
#endif

#endif /* BSP_IIC_H_ */
