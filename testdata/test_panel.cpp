#include "PanelGeometry.h"
#include "BoardPins.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
using namespace PanelGeometry;
static uint16_t source[width*height],stripe[physicalWidth*8+2];
static bool seen[width*height];
static void contact(uint8_t* p,int x,int y){p[0]=0x80|(x>>8);p[1]=x;p[2]=y>>8;p[3]=y;}
int main(){
 // Every raw panel pixel must map onto a unique in-bounds logical pixel.
 for(int py=0;py<physicalHeight;++py)for(int px=0;px<physicalWidth;++px){
  int x=-1,y=-1;assert(touchToLandscape(px,py,x,y));assert(x>=0&&x<width&&y>=0&&y<height);
  size_t index=size_t(y)*width+x;assert(index==sourceIndex(px,py));assert(!seen[index]);seen[index]=true;
 }
 for(bool b:seen)assert(b);
 int x,y;assert(touchToLandscape(0,0,x,y)&&x==0&&y==319);
 assert(touchToLandscape(319,479,x,y)&&x==479&&y==0);
 assert(!touchToLandscape(320,0,x,y));assert(!touchToLandscape(0,480,x,y));assert(!touchToLandscape(-1,0,x,y));
 for(size_t i=0;i<width*height;++i)source[i]=uint16_t(i^(i>>16));
 for(int row=0;row<physicalHeight;row+=8){
  stripe[0]=0x1234;stripe[physicalWidth*8+1]=0x5678;
  rotateStripe(source,stripe+1,row,8);
  assert(stripe[0]==0x1234&&stripe[physicalWidth*8+1]==0x5678);
  for(int dy=0;dy<8;++dy)for(int px=0;px<physicalWidth;++px)
   assert(stripe[1+dy*physicalWidth+px]==source[(319-px)*480+row+dy]);
 }
 assert(buttonAt(8,270)==0&&buttonAt(155,311)==0);
 assert(buttonAt(156,280)==-1&&buttonAt(165,280)==-1);
 assert(buttonAt(166,270)==1&&buttonAt(313,311)==1);
 assert(buttonAt(324,270)==2&&buttonAt(471,311)==2);
 assert(buttonAt(0,270)==-1&&buttonAt(8,269)==-1&&buttonAt(8,312)==-1);
 TouchLatch latch;uint8_t points[70]={};contact(points,20,80);
 assert(latch.consume(points,10,x,y)&&x==80&&y==299);assert(!latch.consume(points,10,x,y));
 memset(points,0,sizeof(points));assert(!latch.consume(points,10,x,y));
 contact(points+7,20,240);assert(latch.consume(points,10,x,y)&&buttonAt(x,y)==1);
 latch.reset();contact(points,20,80);assert(!latch.consume(points,10,x,y)); // Two fingers ignored.
 memset(points,0,sizeof(points));latch.consume(points,10,x,y);
 contact(points,600,0);assert(!latch.consume(points,10,x,y)); // Bad coordinates cannot trigger buttons.
 static_assert(BoardPins::kGnssRx==44&&BoardPins::kGnssTx==43,"UART FNK0104N");
 static_assert(BoardPins::kI2sOut==15&&BoardPins::kI2sMclk==17&&BoardPins::kI2sBclk==18&&BoardPins::kI2sWs==21,"Audio FNK0104N");
 static_assert(BoardPins::kI2cSda==38&&BoardPins::kI2cScl==39&&BoardPins::kTouchReset==48,"I2C FNK0104N");
 puts("PASS: 153600 pixels, rotation/inverse touch, stripe bounds, buttons, touch latch, GPIO contract");
}
