/**
 * @file PanelGeometry.h
 * @brief Conversión entre coordenadas físicas 320x480 y UI horizontal 480x320.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
namespace PanelGeometry {
constexpr int width=480,height=320,physicalWidth=320,physicalHeight=480;
// El eje X táctil físico se transforma en Y horizontal; el signo se invierte
// para que el origen final sea la esquina superior izquierda de la UI.
inline bool touchToLandscape(int rawX,int rawY,int& x,int& y){
 if(rawX<0||rawX>=physicalWidth||rawY<0||rawY>=physicalHeight)return false;
 x=rawY;y=physicalWidth-1-rawX;return true;
}
// Índice del sprite horizontal que corresponde a un píxel físico del panel.
inline size_t sourceIndex(int physicalX,int physicalY){
 return size_t(physicalWidth-1-physicalX)*width+physicalY;
}
inline void rotateStripe(const uint16_t* source,uint16_t* out,int firstRow,int rows){
 for(int y=0;y<rows;++y)for(int x=0;x<physicalWidth;++x)
  out[y*physicalWidth+x]=source[sourceIndex(x,firstRow+y)];
}
// Devuelve 0=MUTE, 1=BRILLO, 2=INFO/TEST; -1 fuera de botones.
inline int buttonAt(int x,int y){
 if(y<270||y>=312)return -1;
 for(int i=0;i<3;++i)if(x>=8+i*158&&x<156+i*158)return i;
 return -1;
}
// El controlador puede informar varios contactos. Una sola pulsación válida
// crea una acción; para otra acción es obligatorio recibir una liberación.
class TouchLatch {
 bool down_=false;
 public:
 void reset(){down_=false;}
 bool consume(const uint8_t* points,unsigned count,int& x,int& y){
  unsigned active=0;int foundX=0,foundY=0;bool valid=false;
  for(unsigned i=0;i<count;++i){const auto* q=points+7*i;if(q[0]&0x80){++active;valid=touchToLandscape(((q[0]&0x3f)<<8)|q[1],((q[2]&0x3f)<<8)|q[3],foundX,foundY);}}
  if(!active){down_=false;return false;}if(down_)return false;down_=true;
  if(active!=1||!valid)return false;x=foundX;y=foundY;return true;
 }
};
}
