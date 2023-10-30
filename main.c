#include "vic20_defines.h"

#define HAS_PAGES
#define HAS_GUI

#define USE_CHARCACHE
#define TARGET_UNEXPANDED

#define HAS_ENEMYAI
#define HAS_BALLMOVEMENT
#define HAS_AUTOHIT

// #define HAS_JUSTME
//#define HAS_JUSTBALL

#if !defined(WIN32)
//#define USE_OVERLAYS
#else
int err=0;
#endif

//#define HAS_MUSIC
//#define HAS_KEYBOARD


#if defined(OSCAR64)

#if defined(TARGET_UNEXPANDED)

#if defined(USE_OVERLAYS)
#define BASEMEMEND 0x19e0
#else
#define BASEMEMEND 0x1ba0
#endif

#pragma stacksize( 64 )
#pragma heapsize( 4 )

#pragma region( stack, 0x0100, 0x01f0, , , {stack} )
//#pragma region( stack, 0x033c, 0x03fe, , , {stack} )

#pragma region( main, 0x1080, BASEMEMEND, , , {code,data,bss   } )

#pragma section( cache, 0 )
#pragma region(bank1,  0x033c, 0x03fe, ,1, { cache } )

#pragma section( garbage, 0 )
#pragma region( bank2, 0x1fcc, 0x1fff, ,2, { garbage } )


#else
#pragma region( main, 0x0480, 0x1ba0, , , {code,data,stack,bss   } )
#endif

#if defined(USE_OVERLAYS)
#pragma overlay( game, 1 )
#pragma section( bcodegame, 0 )
#pragma section( bdatagame, 0 )
#pragma region(bank3, BASEMEMEND+1, 0x1ba0, ,1, { bcodegame, bdatagame } )

#pragma overlay( page, 3 )
#pragma section( bcodepage, 0 )
#pragma section( bdatapage, 0 )
#pragma region(bank4,  BASEMEMEND+1, 0x1ba0, ,3, { bcodepage, bdatapage } )
#endif

#else

int win_getmovement();

#endif

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

#define face_left  1
#define ply_walk   2
#define ply_hit    4
#define ply_act    8
//#define face_right 2

#define FIXEDPOINT 3

typedef struct{
 u8   x,y;
 u8   flags,frame,time;
 u16  pos; 
}_player;

typedef struct{
 u8  x,y,h;
 int  dx,dy,dh;
#if defined(BALL_WISEMOVEMENT)
 s8  sx,sy,err,e2;
 u8  destx,desty;
#endif
 u16 pos,pos2; 
}_ball;

#define status_waitingservice 1
#define status_service        2  
#define status_playing        3  

#define status_ballout        8

#define status_gameended     16

#define target_up    1
#define target_down  2
#define target_left  4
#define target_right 8
#define target_leftright 16

typedef struct{
 u16 pos;
 u8  ch;
}_backbuffer;

typedef struct{
 u16 pos;
 u8  ch,ech;
}_drawbuffer;

#pragma bss(garbage)
__striped _player ply[2];
_ball             ball;
#pragma bss(bss)

__striped _backbuffer back[64-BASE_FRAME];
u8                    iback;

u8                    tmp[8];
u8                    gamestatus;
u8                    x,y;
u8                    movement;

#pragma bss(cache)
__striped _drawbuffer buffer[64-BASE_FRAME];
u8                    ibuffer;
#pragma bss(bss)

#if defined(USE_CHARCACHE)
#pragma bss(cache)
u8                    charcache[(64-BASE_FRAME)*8];
#pragma bss(bss)
#endif

u8 mask[8]={128,64,32,16,8,4,2,1};
u8 d[]={0,VIDEO_W,1,VIDEO_W+1};


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
 //srand();

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

#if defined(BALL_WISEMOVEMENT)
s8 abs8(s8 s)
{
 if(s>0)
  return s;
 else
  return -s;
}

void ball_prepare(u8 x1,u8 y1)
{
 ball.destx=x1;ball.desty=y1;
 ball.dx = abs8(x1-ball.x);
 ball.sx = ball.x<x1 ? 1 : -1;
 ball.dy = abs8(y1-ball.y);
 ball.sy = ball.y<y1 ? 1 : -1; 
 ball.err = (ball.dx>ball.dy ? ball.dx : -ball.dy)/2;
 ball.e2=0;
}

void ball_move()
{
 ball.e2 = ball.err;
 if (ball.e2 >-ball.dx) 
 { ball.err -= ball.dy; ball.x += ball.sx; }
 if (ball.e2 < ball.dy) 
 { ball.err += ball.dx; ball.y += ball.sy; }
}

void ball_dest(u8 dest)
{
 // 16 80 144
 // 120-168 64-96
 u8 px,py,bh,by,bx,bw;
 if(dest&target_up)
  {
   by=64;bh=31;  
   if(dest&target_right)
    {bx=80;bw=63;}
   if(dest&target_left)
    {bx=16;bw=63;}
   else
    {bx=16;bw=127;}
  }
 else
  {
   by=120;bh=63;  
   if(dest&target_right)
    {bx=0;bw=127;}
   else
   if(dest&target_left)
    {bx=80;bw=127;}
   else
   {bx=0;bw=255;}
  } 
 rand();
 py=by+(rnd_a&bh);
 rand();
 px=bx+(rnd_a&bw);

 ball_prepare(px,py);
}
#else
void ball_dest(u8 dest)
{
}
#endif


void ball_setpos(u8 x,u8 y)
{
 ball.x=(x<<FIXEDPOINT)+2;
 ball.y=(y<<FIXEDPOINT)+7;ball.h=0;
}

void ball_setdelta(int x,int y)
{ 
 ball.dx=x;ball.dy=y;ball.dh=8;
}
void ball_pos()
{
 u8 y=(ball.y>>FIXEDPOINT);
 ball.pos=(ball.x>>FIXEDPOINT)+(y<<4)+(y<<2);
 y=((ball.y-(ball.h>>FIXEDPOINT))>>FIXEDPOINT);
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
  x=((ball.y-(ball.h>>FIXEDPOINT))&7);
 else
  x=(ball.y&7);
 y=(ball.x&7);
 if(t&&x)
  pdest[x-1]|=mask[y];
 pdest[x]|=mask[y];
}

void ball_draw()
{
 ball_pos();
 ball_mix(ball.pos,0);
 ball_mix(ball.pos2,1);
}



void ball_act()
{
#if defined(BALL_WISEMOVEMENT)
 if(ball.sx||ball.sy)
 {
  ball_move();
  if((ball.y<(6<<FIXEDPOINT))||(ball.y>(22<<FIXEDPOINT))||(ball.x<2)||(ball.x>(20<<FIXEDPOINT)-2))
   gamestatus=status_ballout;
 }
#else
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
#endif
 if(ball.y<(11<<3))
  ball.h+=ball.dh*2;
 else
  ball.h+=ball.dh;
 if(ball.h>(23<<FIXEDPOINT))
  {ball.dh=-ball.dh;ball.h=(23<<FIXEDPOINT);}
 else
  if(ball.h<(1<<FIXEDPOINT))
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
 u8 by=0,l=ply[w].x&0x7,ch;
 
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
 if(ply[w].flags&ply_act)
  ;
 else
 if(ply[w].flags&ply_hit)
  {
   ply[w].time++;
   if(ply[w].time>5)
    {
     ply[w].time=0;
     if(ply[w].frame==5)
      ply[w].frame=8;
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

 gamestatus=status_waitingservice;
 
}

u8 player_canhitball(u8 w)
{ 
 if((ball.y>=ply[w].y+12)&&(ball.y<=ply[w].y+18))
 {
  if(ply[w].flags&face_left)
   {
    if((ball.x>=ply[w].x)&&ball.x<=ply[w].x+(8))
     return 1;
   }
  else
  if((ball.x>=ply[w].x+8)&&(ball.x<=ply[w].x+16))
   return 1;
 }
 return 0;
}

void ball_hit(u8 w)
{
 ball.dy=-ball.dy; 
 if(ball.x>(VIDEO_W/2)*8)
  ball.dx=-1;
 else
  ball.dx=1;
 if(ball.h<(12<<3))
  ball.dh=2;
 //rand();
 //ball.dx=((rnd_a&7)>>1)-2;
}

void player_hit(u8 w)
{
 ply[w].frame=5;ply[w].time=0;
 ply[w].flags|=ply_hit;
}
void player_autohit(u8 w)
{
 if(player_canhitball(w))
  {
   player_hit(w);
   ball_hit(w);
  }
}

void player_act(u8 w)
{
 if((movement&JOY_FIRE)||(ply[w].flags&ply_hit))
  {
   if(ply[w].flags&ply_hit)
    ;
#if !defined(HAS_AUTOHIT)
   else
    {
     player_hit(w);
     if(ball.dy>0)
      if(player_canhitball(w))
       ball_hit(w);
    }
#endif
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

#if defined(HAS_AUTOHIT)
 if(ply[w].flags&(ply_hit|ply_act))
  ;
 else
  if(ball.dy>0)
   player_autohit(w);
#endif
}

void ai_act(u8 w)
{
 if(ply[w].flags&ply_hit)
  ;
 else
  {
   u8 dx,dy;
   ply[w].flags&=(0xff-ply_walk);
   if(ball.dy>0)
    {dx=(VIDEO_W/2)<<3;dy=72;}
   else
    {dx=ball.x-8;dy=64;} 

   if(ply[w].x>dx)
    {
     ply[w].flags|=ply_walk;
     ply[w].flags|=face_left;
     if(ply[w].x>0)
      ply[w].x--;
    }
   else
    if(ply[w].x<dx)
     {
      ply[w].flags|=ply_walk;
      ply[w].flags&=(0xFF-face_left);;
      if(ply[w].x<(VIDEO_W-2)*8)
       ply[w].x++;
     }
   if(ply[w].y>dy)
    ply[w].y--;
   else
    if(ply[w].y<dy)
     ply[w].y++;
  }

 if(ball.dy<0)
  player_autohit(w);
}

#if defined(USE_OVERLAYS)
#pragma code ( bcodegame )
#pragma data ( bdatagame )
#endif

void wait_forunfire()
{
 while(movement&JOY_FIRE)
  game_input();
}

void sync_show()
{
 u8*v=ADDR(VIDEO_RAM);  

#if !defined(WIN32)
 REFRESH  
#endif

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

#if defined(WIN32)
 REFRESH
#endif
}

void game_waitingservice()
{
 ball.h+=ball.dh;
 if(ball.h>(8<<FIXEDPOINT))
  ball.dh=-ball.dh;
 else
  if(ball.h<(1<<FIXEDPOINT))
   ball.dh=-ball.dh;  
 if(movement&JOY_FIRE)
  {
   ball.dh=8;ball.h=8<<FIXEDPOINT;
   ply[1].frame=5;ply[1].flags|=ply_act;
   gamestatus=status_service;
   wait_forunfire();
  }
}

#define target_up    1
#define target_down  2
#define target_left  4
#define target_right 8
#define target_leftright 16



void game_service()
{
 ball.h+=ball.dh;
 if(ball.h>(24<<FIXEDPOINT))
  ball.dh=-ball.dh;
 else
  if(ball.h<(1<<FIXEDPOINT))
   {
    ball.dh=-ball.dh;  
    ply[1].frame=0;ply[1].flags=0;
    gamestatus=status_waitingservice;
   }
 if(movement&JOY_FIRE)
  {  
   ply[1].flags=0;
   if((ball.h>=(10<<FIXEDPOINT))&&(ball.h<(20<<FIXEDPOINT)))
    {
     ball_dest(target_up|target_right);
     ball.h+=(8<<FIXEDPOINT);
     ball.dh=-2;
     ply[1].frame=8;ply[1].time=0;
     ply[1].flags|=ply_hit;
     gamestatus=status_playing;
    }
   else
    {
     ply[1].frame=0;
     ball.dh=8;ball.h=0;    
     gamestatus=status_waitingservice;
    }
  }
}

void game_draw_characters()
{ 
 //sync_hide();
 switch(gamestatus)
  {
   case status_waitingservice:
    game_waitingservice();
   break;
   case status_service:
    game_service();
   break;
   case status_playing:
    {
     player_act(1);
    #if defined(HAS_ENEMYAI)
     ai_act(0);
    #endif
    #if defined(HAS_BALLMOVEMENT)
     ball_act();
    #endif
    }
  }
 
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

#if defined(USE_OVERLAYS)
#pragma code ( code )
#pragma data ( data )
#endif

// -------------------------------------------------------------
#if defined(USE_OVERLAYS)
#pragma code ( bcodepage )
#pragma data ( bdatapage )
#endif

#define IMPLEMENT_C_HBUNPACK
#include "hupack.c"

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

  hunpack(ADDR(0x1fff-168),ADDR(COLOR_RAM));
  hunpack(ADDR(0x1fff-168)+24,ADDR(VIDEO_RAM));
  
  //page_unpack(ADDR(0x1fff-237));
  #endif
  #endif

  

  break;

 }
}

#if defined(USE_OVERLAYS)
#pragma code ( code )
#pragma data ( data )
#endif


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
     load("GAME");
#endif
     while(1)
      {
       game_input();       
       game_draw_characters();  
       gui_update();       

       if(gamestatus==status_gameended)
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