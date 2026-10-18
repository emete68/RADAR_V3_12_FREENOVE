/** @file PowerMonitor.cpp Monitor preparado para una batería futura, no instalada. */
#include "PowerMonitor.h"
#include <Arduino.h>
#include "BoardPins.h"
#include "ProjectConfig.h"
void PowerMonitor::begin(){analogReadResolution(12);analogSetPinAttenuation(BoardPins::kBatterySense,ADC_11db);}
PowerState PowerMonitor::read() const {
 PowerState s;
 if(ProjectConfig::kBatteryInstalled){
  // Media de ocho lecturas para reducir ruido del ADC antes de dividir el
  // divisor resistivo de la placa.
  uint32_t mv=0;for(int i=0;i<8;i++)mv+=analogReadMilliVolts(BoardPins::kBatterySense);
  s.batteryVolts=mv*0.00025f;s.batteryPercent=constrain((s.batteryVolts-3.2f)*100.f,0.f,100.f);
 }
 return s; // La FNK0104N no expone señales de carga/Power Good a este código.
}
