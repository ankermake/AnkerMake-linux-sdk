#ifndef __ASM_H__
#define __ASM_H__
/*
 *  * END - mark end of function
 *   */
#define END(function)                                   \
	.end    function;                       \
	.size   function, .-function


#endif
