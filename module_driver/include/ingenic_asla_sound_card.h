#ifndef _MD_ASLA_SND_CARD_H_
#define _MD_ASLA_SND_CARD_H_

enum select_codec {
    SELECT_EXT_CODEC,    /* 选择外部codec */
    SELECT_INNER_CODEC,  /* 选择内部codec */
    SELECT_ENABLE_MCLK,  /* 选择使能mclk */
    SELECT_DISABLE_MCLK, /* 选择失能mclk */
};

#endif /* _MD_ASLA_SND_CARD_H_ */