#include "keyboard.h"
#include "io.h"
#include "global.h"
#include "interrupt.h"
#include "print.h"

#define KBD_BUF_PORT 0x60
#define EXT_CODE 0xe0

static bool ctrl_status, shift_status, alt_status, caps_lock_status;
//只支持主键盘
static char keymap[0x3a][2] =
{
    /*00*/{0x0, 0x0}, {0x0, 0x0}, {'1', '!'}, {'2', '@'}, 
    /*04*/{'3', '#'}, {'4', '$'}, {'5', '%'}, {'6', '^'}, 
    /*08*/{'7', '&'}, {'8', '*'}, {'9', '('}, {'0', ')'},
    /*0c*/{'-', '_'}, {'=', '+'}, {'\b','\b'},{'\t','\t'},
    /*10*/{'q', 'Q'}, {'w', 'W'}, {'e', 'E'}, {'r', 'R'},
    /*14*/{'t', 'T'}, {'y', 'Y'}, {'u', 'U'}, {'i', 'I'},
    /*18*/{'o', 'O'}, {'p', 'P'}, {'[', '{'}, {']', '}'},
    /*1c*/{'\n','\n'},{0x0, 0x0}, {'a', 'A'}, {'s', 'S'},
    /*20*/{'d', 'D'}, {'f', 'F'}, {'g', 'G'}, {'h', 'H'},
    /*24*/{'j', 'J'}, {'k', 'K'}, {'l', 'L'}, {';', ':'},
    /*28*/{'\'','\"'},{'`', '~'}, {0x0, 0x0}, {'\\','|'}, 
    /*2c*/{'z', 'Z'}, {'x', 'X'}, {'c', 'C'}, {'v', 'V'}, 
    /*30*/{'b', 'B'}, {'n', 'N'}, {'m', 'M'}, {',', '<'},
    /*34*/{'.', '>'}, {'/', '?'}, {0x0, 0x0}, {'*', '*'},
    /*38*/{0x0, 0x0}, {' ', ' '}
};


static void intr_keyboard_handler(void){
	uint8_t scancode = inb(KBD_BUF_PORT);
	if(scancode == EXT_CODE){
		//主键盘扩展码为两个字节, 只需要再读一个字节
		scancode = inb(KBD_BUF_PORT);
	}
	switch(scancode){
		//左右shift通码及断码
		case 0x2a:
		case 0x36:
		case 0xaa:
		case 0xb6:
			shift_status ^= 1;
			return;
		//左右alt通码及断码
		case 0x38:
		case 0xb8:
			alt_status ^= 1;
			return;
		//左右ctrl通码及断码
		case 0x1d:
		case 0x9d:
			ctrl_status ^= 1;
			return;
		//大写锁只考虑通码
		case 0x3a:
			caps_lock_status ^= 1;
			return;
	}
	//其余断码不处理
	if(scancode & 0x80){
		return;
	}

	bool shift = false;
	if(shift_status && caps_lock_status){
		shift = false;
	}else if(shift_status || caps_lock_status){
		shift = true;
	}

	char ch = keymap[scancode][shift];
	//ascii 为0不处理
	if(ch){
		//put_char(ch);
	}
}



void keyboard_init(){
	printk("keyboard init start\n");
	register_handler(0x21, intr_keyboard_handler);
	printk("keyboard init done\n");
}

