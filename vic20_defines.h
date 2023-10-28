
#define JIFFYH		0xA0
#define JIFFYM		0xA1
#define JIFFYL		0xA2
#define VICSCRHO 0x9000    // 36864 - Horizontal position of the screen
#define VICSCRVE 0x9001    // 36865 - Vertical position of the screen
#define VICCOLNC 0x9002    // 36866 - Screen width in columns and video memory addr.
#define VICROWNC 0x9003    // 36867 - Screen height, 8x8 or 8x16 chars, scan line addr.
#define VICRAST  0x9004    // 36868 - Bits 8-1 of the current raster line
#define VICCHGEN 0x9005    // 36868 - Character gen. and video matrix addresses.
#define GEN1     0x900A    // First sound generator
#define GEN2     0x900B    // Second sound generator
#define GEN3     0x900C    // Third sound generator
#define NOISE    0x900D    // Noise sound generator
#define VOLUME   0x900E    // Volume and additional colour info
#define VICCOLOR 0x900F    // Screen and border colours

#define PORTAVIA1 0x9111   // Port A 6522 (joystick)
#define PORTAVIA1d 0x9113  // Port A 6522 (joystick)
#define PORTBVIA2 0x9120   // Port B 6522 2 value (joystick)
#define PORTBVIA2d 0x9122  // Port B 6522 2 direction (joystick

#define MEMSCR   0x1E00    // Start address of the screen memory (unexp. VIC)
#define MEMCLR   0x9600    // Start address of the colour memory (unexp. VIC)

#define REPEATKE 0x028A    // Repeat all keys

#define VOICE1  GEN1      // Voice 1 for music
#define VOICE2  GEN2      // Voice 2 for music
#define EFFECTS GEN3      // Sound effects (not noise)
#define COLOR_BLACK             0x00
#define COLOR_WHITE             0x01
#define COLOR_RED               0x02
#define COLOR_CYAN              0x03
#define COLOR_VIOLET            0x04
#define COLOR_GREEN             0x05
#define COLOR_BLUE              0x06
#define COLOR_YELLOW            0x07
/* Only the background and multi-color characters can have these colors. */
#define COLOR_ORANGE            0x08
#define COLOR_LIGHTORANGE       0x09
#define COLOR_PINK              0x0A
#define COLOR_LIGHTCYAN         0x0B
#define COLOR_LIGHTVIOLET       0x0C
#define COLOR_LIGHTGREEN        0x0D
#define COLOR_LIGHTBLUE         0x0E
#define COLOR_LIGHTYELLOW       0x0F

#define COLOR_RAM       0x9600

#define VIDEO_RAM       0x1e00

#define VIA1DDR     0x9113
#define VIA2DDR     0x9122             // ?
#define JOY0        0x9111
#define JOY0B       0x9120             // output register B VIA #2
#define JOYUP       0x4                // joy up bit
#define JOYDWN      0x8                // joy down
#define JOYL        0x10               // joy left
#define JOYT        0x20              // joy fire
#define JOYR        0x80               // joy right bit

#define KEY_C       0x22
#define KEY_H       0x2B

#define SETLFS      $FFBA
#define SETNAM      $FFBD
#define LOAD        $FFD5
#define GETIN       $FFE4
#define CHKIN       $FFC6

#define C1 135
#define D1 147
#define E1 159
#define F1 163
#define G1 175
#define A1 183
#define B1 191

#define C2 195
#define C3 225
#define C4 240


#if defined(WIN32)
int resource_write(void*mem,int size,const char*name);
extern int VIC20_refresh();
void memory_set(unsigned int address, unsigned char value);
unsigned char memory_get(unsigned int address);
extern unsigned char memory[65536];;
#define REFRESH if(VIC20_refresh()==0) return 1;

#define POKE(addr,val)     memory_set((int)addr,val);
#define PEEK(addr)         memory_get((int)addr)
#define ADDR(addr)         &memory[(int)addr]

#define __striped
#define __zeropage

void wait()
{
 int i;
 for(i=0;i<50;i++)
  VIC20_refresh();
}

#else

//#pragma region( main, 0x1080, 0x1c00, , , {code, data, bss, heap, stack} )

#define REFRESH sync();

#define POKE(addr,val)     (*(unsigned char*) (addr) = (val))
#define POKEW(addr,val)    (*(unsigned*) (addr) = (val))
#define PEEK(addr)         (*(unsigned char*) (addr))
#define PEEKW(addr)        (*(unsigned*) (addr))

#define ADDR(addr) ((u8*)addr)

void * memset(void * dst, int value, int size)
{
	__asm
	{
			lda	value

			ldx	size + 1
			beq	_w1
			ldy	#0
	_loop1:
			sta (dst), y
			iny
			bne	_loop1
			inc dst + 1
			dex
			bne	_loop1
	_w1:
			ldy	size
			beq	_w2
	_loop2:
			dey
			sta (dst), y
			bne _loop2
	_w2:
	}
	return dst;
}

#endif

typedef unsigned char  u8;
typedef char           s8;
typedef unsigned short u16;