#include "vga.h"
#include "tty.h"
#include "stdint.h"
#include "string.h"
#include "sync.h"
#include "io.h"

static const uint32_t VGA_WIDTH = 80;  
static const uint32_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xc00B8000;

/// 默认颜色
static uint8_t default_color = VGA_COLOR_LIGHT_GREY | VGA_COLOR_BLACK << 4;
static uint32_t terminal_row;   ///< 当前行
static uint32_t terminal_column; ///< 当前列
static uint8_t terminal_color;   ///< 当前颜色
static struct mutex_lock tty_lock;  ///< 锁
static uint16_t* terminal_buffer;

static const uint32_t tab4 = 4;
//static const uint32_t tab8 = 8;

/** 光标寄存器读写端口 */
#define CUR_PORT(x) ((x) + 0x3d4)
/** 光标值高位指令 */
#define CUR_POS_HIGH 0x0e
/** 光标值低位指令 */
#define CUR_POS_LOW  0x0f

/**
 * 获取当前光标位置
 */
uint16_t get_cursor_pos(void){

	uint16_t pos = 0;
	outb(CUR_PORT(0), CUR_POS_LOW);
	pos |= inb(CUR_PORT(1));
	outb(CUR_PORT(0), CUR_POS_HIGH);
	pos |= ((uint16_t)inb(CUR_PORT(1))) << 8;
	return pos;
}

/**
 * 设置光标位置
 */
void update_cursor(uint32_t x, uint32_t y){

	uint16_t pos = y * VGA_WIDTH + x;
	outb(CUR_PORT(0), CUR_POS_LOW);
	outb(CUR_PORT(1), (uint8_t)(pos & 0xff));
	outb(CUR_PORT(0), CUR_POS_HIGH);
	outb(CUR_PORT(1), (uint8_t)((pos >> 8) & 0xff));
}

/**
 * 终端初始化
 */
void terminal_init(void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = default_color;
	terminal_buffer = VGA_MEMORY;
	terminal_clear();
	mutex_lock_init(&tty_lock);
}

/**
 * 设置颜色
 */
void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

/**
 * 定位
 */
void terminal_putentryat(unsigned char c, uint8_t color, uint32_t x, uint32_t y) {
	const uint32_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

/**
 * 清屏
 */
void terminal_clear() {
	uint32_t x, y;
	for (y = 0; y < VGA_HEIGHT; y++) {
		for (x = 0; x < VGA_WIDTH; x++) {
			const uint32_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

/**
 * 覆盖式滚屏
 */
void terminal_roll(){
	uint32_t x, y;
	for (y = 0; y < VGA_HEIGHT - 1; y++) {
		for (x = 0; x < VGA_WIDTH; x++) {
			const uint32_t pre = y * VGA_WIDTH + x;
			const uint32_t cur = (y + 1) * VGA_WIDTH + x;
			terminal_buffer[pre] = terminal_buffer[cur];
		}
	}
	//最后一行清空
	for(x = 0; x < VGA_WIDTH; x++){
		terminal_putentryat(' ', terminal_color, x, y);
	}
}

/**
 * 输出字符
 */
void terminal_putchar(char c) {
	switch(c){
		case '\r':
		case '\n':
			terminal_column = 0;
			++terminal_row;
			break;
		case '\b':
			if(terminal_column > 0){
				terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
				--terminal_column;
			}
			break;
		case '\t':
			for(int i = 0; i < tab4; i++)
				terminal_putchar(' ');
			break;
		default:
			terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
			if(++terminal_column == VGA_WIDTH){
				terminal_column = 0;
				++terminal_row;
			}
	}

	if(terminal_row == VGA_HEIGHT){
		--terminal_row;
		terminal_roll();
	}
}

#define update_fg(fg) (terminal_color = vga_entry_color((fg), VGA_BG_MASK(terminal_color)))
#define update_bg(bg) (terminal_color = vga_entry_color(VGA_FG_MASK(terminal_color), (bg)))
#define set_flickr()  (terminal_color &= 0x80)
#define set_default() (terminal_color = default_color)

/**
 * 输出size个字符
 */
int32_t terminal_write(const char* data, uint32_t size) {
	uint32_t i;
	for (i = 0; i < size; i++){
		//vt100
		
		if(i + 1 < size && memcmp(&data[i], "\e[", 2) == 0){

			i += 2;
			//先试着找到第一个m
			char *mp = memchr(&data[i], 'm', size - i);

			//  \033[<数字>m 格式
			if(mp)
			{
				uint32_t len = mp - &data[i];
				const char *token = &data[i];
				do{
					//前景
					if(memcmp(token, "30", 2) == 0)
						update_fg(VGA_COLOR_BLACK);
					else if(memcmp(token, "31", 2) == 0)
						update_fg(VGA_COLOR_RED);
					else if(memcmp(token, "32", 2) == 0)
						update_fg(VGA_COLOR_GREEN);
					else if(memcmp(token, "33", 2) == 0)
						update_fg(VGA_COLOR_LIGHT_BROWN);
					else if(memcmp(token, "34", 2) == 0)
						update_fg(VGA_COLOR_BLUE);
					else if(memcmp(token, "35", 2) == 0)
						update_fg(VGA_COLOR_LIGHT_MAGENTA);
					else if(memcmp(token, "36", 2) == 0)
						update_fg(VGA_COLOR_CYAN);
					else if(memcmp(token, "37", 2) == 0)
						update_fg(VGA_COLOR_WHITE);

					//背景
					else if(memcmp(token, "40", 2) == 0)
						update_bg(VGA_COLOR_BLACK);
					else if(memcmp(token, "41", 2) == 0)
						update_bg(VGA_COLOR_RED);
					else if(memcmp(token, "42", 2) == 0)
						update_bg(VGA_COLOR_GREEN);
					else if(memcmp(token, "43", 2) == 0)
						update_bg(VGA_COLOR_BROWN);
					else if(memcmp(token, "44", 2) == 0)
						update_bg(VGA_COLOR_BLUE);
					else if(memcmp(token, "46", 2) == 0)
						update_bg(VGA_COLOR_CYAN);
					else if(memcmp(token, "47", 2) == 0)
						update_bg(VGA_COLOR_LIGHT_GREY);

					//其他特殊命令
					else if(memcmp(token, "0", 1) == 0)
						set_default();
					else if(memcmp(token, "5", 1) == 0)
					       	set_flickr();	


				}while((token = memchr(token, ';', len)) && token++);

				//这里到 'm'
				i += len;

			//  \033[字母 格式
			}else if(i + 1 < size)
			{
				if(memcmp(&data[i], "2J", 2) == 0)
					terminal_clear();
				i++;
			}else   i--;

			continue;
		}
		

		terminal_putchar(data[i]);
	}

	return (int32_t)size;
}

int32_t terminal_read(char *data, uint32_t size) {

	/** empty */
	return 0;
}

/**
 * 输出字符串
 */
void terminal_writestr(const char* data) {
	mutex_lock_acquire(&tty_lock);
	terminal_write(data, strlen(data));
	mutex_lock_release(&tty_lock);	
}
