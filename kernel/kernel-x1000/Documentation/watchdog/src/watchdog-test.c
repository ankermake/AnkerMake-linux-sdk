/*
 * Watchdog Driver Test Program
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <unistd.h>

int fd;

/*
 * This function simply sends an IOCTL to the driver, which in turn ticks
 * the PC Watchdog card to reset its internal timer so it doesn't trigger
 * a computer reset.
 */
static void keep_alive(void)
{
    int dummy;

    ioctl(fd, WDIOC_KEEPALIVE, &dummy);
}

static void term(int sig)
{
	int flags;

	flags = WDIOS_DISABLECARD;
	ioctl(fd, WDIOC_SETOPTIONS, &flags);

    close(fd);
    fprintf(stderr, "Stopping watchdog ticks...\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    int flags;

    fd = open("/dev/watchdog", O_WRONLY);

    if (fd == -1) {
		fprintf(stderr, "Watchdog device not enabled.\n");
		fflush(stderr);
		exit(-1);
    }

	flags = WDIOS_ENABLECARD;
	ioctl(fd, WDIOC_SETOPTIONS, &flags);

	int timeout = 1;
	ioctl(fd, WDIOC_SETTIMEOUT, &timeout);

	/*
	 * 程序被中断之后关闭watchdog
	 */
	signal(SIGINT, term);

    while(1) {
		keep_alive();
		usleep(200*1000);
    }

	flags = WDIOS_DISABLECARD;
	ioctl(fd, WDIOC_SETOPTIONS, &flags);

end:
    close(fd);
    return 0;
}
