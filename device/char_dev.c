#include <global.h>
#include <stdint.h>
#include <keyboard.h>
#include <tty.h>
#include <char_dev.h>


chdev_readf chdev_rdtlb[7] = {

	kb_read, NULL, NULL, NULL, NULL, NULL, NULL,
};

chdev_writef chdev_wrtlb[7] = {

	terminal_write, NULL, NULL, NULL, NULL, NULL, NULL,
};




