#include "vic20_defines.h"

#define HAS_PAGES
#define HAS_GUI

#define USE_CHARCACHE

// #define HAS_ENEMYAI
// #define HAS_BALLMOVEMENT

// #define HAS_JUSTME
//#define HAS_JUSTBALL

#if !defined(WIN32)
//#define USE_OVERLAYS
#else
int err=0;
#endif
//#define USE_FULL_OVERLAYS
#define HAS_MUSIC

#define HAS_KEYBOARD
//#define HAS_LIGHTING

#if defined(OSCAR64)

#pragma stacksize( 64 )
#pragma heapsize( 4 )

#pragma region( stack, 0x0100, 0x01f0, , , {stack} )
//#pragma region( bss, 0x033c, 0x03a7, , , {bss} )

#pragma region( main, 0x1080, 0x1ba0, , , {code,data,bss   } )


#else

int win_getmovement();

#endif

#define IMPLEMENT_C_HBUNPACK
#include "hupack.c"

#if defined(USE_OVERLAYS)
#if defined(WIN32)
void load(const char * fname)
{
}
#else
#include <c64/kernalio.h>
void load(const char * fname)
{
	krnio_setnam(fname);
	krnio_load(1, 8, 1);
}
#endif
#endif

u8  rnd_a;
u16 seed = 31232;
void rand(void)
{
 seed ^= seed << 7;
 seed ^= seed >> 9;
 seed ^= seed << 8;
 rnd_a=seed&0xFF;
	//return seed;
}
void srand()
{ 
 #if defined(WIN32)
 rnd_a=0;
 #else
 __asm{
try_again:
        lda $a2          //       ;jiffy as random seed
        beq try_again    //      ;can't be zero
        sta rnd_a
 }
 #endif 
 seed=(seed&0xff00)|rnd_a;
}

// -------------------------------------------------------------

#define BASE_VIDEO_W 22
#define VIDEO_W      20
#define VIDEO_H      23

#define PAGE_TITLESCREEN 1
#define PAGE_INGAME      2
#define PAGE_GAMEOVER    3

// -------------------------------------------------------------

#define JOY_UP  0x04
#define JOY_DOWN 0x08
#define JOY_LEFT 0x10
#define JOY_RIGHT 0x80
#define JOY_FIRE 0x20

// -------------------------------------------------------------

#define BASE_FRAME    48
#define FRAMES_START 0x1ba0

// -------------------------------------------------------------

#define game_ended 1

#define face_left  1
#define ply_walk   2
#define ply_hit    4
//#define face_right 2

#define FIXEDPOINT 3

typedef struct{
 u8   x,y;
 u8   flags,frame,time;
 u16  pos; 
}_player;

typedef struct{
 u8  x,y,h;
 s8  dx,dy,dh;
 u16 pos,pos2; 
}_ball;

u8      gamestatus;
u8      x,y,ch,r;
u8      movement;

typedef struct{
 u16 pos;
 u8  ch;
}_backbuffer;

typedef struct{
 u16 pos;
 u8  ch,ech;
}_drawbuffer;

__striped _player ply[2];
_ball   ball;

__striped _backbuffer back[64-BASE_FRAME];
u8                    iback;
__striped _drawbuffer buffer[64-BASE_FRAME];
u8                    ibuffer;
#if defined(USE_CHARCACHE)
u8                    charcache[(64-BASE_FRAME)*8];
#endif


#if defined(WIN32)
char res[]="R00";
#endif

// -------------------------------------------------------------

#if defined(WIN32)
void loaddata(const char*name,u8*mem);
extern int win_joystick;
u8 getkey()
{
 return win_joystick; 
}
#else
//CBM entry for computers other than the PET.

void loadres(u8 which)
{
 u8*res=ADDR(0x02a1);
 res[0]='R';res[1]='0';res[2]='0'+which;
 __asm {
		lda #$00
		ldx #$08
		ldy #$01
		jsr SETLFS
		lda #$03
		ldx #$a1 //<res
		ldy #$02 //>res
		jsr SETNAM
		lda #$00
		jsr LOAD
  BCS  error 
  rts
error:
  sta VICCOLOR
  rts
 }
}
void wait()
{
 __asm{
 		LDA JIFFYL
		STA $FF			// ; save this jiffy
loop3:	LDA JIFFYL
		SEC
		SBC $FF
		CMP #$80		// ; wait 2+ seconds
		BNE loop3
 }
}
void sync()
{
 __asm{
ll:
       lda $9004
       bne ll
 } 
 }

void syncPAL()
{
 __asm{
 loopsync:  lda $9004 //VICRAST     ; Synchronization loop
            cmp #140
            bne loopsync
}
}

#endif

// -------------------------------------------------------------


// -------------------------------------------------------------

#define SCREEN_COLOUR	COLOR_GREEN
#define BORDER_COLOUR COLOR_BLACK

#define SCREENBORDER_VALUE 	(SCREEN_COLOUR <<4) + 0x08 + BORDER_COLOUR


void game_init()
{
 srand();

 POKE(VICCOLOR,SCREENBORDER_VALUE);

 // load small custom charset
#if defined(WIN32)
 res[1]='0';res[2]='1';
 loaddata(res,ADDR(0)); 
#else 
 loadres(1);
#endif
#if defined(WIN32)
 res[1]='0';res[2]='3';
 loaddata(res,ADDR(0)); 
#else 
 loadres(3);
#endif

 POKE(VICCOLNC,VIDEO_W | 0x80);
#if defined(WIN32)
 POKE(VICSCRHO,PEEK(VICSCRHO)+(BASE_VIDEO_W-VIDEO_W)*8);
#else
 POKE(VICSCRHO,PEEK(VICSCRHO)+(BASE_VIDEO_W-VIDEO_W));
#endif
}

void game_input()
{
#if defined(WIN32)
 movement=win_getmovement();
#else
 __asm{
       lda #$7f    //; Set data direction register for VIA2 so that East can be read. This turns
       sta $9122   //;   OFF some keys, so we make sure to restore it later.
       lda $9111   //; Read VIA for North (bit 2), South (bit 3), West (bit 4) and Fire (bit 5)
       and #$3c    //; Clear bits we don't care about (0, 1, 6, 7)*
       sta movement   //  ; Stash this value somewhere
       lda $9120   //; Read East (bit 7)
       and #$80   // ; We only care about bit 7 of this VIA2 port
       ora movement //    ; Combine bit 7 of VIA2B with previously-stashed value of $fe
       pha         //; Push the final result while we put the data direction register back
       lda #$ff    //; Put the data direction register back for VIA2
       sta $9122   //; ,,
       pla         //; Put the register value in A
       eor #$ff
       sta movement  //   ;   And/or store it for use with BIT
 }
 #endif
}

#define mode_hide 1
#define mode_draw 2
#define mode_save 3

u8 mask[8]={128,64,32,16,8,4,2,1};

void ball_setpos(u8 x,u8 y)
{
 ball.x=(x<<FIXEDPOINT)+2;
 ball.y=(y<<FIXEDPOINT)+7;ball.h=0;
}

void ball_setdelta(s8 x,s8 y)
{ 
 ball.dx=x;ball.dy=y;ball.dh=1;
}
void ball_pos()
{
 u8 y=(ball.y>>FIXEDPOINT);
 ball.pos=(ball.x>>FIXEDPOINT)+(y<<4)+(y<<2);
 y=((ball.y-ball.h)>>FIXEDPOINT);
 ball.pos2=(ball.x>>FIXEDPOINT)+(y<<4)+(y<<2);
}

u8*add_back(u16 pos)
{
 u8*v=ADDR(VIDEO_RAM);
#if defined(USE_CHARCACHE)
 u8 dest=ibuffer;
#else
 u8 dest=BASE_FRAME+ibuffer;
#endif
 u8*pdest;
 u8 a=v[pos];
 u8*pa;
 for(x=0;x<ibuffer;x++)
  if(buffer[x].pos==pos)
   {    
#if defined(USE_CHARCACHE)
    dest=x;
    return charcache+(dest<<3);
#else
    dest=BASE_FRAME+x;
    return ADDR(0x1C00)+(dest<<3);
#endif
   }
 // check if pos is in backbuffer
 for(x=0;x<iback;x++)
  if(back[x].pos==pos)
   {
    a=back[x].ch;
    break;
   }
 // add a new 
 buffer[ibuffer].pos=pos;
 buffer[ibuffer].ch=a;
 buffer[ibuffer].ech=dest;
 ibuffer++;
#if defined(WIN32)
 {
  int ibuffersize=sizeof(buffer)/sizeof(buffer[0]);
  if(ibuffer>=ibuffersize)
   err++;
 }
#endif

 //v[pos]=dest;

#if defined(USE_CHARCACHE)
 pdest=charcache+(dest<<3);
#else
 pdest=ADDR(0x1C00)+(dest<<3);
#endif
 pa=ADDR(0x1C00)+(a<<3); 
 for(x=0;x<8;x++)
  pdest[x]=pa[x];

 return pdest;
}

void ball_mix(u16 pos,u8 t)
{
 u8*pdest=add_back(pos);

 if(t)
  x=((ball.y-ball.h)&7);
 else
  x=(ball.y&7);
 y=(ball.x&7);
 if(t)
  pdest[x]|=mask[y];
 pdest[x+1]|=mask[y];
}

void ball_draw()
{
 ball_pos();
 ball_mix(ball.pos,0);
 ball_mix(ball.pos2,1);
}



void ball_act()
{
 ball.x+=ball.dx;
 if(ball.y<(6<<FIXEDPOINT))
  ball.dy=-ball.dy;
 else
  if(ball.y>(22<<FIXEDPOINT))
   ball.dy=-ball.dy;
 if(ball.x<2)
  ball.dx=-ball.dx;
 else
  if(ball.x>(20<<FIXEDPOINT)-2)
   ball.dx=-ball.dx;
 ball.y+=ball.dy;
 ball.h+=ball.dh;
 if(ball.h>23)
  ball.dh=-ball.dh;
 else
  if(ball.h<1)
   ball.dh=-ball.dh;  
}

void efx_flip(u8*ptr)
{
#if defined(WIN32)
 u8 pow1[]={128,64,32,16,8,4,2,1};
 u8 pow2[]={1,2,4,8,16,32,64,128};
 for(y=0;y<8;y++)
 {
  u8 a=0;
  for(x=0;x<8;x++)
   if(ptr[y])
    if(ptr[y]&pow1[x])
     a|=pow2[x];
  ptr[y]=a;
 }
#else
 u8 t=PEEK(0xfd);
 __asm{
       ldy #$0
loopx:
       lda ($0d),y
       sta $fd
       lda #$01
loop:  lsr $fd
       rol 
       bcc loop
       sta ($0d),y
       iny
       cpy #$08
       bne loopx       
 }
 POKE(0xfd,t);
#endif
}

void ply_setpos(u8 w,u8 x,u8 y,u8 face)
{
 ply[w].x=x<<FIXEDPOINT;
 ply[w].y=y<<FIXEDPOINT;
 ply[w].flags=face;
}

u8 d[]={0,VIDEO_W,1,VIDEO_W+1};
u8 tmp[8];

void gettmp(u8 a,u8 flip)
{
 u8*pa=ADDR(0x1C00)+(a<<3); 
 if(flip)
 {
  for(x=0;x<8;x++)   
   tmp[x]=((pa[x]&0x01)<<7)|((pa[x]&0x02)<<5)|((pa[x]&0x04)<<3)|((pa[x]&0x08)<<1)|((pa[x]&0x10)>>1)|((pa[x]&0x20)>>3)|((pa[x]&0x40)>>5)|((pa[x]&0x80)>>7);
 }
 else
 {
  for(x=0;x<8;x++)
   tmp[x]=pa[x]; 
 }
}

void ply_mix(u8 w)
{
 u8*frames=ADDR(FRAMES_START)+(ply[w].frame<<2);
 u8 by=0,l=ply[w].x&0x7;
 
 if(ply[w].flags&face_left)
  by=2;

 for(ch=0;ch<2;ch++)
  {
   u8 y;
   if(ch==1) l=8-l;
   for(y=0;y<4;y++)
    {
     u8*pdest;
     u8 a=frames[(y+by)&3];
     gettmp(a,by);   

     pdest=add_back(ply[w].pos+d[y]+ch);
     
     for(x=0;x<8;x++)
      if(ch==0)
       pdest[x]|=tmp[x]>>l;
      else
       pdest[x]|=tmp[x]<<l;
    }
   if(l==0) 
    break;
  }
}
void ply_pos(u8 w)
{
 u8 y=(ply[w].y>>FIXEDPOINT);
 ply[w].pos=(ply[w].x>>FIXEDPOINT)+(y<<4)+(y<<2);
}

void ply_draw(u8 w)
{ 
 
 if(ply[w].flags&ply_hit)
  {
   ply[w].time++;
   if(ply[w].time>5)
    {
     ply[w].time=0;
     if(ply[w].frame==5)
      ply[w].frame=8;
     else
      if(ply[w].frame==8)
       ply[w].frame=4;
      else
       ply[w].flags-=ply_hit;
    }
  }
 else
 if(ply[w].flags&ply_walk)
  {
   if((ply[w].frame<1)||(ply[w].frame>3))
    {ply[w].frame=1;ply[w].time=0;}
   ply[w].time++;
   if(ply[w].time>2)
    {
     ply[w].time=0;
     ply[w].frame++;
     if(ply[w].frame==4)
      ply[w].frame=1;
    }
  }
 else
  ply[w].frame=0;
 ply_pos(w);
 ply_mix(w);

}

void game_reset()
{  
 ply_setpos(0,11,8,face_left);
 
 ply_setpos(1,4,19,0);
 ball_setpos(5,20);

 ball_setdelta(1,-1); 

 iback=0;
 
}

void player_act(u8 w)
{
 if((movement&JOY_FIRE)||(ply[w].flags&ply_hit))
  {
   if(ply[w].flags&ply_hit)
    ;
   else
   {
    ply[w].frame=5;ply[w].time=0;
    ply[w].flags|=ply_hit;
   }
  }
 else
 if(movement&(JOY_LEFT|JOY_RIGHT|JOY_UP|JOY_DOWN))
  {  
   ply[w].flags|=ply_walk;
   if(movement&JOY_LEFT)
    {
     ply[w].flags|=face_left;
     if(ply[w].x>0)
      ply[w].x-=2;
    }
   else
    if(movement&JOY_RIGHT)
     {
      ply[w].flags&=(0xFF-face_left);
      if(ply[w].x<(VIDEO_W-2)*8)
       ply[w].x+=2;
     }
   if(movement&JOY_UP)
    {
     if(ply[w].y>12<<3)
       ply[w].y--;
    }
   else
    if(movement&JOY_DOWN)
     {
      if(ply[w].y<20<<3)
       ply[w].y++;
     }   
  }
 else
  ply[1].flags&=(0xff-ply_walk);
}

void ai_act(u8 w)
{
 if(ply[w].x>ball.x)
  {
   ply[w].flags|=ply_walk;
   ply[w].flags|=face_left;
   if(ply[w].x>0)
    ply[w].x--;
  }
 else
  if(ply[w].x<ball.x)
   {
    ply[w].flags|=ply_walk;
    ply[w].flags&=(0xFF-face_left);;
    if(ply[w].x<(VIDEO_W-2)*8)
     ply[w].x++;
   }
  else
   ply[w].flags&=(0xff-ply_walk);
}

void sync_hide()
{
 u8*v=ADDR(VIDEO_RAM);  
 while(iback--)
  v[back[iback].pos]=back[iback].ch;
 iback=0;
}
void sync_show()
{
 u8*v=ADDR(VIDEO_RAM);  
 
 REFRESH  

 // restore previous elements
 while(iback--)
  v[back[iback].pos]=back[iback].ch;
 iback=0;
 // draw new ones
 for(x=0;x<ibuffer;x++)
  {
#if defined(USE_CHARCACHE)
   v[buffer[x].pos]=BASE_FRAME+buffer[x].ech;
#else
   v[buffer[x].pos]=buffer[x].ech;
#endif
   back[x].pos=buffer[x].pos;
   back[x].ch=buffer[x].ch;
  }
#if defined(USE_CHARCACHE)
 v=ADDR(0x1C00)+((BASE_FRAME)<<3);
 for(x=0;x<ibuffer<<3;x++)
  v[x]=charcache[x];
#endif
 
 iback=ibuffer;ibuffer=0;
}

void game_draw_characters()
{ 
 //sync_hide();

 player_act(1);
#if defined(HAS_ENEMYAI)
 ai_act(0);
#endif
#if defined(HAS_BALLMOVEMENT)
 ball_act();
#endif
 
#if defined(HAS_JUSTBALL)
 ball_draw();
#elif defined(HAS_JUSTME)
 ply_draw(1);
#else
 ply_draw(0);
 ply_draw(1);
 ball_draw();
#endif 
 sync_show(); 
}

void gui_update()
{
}

// -------------------------------------------------------------

void page_draw_page(u8 r)
{
 #if defined(HAS_PAGES)
 while(1)
  {
  u8 pg=r;

  memset(ADDR(COLOR_RAM),SCREEN_COLOUR,VIDEO_W*VIDEO_H);   

 #if defined(WIN32)
  res[2]='0'+r;
  loaddata(res,ADDR(0)); 
 #else 
  loadres(r);
 #endif
  #endif

  #if defined(HAS_PAGES)
  #if defined(HAS_PETPAGES)
  POKE(VICCHGEN,(PEEK(VICCHGEN)&0xF0)|0x0);
  page_unpack(ADDR(0x1c38));
  #else
  POKE(VICCHGEN,(PEEK(VICCHGEN)&0xF0)|0xF);

  hunpack(ADDR(0x1fff-167),ADDR(COLOR_RAM));
  hunpack(ADDR(0x1fff-167)+23,ADDR(VIDEO_RAM));
  
  //page_unpack(ADDR(0x1fff-237));
  #endif
  #endif

  

  break;

 }
}

// -------------------------------------------------------------

#if defined(WIN32)
int VIC20_main()
#else
int main()
#endif
{ 
#if defined(USE_OVERLAYS)
 load("PAGE");
#endif

 game_init();
  
 // main game lopp
 while(1)
  {
   // home
   //page_draw_page(PAGE_TITLESCREEN);

   // start game
   

   while(1)
    {
     page_draw_page(PAGE_INGAME);
     REFRESH

     game_reset();

// ---------------------------
#if defined(USE_OVERLAYS)
#if defined(USE_FULL_OVERLAYS)
     load("GAME");
#endif
#endif
     while(1)
      {
       game_input();       
       game_draw_characters();  
       gui_update();       

       if(gamestatus&game_ended)
        break;

   #if !defined(WIN32)
       __asm{
        LDA JIFFYL
loop1:	CMP JIFFYL
		       BEQ loop1
       }
   #endif
      }

#if defined(HAS_MUSIC)     
     POKE(EFFECTS,0);
#endif

#if defined(USE_OVERLAYS)
     load("PAGE");
#endif
     page_draw_page(PAGE_GAMEOVER);
     break;
    }
  }
 
 return 0;
}