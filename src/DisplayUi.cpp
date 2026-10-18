/**
 * @file DisplayUi.cpp
 * @brief Dibuja la interfaz en un sprite PSRAM y lo entrega al panel QSPI.
 */
#include "DisplayUi.h"
#include "BoardPins.h"
#include "ProjectConfig.h"
void DisplayUi::begin(){
 // TFT_eSPI solo aporta fuentes y primitivas de dibujo. No se llama a init()
 // porque el dueño exclusivo de los pines QSPI es ST77922Panel.
 frame_.setColorDepth(16);sprite_=frame_.createSprite(480,320)!=nullptr;frame_.setSwapBytes(true);
 if(!sprite_||!panel_.begin()){Serial.println("ERROR: pantalla/PSRAM no disponible");return;}
 // Limpiar el primer cuadro y enviarlo antes de encender la retroiluminación.
 frame_.fillSprite(TFT_BLACK);panel_.present(static_cast<uint16_t*>(frame_.getPointer()));
 ledcSetup(7,5000,8);ledcAttachPin(BoardPins::kTftBacklight,7);ledcWrite(7,255);
}
// Los tres niveles reducen el consumo y son más previsibles que un deslizador.
void DisplayUi::brightness(){if(!available())return;const uint8_t levels[]={40,120,255};bright_=(bright_+1)%3;ledcWrite(7,levels[bright_]);}
void DisplayUi::showBoot(const char* t){if(!available())return;frame_.fillSprite(TFT_BLACK);frame_.setTextColor(TFT_WHITE,TFT_BLACK);frame_.drawCentreString("RADAR V3.12",240,100,4);frame_.drawCentreString(t,240,145,2);panel_.present(static_cast<uint16_t*>(frame_.getPointer()));}
void DisplayUi::render(bool fix,uint32_t sats,float speed,const RadarCandidate& r,
 bool wifi,uint32_t count,bool muted,bool touch,bool audio,double lat,double lon,uint32_t chars){
 if(!available())return;auto* d=&frame_;const uint16_t bg=0x0843,panel=0x10a5,accent=0x05dd;
 d->fillSprite(bg);d->setTextColor(TFT_WHITE,bg);d->drawString("RADAR V3.12",12,8,2);
 d->setTextColor(fix?TFT_GREEN:TFT_ORANGE,bg);d->drawRightString(fix?String(sats)+" SAT":"BUSCANDO GPS",468,8,2);
 d->drawFastHLine(12,33,456,panel);
 if(info){ // Pantalla de diagnóstico: útil para cableado y primera recepción.
  d->setTextColor(TFT_WHITE,bg);d->drawString("FNK0104N / ST77922 QSPI + tactil",12,47,2);
  d->drawString(String("GPS: ")+String(lat,6)+", "+String(lon,6),12,77,2);
  d->drawString(String("NMEA recibidos: ")+chars,12,107,2);
  d->drawString(String("Base oficial: ")+count+" puntos",12,137,2);
  d->drawString(String("Tactil: ")+(touch?"OK":"NO")+"  Audio: "+(audio?"OK":"NO"),12,167,2);
  d->drawString(String("WiFi: ")+(wifi?"conectado":"sin conexion"),12,197,2);
  d->drawString(ProjectConfig::kFirmwareVersion,12,231,2);
 }else{ // Pantalla normal: velocidad, punto próximo y estado de aviso.
  d->fillRoundRect(12,45,220,158,8,panel);d->setTextColor(TFT_WHITE,panel);
  d->drawCentreString(fix?String((int)roundf(speed)):"--",122,66,8);
  d->drawCentreString("km/h GPS",122,160,4);
  d->setTextColor(accent,bg);d->drawString("PROXIMO PUNTO",248,48,2);
  d->setTextColor(TFT_WHITE,bg);d->drawString(r.valid?String((int)r.distanceMeters)+" m":"--",248,80,4);
  d->drawString(r.valid?(r.record.type==2?"TRAMO":"FIJO"):"Sin punto cercano",248,122,2);
  d->drawString(r.valid?String("Limite: ")+(r.record.speedKph?String(r.record.speedKph):"--"):"",248,160,4);
  d->drawString(!count?"BASE NO DISPONIBLE":muted?"AVISOS SILENCIADOS":!fix?"Esperando posicion GPS":r.valid&&r.inTravelDirection?"Punto por delante (*)":"Avisador preparado",12,218,2);
  d->setTextColor(0xad55,bg);d->drawString(String(count)+" puntos | (*) Sin ajuste a carretera",12,247,1);
 }
 // Las posiciones coinciden con PanelGeometry::buttonAt(). Mantener ambas
 // definiciones sincronizadas si se rediseña la interfaz.
 const char* labels[]={info?"VOLVER":muted?"SONIDO":"MUTE 5m","BRILLO",info?"TEST AUDIO":"INFO"};
 for(int i=0;i<3;++i){d->fillRoundRect(8+i*158,270,148,42,5,panel);d->setTextColor(TFT_WHITE,panel);d->drawCentreString(labels[i],82+i*158,283,2);}
 if(!panel_.present(static_cast<uint16_t*>(frame_.getPointer())))Serial.println("ERROR QSPI: reiniciar y revisar placa");
}
