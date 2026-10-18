/** @file Connectivity.cpp Wi-Fi/OTA opcional; está deshabilitado por defecto. */
#include "Connectivity.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "ProjectConfig.h"
namespace {bool enabled=false,ota=false;}
void Connectivity::begin(){
 // Las credenciales de marcador evitan activar la radio Wi-Fi accidentalmente.
 enabled=strlen(ProjectConfig::kWifiSsid)&&strcmp(ProjectConfig::kWifiSsid,"CONFIGURAR_SSID");
 if(!enabled){WiFi.mode(WIFI_OFF);return;}
 WiFi.mode(WIFI_STA);WiFi.begin(ProjectConfig::kWifiSsid,ProjectConfig::kWifiPassword);lastAttemptMs_=millis();
}
void Connectivity::update(){
 if(!enabled)return;
 // Reintento espaciado para no bloquear ni consumir energía continuamente.
 if(!connected()&&millis()-lastAttemptMs_>=30000){lastAttemptMs_=millis();WiFi.reconnect();}
 if(connected()&&strlen(ProjectConfig::kOtaPassword)){
  // OTA requiere además contraseña no vacía; así no queda expuesto por defecto.
  if(!ota){ArduinoOTA.setHostname("radar-freenove");ArduinoOTA.setPassword(ProjectConfig::kOtaPassword);ArduinoOTA.begin();ota=true;}
  ArduinoOTA.handle();
 }
}
bool Connectivity::connected()const{return enabled&&WiFi.status()==WL_CONNECTED;}
