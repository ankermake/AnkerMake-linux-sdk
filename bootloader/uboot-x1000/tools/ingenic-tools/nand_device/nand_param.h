#ifndef __NAND_PARAM_H
#define __NAND_PARAM_H
int ato_nand_register_func(void);
int dosilicon_nand_register_func(void);
int foresee_nand_register_func(void);
int gd_nand_register_func(void);
int mxic_nand_register_func(void);
int winbond_nand_register_func(void);
int xtx_nand_register_func(void);
int zetta_nand_register_func(void);
static int (*nand_param[])(void) = {
/*##################*/
ato_nand_register_func,
/*##################*/
/*##################*/
dosilicon_nand_register_func,
/*##################*/
/*##################*/
foresee_nand_register_func,
/*##################*/
/*##################*/
gd_nand_register_func,
/*##################*/
/*##################*/
mxic_nand_register_func,
/*##################*/
/*##################*/
winbond_nand_register_func,
/*##################*/
/*##################*/
xtx_nand_register_func,
/*##################*/
/*##################*/
zetta_nand_register_func,
/*##################*/
};
#endif
