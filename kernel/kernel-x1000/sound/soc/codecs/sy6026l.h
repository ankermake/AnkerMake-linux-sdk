#ifndef __SY6026L_REG_H__
#define __SY6026L_REG_H__

#define DEV_ID          0x1
#define SOFT_MUTE       0x6
#define MASTER_VOLUME   0x7
#define I2S_CONTROL     0x15

/*I2S CONTROL*/
#define I2S_CTRL_SCLK_INV       BIT(6)
#define I2S_CTRL_LR_POL         BIT(5)
#define I2S_CTRL_ENABLE         BIT(4)
#define I2S_CTRL_I2S_MODE       (0x0 << 2)
#define I2S_CTRL_LJ_MODE        (0x1 << 2)
#define I2S_CTRL_RJ_MODE        (0x2 << 2)
#define I2S_CTRL_VBITS_24       (0x0)
#define I2S_CTRL_VBITS_20       (0x1)
#define I2S_CTRL_VBITS_18       (0x2)
#define I2S_CTRL_VBITS_16       (0x3)
#endif /*__SY6026L_REG_H__*/
