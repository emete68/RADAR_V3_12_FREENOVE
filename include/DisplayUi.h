/** @file DisplayUi.h Fachada de la interfaz y de su frame-buffer PSRAM. */
#pragma once
#include <TFT_eSPI.h>
#include "RadarTypes.h"
#include "ST77922Panel.h"
class DisplayUi {
 public:
  void begin();void showBoot(const char* text);void brightness();
  bool available()const{return sprite_&&panel_.available();}
  void render(bool fix,uint32_t sats,float speed,const RadarCandidate& radar,
              bool wifi,uint32_t count,bool muted,bool touch,bool audio,
              double lat,double lon,uint32_t chars);
  // Público porque main decide si un toque INFO abre/cierra el diagnóstico.
  bool info=false;
 private:
  TFT_eSPI graphics_;TFT_eSprite frame_{&graphics_};ST77922Panel panel_;
  bool sprite_=false;uint8_t bright_=2;
};
