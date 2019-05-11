#include "keyboard.h"
#include "io.h"
#include "global.h"
#include "interrupt.h"
#include "print.h"

#define KBD_BUF_PORT 0x60

static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

static char Keyboard::keymap[0x3a][2] =
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

}


void keyboard_init(){
	put_str("keyboard init start\n");
	register_handler(0x21, intr_keyboard_handler);
	put_str("keyboard init done\n");
}

