#include "keyboard_map.h"
/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08
#define ENTER_KEY_CODE 0x1C
#define BACKSPACE_KEY_CODE 0x0E
#define LEFT_SHIFT_PRESSED 0x2A
#define RIGHT_SHIFT_PRESSED 0x36
#define LEFT_SHIFT_RELEASED 0xAA
#define RIGHT_SHIFT_RELEASED 0xB6
#define TAB 0x0F 
#define CAPS_PRESSED 0x3A
#define NUM_PRESSED 0x45
extern unsigned char keyboard_map[128];
extern unsigned char keyboard_map_shift[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);
int shift =0,CAPS=0,NUM=0,combination=0,q=0,user=0;
int color=0xf1;
unsigned int current_loc = 0;
char *vidptr = (char*)0xb8000;
const char *msg = "Please Enter Your Name:>\n";
char *s;

struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];


void idt_init(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	/* populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

	/*     Ports
	*	 PIC1	PIC2
	*Command 0x20	0xA0
	*Data	 0x21	0xA1
	*/

	/* ICW1 - begin initialization */
	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	/* ICW2 - remap offset address of IDT */
	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	/* ICW3 - setup cascading */
	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);

	/* ICW4 - environment info */
	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);
	/* Initialization finished */

	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void kb_init(void)
{
	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	write_port(0x21 , 0xFD);
}

void kprint(const char *str)
{
	unsigned int i = 0;
	while (str[i] != '\n') {
		vidptr[current_loc++] = str[i++];
		vidptr[current_loc++] = color;
	}
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_loc = current_loc + (line_size - current_loc % (line_size));
}

void kprint_newline(void)
{
	
	
	unsigned int i = 0;
	if(user==0)
	{
		while (msg[i] != '\n') 
		{
			vidptr[current_loc++] = msg[i++];
			vidptr[current_loc++] = color;
		}
	
	}
	else
	{unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_loc = current_loc + (line_size - current_loc % (line_size));
		
		while (s[i] != '\0') 
		{
			vidptr[current_loc++] = s[i++];
			vidptr[current_loc++] = color;
		}
	}
}
void clear_screen(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
		vidptr[i++] = ' ';
		vidptr[i++] = color;
	}
}

void keyboard_handler_main(void)
{
	unsigned char status;
	
	char keycode;

	/* write EOI */
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	/* Lowest bit of status will be set if buffer is not empty */
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;
		if(user==0)
		{
			if(keycode == ENTER_KEY_CODE) 
			{
				s[q++]=':';
				s[q++]='>';
				s[q++]='\0';
				user=1;
				kprint_newline();
				return;
			}
			if(keycode == RIGHT_SHIFT_PRESSED || keycode == LEFT_SHIFT_PRESSED)
		 {
                        shift=1;
			return;
		 }
		if(keycode == CAPS_PRESSED)
		 {
			if(CAPS == 1)
	                  {CAPS=0;}
			else
			{CAPS=1;}
			return;
		 }
			if(shift == 1 || CAPS == 1)
			{
				s[q++]=keyboard_map_shift[(keycode)];
				shift=0;		
			}
			else
			s[q++]=keyboard_map[keycode];
			
			
		}



		if(keycode == ENTER_KEY_CODE) {
			kprint_newline();
			return;
		}
		if(keycode == BACKSPACE_KEY_CODE) {
			if((current_loc%80)>16)
			{			
			current_loc--;
			current_loc--;
			vidptr[current_loc] = ' ';}
			return;
		}
		if(keycode == RIGHT_SHIFT_PRESSED || keycode == LEFT_SHIFT_PRESSED)
		 {
                        shift=1;
			return;
		 }
		if(keycode == CAPS_PRESSED)
		 {
			if(CAPS == 1)
	                  {CAPS=0;}
			else
			{CAPS=1;}
			return;
		 }
		if(keycode == NUM_PRESSED)
		 {
			if(NUM == 1)
	                  {NUM=0;}
			else
			{NUM=1;}
			return;
		 }
		if(keycode == TAB)
		{
			unsigned int i = 0;
	              while (i <3)
			{
			vidptr[current_loc++] =' ';
			vidptr[current_loc++] = color;
			i++;
			} 
		return;
		}
		if(NUM == 0)
		{
		 if(keycode >= 71 && keycode <=82 && keycode!=74 && keycode!=78)
			return;
		}	
		
		if(shift == 1 || CAPS == 1)
		{
			vidptr[current_loc++] = keyboard_map_shift[(unsigned char) keycode];
			shift=0;		
		}
		else
		vidptr[current_loc++] = keyboard_map[(unsigned char) keycode];
		vidptr[current_loc++] = color;
		
	}
}


void kmain(void)
{
	const char *str = "Hello User !! Welcome To Our OS \n ";
	clear_screen();
	kprint(str);
	kprint_newline();
	idt_init();
	kb_init();
	while(1);
}
