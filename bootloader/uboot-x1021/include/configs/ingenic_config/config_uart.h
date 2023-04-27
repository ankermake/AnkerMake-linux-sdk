
#ifdef CONFIG_SPL_SERIAL_SUPPORT

#if CONFIG_SYS_UART_INDEX == 0
#define ARG_CONSOLE_TTY "console=ttyS0,"
#elif CONFIG_SYS_UART_INDEX == 1
#define ARG_CONSOLE_TTY "console=ttyS1,"
#elif CONFIG_SYS_UART_INDEX == 2
#define ARG_CONSOLE_TTY "console=ttyS2,"
#elif CONFIG_SYS_UART_INDEX == 3
#define ARG_CONSOLE_TTY "console=ttyS3,"
#elif CONFIG_SYS_UART_INDEX == 4
#define ARG_CONSOLE_TTY "console=ttyS4,"
#else
#error "please add more define here"
#endif

#if CONFIG_BAUDRATE == 115200
#define ARG_CONSOLE_RATE "115200n8"
#elif CONFIG_BAUDRATE == 3000000
#define ARG_CONSOLE_RATE "3000000n8"
#else
#error "please add more define here"
#endif

#define ARGS_CONSOLE ARG_CONSOLE_TTY ARG_CONSOLE_RATE

#endif /* CONFIG_SYS_UART_INDEX */

#ifndef CONFIG_SPL_SERIAL_SUPPORT
#define ARGS_CONSOLE "no_console"
#endif

#ifdef CONFIG_ARG_NO_CONSOLE
#undef ARGS_CONSOLE
#define ARGS_CONSOLE "no_console"
#endif
