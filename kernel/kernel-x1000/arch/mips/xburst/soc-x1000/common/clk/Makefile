ifeq ($(CONFIG_FPGA_TEST),y)
obj-y += clk_fpga.o
else
obj-y += clk.o clk_cgu.o clk_cgu_audio.o clk_cpccr.o clk_gate.o	clk_pll.o clk_wdt.o clk_softclk.o clk_rtc.o
endif

obj-y += clk_sources.o
