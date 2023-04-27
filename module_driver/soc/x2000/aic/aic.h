#ifndef _AIC_H_
#define _AIC_H_

enum aic_clk_id {
    AIC_TX_MCLK,
    AIC_RX_MCLK,
};

enum aic_clk_dir {
    AIC_CLK_OUTPUT,
    AIC_CLK_NOT_OUTPUT,
};

/* set_dai_fmt */
#define AIC_FMT_RX (1 << 31)
#define AIC_FMT_TX (1 << 30)

void aic_mute_icodec(int is_mute);

#endif /* _AIC_H_ */
