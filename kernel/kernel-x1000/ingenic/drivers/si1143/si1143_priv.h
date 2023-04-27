#define SI1143_PART_ID          0x43
#define SI1142_PART_ID          0x42

#define REG_PART_ID		0x00
#define REG_REV_ID		0x01
#define REG_SEQ_ID		0x02
#define REG_INT_CFG		0x03
#define REG_IRQ_ENABLE		0x04
#define REG_IRQ_MODE		0x05
#define REG_HW_KEY		0x07
#define REG_MEAS_RATE		0x08
#define REG_PS_RATE         0x0A
#define REG_PS_LED21		0x0f
#define REG_PS_LED3		0x10
#define REG_PS1_TH0		0x11
#define REG_PS1_TH1     0x12
#define REG_PS2_TH0		0x13
#define REG_PS2_TH1		0x14
#define REG_UCOEF1		0x13
#define REG_UCOEF2		0x14
#define REG_UCOEF3		0x15
#define REG_UCOEF4		0x16
#define REG_PARAM_WR		0x17
#define REG_COMMAND         0x18
#define REG_RESPONSE		0x20
#define REG_IRQ_STATUS		0x21
#define REG_ALSVIS_DATA		0x22
#define REG_ALSIR_DATA		0x24
#define REG_PS1_DATA		0x26
#define REG_PS2_DATA		0x28
#define REG_PS3_DATA		0x2a
#define REG_AUX_DATA		0x2c
#define REG_PARAM_RD		0x2e
#define REG_CHIP_STAT		0x30

#define UCOEF1_DEFAULT		0x7b
#define UCOEF2_DEFAULT		0x6b
#define UCOEF3_DEFAULT		0x01
#define UCOEF4_DEFAULT		0x00

/* Helper to figure out PS_LED register / shift per channel */
#define PS_LED_REG(ch) \
	(((ch) == 2) ? REG_PS_LED3 : REG_PS_LED21)
#define PS_LED_SHIFT(ch) \
	(((ch) == 1) ? 4 : 0)

/* Parameter offsets */
#define PARAM_CHLIST		    0x01
#define PARAM_PSLED12_SELECT	0x02
#define PARAM_PSLED3_SELECT	0x03
#define PARAM_PS_ENCODING	0x05
#define PARAM_ALS_ENCODING	0x06
#define PARAM_PS1_ADC_MUX	0x07
#define PARAM_PS2_ADC_MUX	0x08
#define PARAM_PS3_ADC_MUX	0x09
#define PARAM_PS_ADC_COUNTER	0x0a
#define PARAM_PS_ADC_GAIN	0x0b
#define PARAM_PS_ADC_MISC	0x0c
#define PARAM_ALS_ADC_MUX	0x0d
#define PARAM_ALSIR_ADC_MUX	0x0e
#define PARAM_AUX_ADC_MUX	0x0f
#define PARAM_ALSVIS_ADC_COUNTER	0x10
#define PARAM_ALSVIS_ADC_GAIN	0x11
#define PARAM_ALSVIS_ADC_MISC	0x12
#define PARAM_LED_RECOVERY	0x1c
#define PARAM_ALSIR_ADC_COUNTER	0x1d
#define PARAM_ALSIR_ADC_GAIN	0x1e
#define PARAM_ALSIR_ADC_MISC	0x1f
#define PARAM_ADC_OFFSET		0x1a

/* Channel enable masks for CHLIST parameter */
#define CHLIST_EN_PS1		BIT(0)
#define CHLIST_EN_PS2		BIT(1)
#define CHLIST_EN_PS3		BIT(2)
#define CHLIST_EN_ALSVIS		BIT(4)
#define CHLIST_EN_ALSIR		BIT(5)
#define CHLIST_EN_AUX		BIT(6)
#define CHLIST_EN_UV		    BIT(7)

/* Proximity measurement mode for ADC_MISC parameter */
#define PS_ADC_MODE_NORMAL	BIT(2)
/* Signal range mask for ADC_MISC parameter */
#define ADC_MISC_RANGE		BIT(5)

/* Commands for REG_COMMAND */
#define CMD_NOP			    0x00
#define CMD_RESET		    0x01
#define CMD_PS_FORCE		0x05
#define CMD_ALS_FORCE		0x06
#define CMD_PSALS_FORCE		0x07
#define CMD_PS_PAUSE		0x09
#define CMD_ALS_PAUSE		0x0a
#define CMD_PSALS_PAUSE		0x0b
#define CMD_PS_AUTO		    0x0d
#define CMD_ALS_AUTO		0x0e
#define CMD_PSALS_AUTO		0x0f
#define CMD_PARAM_QUERY		0x80
#define CMD_PARAM_SET		0xa0

#define RSP_INVALID_SETTING	0x80
#define RSP_COUNTER_MASK		0x0F

/* Minimum sleep after each command to ensure it's received */
#define MINSLEEP_MS	5
/* Return -ETIMEDOUT after this long */
#define TIMEOUT_MS	25
#define SAMPLE_PEROID_MS 50


/* Interrupt configuration masks for INT_CFG register */
#define INT_CFG_OE		    BIT(0) /* enable interrupt */
#define INT_CFG_MODE		    BIT(1) /* auto reset interrupt pin */

/* Interrupt enable masks for IRQ_ENABLE register */
#define MASK_ALL_IE		    (BIT(4) | BIT(3) | BIT(2) | BIT(0))

#define MUX_TEMP			    0x65
#define MUX_VDD			    0x75

/* Proximity LED current; see Table 2 in datasheet */
#define LED_CURRENT_45mA		0x04
