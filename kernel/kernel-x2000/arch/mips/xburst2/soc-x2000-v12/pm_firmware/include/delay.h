static inline void udelay(int d)
{
	do{
		__asm__ __volatile__ ("nop \n\t");
		d -= (1000 + 23)/ 24 * 2;
	}while(d > 0);

}
