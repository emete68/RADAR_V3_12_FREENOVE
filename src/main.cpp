/**
 * @file main.cpp
 * @brief Orquestador principal del avisador RADAR V3.12.
 *
 * La aplicación no usa `delay()` para temporizaciones funcionales. La única
 * pausa breve al final de loop() cede CPU a Wi-Fi, USB y tareas FreeRTOS.
 * Cada servicio se actualiza por separado, de forma que la UI nunca impide
 * que se vacíe el UART del GPS.
 */
#include <Arduino.h>
#include <Wire.h>
#include "BoardPins.h"
#include "Buzzer.h"
#include "Connectivity.h"
#include "DisplayUi.h"
#include "GnssService.h"
#include "ProjectConfig.h"
#include "RadarDatabase.h"
#include "TouchInput.h"
#include "PanelGeometry.h"
namespace {
// Instancias únicas: se construyen antes de setup(), pero sus periféricos se
// inicializan explícitamente en setup(), cuando ya hay reloj y alimentación.
GnssService gnss; RadarDatabase database; Connectivity connectivity;
DisplayUi ui; Buzzer buzzer; TouchInput touch; RadarCandidate nearest;
// Marcas de tiempo en milisegundos. `millis()` se desborda de forma segura
// para las restas sin signo usadas más abajo.
uint32_t searched=0,drawn=0,bootChanged=0; bool bootDown=false;
// El botón MUTE y el comando USB `m` son conmutadores: cinco minutos de
// silencio o cancelación inmediata del silencio ya activo.
void mute(){if(buzzer.muted())buzzer.unmute();else buzzer.silenceFor(300000);}
}
void setup(){
 // USB CDC sirve tanto para diagnóstico como para enviar comandos de banco.
 Serial.begin(115200);delay(150);pinMode(BoardPins::kBoot,INPUT_PULLUP);
 Serial.printf("Arranque %s PSRAM=%u libre=%u\n",ProjectConfig::kFirmwareVersion,ESP.getPsramSize(),ESP.getFreePsram());
 // Táctil y códec de audio comparten este I2C. 100 kHz prioriza robustez de
 // cableado frente a la velocidad: ninguna operación de esta aplicación exige
 // un bus más rápido.
 Wire.begin(BoardPins::kI2cSda,BoardPins::kI2cScl,100000);Wire.setTimeOut(50);
 // La pantalla se inicializa antes de encender la retroiluminación, evitando
 // mostrar basura durante el arranque.
 ui.begin();ui.showBoot("Inicializando RADAR...");
 Serial.printf("Pantalla inicializada=%d\n",ui.available());
 // Estos servicios degradan de forma independiente: por ejemplo, sin GPS la
 // interfaz y las pruebas de sonido continúan disponibles.
 touch.begin();buzzer.begin();gnss.begin();connectivity.begin();database.begin();
 Serial.printf("RADAR %s: base=%lu tactil=%d audio=%d PSRAM=%u\n",ProjectConfig::kFirmwareVersion,(unsigned long)database.count(),touch.available(),buzzer.available(),ESP.getPsramSize());
 Serial.println("FNK0104N. USB: m silencio; a prueba audio; i informacion; b brillo");
}
void loop(){
 // Primero se consumen NMEA y red: así no se pierden caracteres aunque el
 // dibujo de la pantalla se ejecute después.
 const uint32_t now=millis();gnss.update();connectivity.update();
 // Diagnóstico periódico útil con monitor serie. No modifica el estado.
 static uint32_t lastDiagnostic=0;
 if(now-lastDiagnostic>=5000){lastDiagnostic=now;Serial.printf("DIAG uptime=%lu pantalla=%d tactil=%d audio=%d PSRAM=%u base=%lu NMEA=%lu\n",(unsigned long)now,ui.available(),touch.available(),buzzer.available(),ESP.getPsramSize(),(unsigned long)database.count(),(unsigned long)gnss.characters());}
 // `pressed` solo informa del flanco de una pulsación válida, no del contacto
 // mantenido; evita repetir acciones mientras se mantiene el dedo.
 int x,y;if(touch.pressed(x,y)){
  const int button=PanelGeometry::buttonAt(x,y);
  if(button==0){if(ui.info)ui.info=false;else mute();}
  else if(button==1)ui.brightness();
  else if(button==2){if(ui.info)buzzer.test();else ui.info=true;}
 }
 // El botón físico BOOT es una alternativa de emergencia para silenciar.
 // Se aplica un antirrebote temporal sencillo de 40 ms.
 bool down=digitalRead(BoardPins::kBoot)==LOW;
 if(down!=bootDown&&now-bootChanged>=40){bootDown=down;bootChanged=now;if(down)mute();}
 // Comandos de banco por USB: m=mute, a=audio, i=información, b=brillo.
 while(Serial.available()){switch(Serial.read()){case 'm':mute();break;case 'a':buzzer.test();break;case 'i':ui.info=!ui.info;break;case 'b':ui.brightness();break;}}
 // No se conserva un radar anterior cuando se pierde la posición GNSS.
 if(!gnss.hasFix())nearest=RadarCandidate{};
 // La búsqueda se limita a una vez por segundo: reduce cálculos geográficos
 // y mantiene la respuesta de la pantalla uniforme.
 if(gnss.hasFix()&&now-searched>=ProjectConfig::kSearchRefreshMs){
  searched=now;nearest=database.findNearest(gnss.latitude(),gnss.longitude(),gnss.courseDegrees(),ProjectConfig::kAlertDistanceMeters*2);
 }
 // Solo se alerta si existe posición y rumbo recientes, el vehículo supera
 // la velocidad mínima y el punto queda por delante dentro del alcance.
 bool alert=gnss.hasFix()&&gnss.courseAvailable()&&gnss.speedKph()>=ProjectConfig::kMinimumSpeedKph&&nearest.valid&&nearest.inTravelDirection&&nearest.distanceMeters<=ProjectConfig::kAlertDistanceMeters;
 // test() can run after the loop timestamp; use a fresh time to avoid
 // unsigned elapsed-time underflow cancelling the tone immediately.
 buzzer.update(alert,alert&&nearest.distanceMeters<=ProjectConfig::kUrgentDistanceMeters,millis());
 // La UI se refresca a 4 fps. El búfer completo se manda por QSPI en cada
 // cuadro; el ritmo elegido deja margen al parser NMEA y al audio.
 if(now-drawn>=ProjectConfig::kUiRefreshMs){drawn=now;ui.render(gnss.hasFix(),gnss.satellites(),gnss.speedKph(),nearest,connectivity.connected(),database.count(),buzzer.muted(),touch.available(),buzzer.available(),gnss.latitude(),gnss.longitude(),gnss.characters());}
 delay(2);
}
