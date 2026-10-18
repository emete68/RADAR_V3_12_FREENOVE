/**
 * @file ST77922Panel.cpp
 * @brief Controlador QSPI mínimo para el ST77922 específico de la FNK0104N.
 *
 * La pantalla física es 320x480 vertical; la interfaz se dibuja como 480x320
 * horizontal. Cada franja se rota a un búfer DMA interno antes de transmitir.
 */
#include "ST77922Panel.h"
#include "ST77922Init.h"
#include "BoardPins.h"
#include "PanelGeometry.h"
#include <esp_heap_caps.h>
namespace {
// Ocho filas usan 5120 B: tamaño pequeño para RAM DMA interna y suficiente
// para no realizar 480 transacciones de una sola fila por cuadro.
constexpr size_t stripeRows=8,stripeBytes=320*stripeRows*2;
}
bool ST77922Panel::command(uint8_t cmd,const void* data,size_t length){
 // El protocolo del panel usa una fase QSPI de comando 0x02 y la orden LCD
 // real viaja en los bits altos de la dirección de 24 bits.
 spi_transaction_ext_t t={};t.base.flags=SPI_TRANS_VARIABLE_CMD|SPI_TRANS_VARIABLE_ADDR;
 t.base.cmd=0x02;t.base.addr=uint32_t(cmd)<<8;t.command_bits=8;t.address_bits=24;
 // Importante: una orden sin parámetros debe llevar puntero nulo. El driver
 // ESP-IDF rechaza un tx_buffer no nulo cuando la fase MOSI mide cero bits.
 t.base.tx_buffer=length?data:nullptr;t.base.length=length*8;
 digitalWrite(BoardPins::kLcdCs,LOW);auto err=spi_device_polling_transmit(device_,reinterpret_cast<spi_transaction_t*>(&t));digitalWrite(BoardPins::kLcdCs,HIGH);
 return err==ESP_OK;
}
bool ST77922Panel::begin(){
 pinMode(BoardPins::kLcdCs,OUTPUT);digitalWrite(BoardPins::kLcdCs,HIGH);
 pinMode(BoardPins::kTftBacklight,OUTPUT);digitalWrite(BoardPins::kTftBacklight,LOW);
 // PSRAM no es una fuente DMA QSPI segura para este camino; la franja vive
 // explícitamente en RAM interna compatible con DMA.
 stripe_=static_cast<uint16_t*>(heap_caps_malloc(stripeBytes,MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL));if(!stripe_)return false;
 spi_bus_config_t bus={};bus.mosi_io_num=BoardPins::kLcdD0;bus.miso_io_num=BoardPins::kLcdD1;
 bus.sclk_io_num=BoardPins::kLcdClock;bus.quadwp_io_num=BoardPins::kLcdD2;bus.quadhd_io_num=BoardPins::kLcdD3;
 // La placa es QSPI de cuatro líneas. Los campos octales sobrantes deben ser
 // -1, nunca 0: GPIO0 es BOOT y un cero implícito rompería el arranque.
 bus.data4_io_num=-1;bus.data5_io_num=-1;bus.data6_io_num=-1;bus.data7_io_num=-1;
 bus.max_transfer_sz=stripeBytes;bus.flags=SPICOMMON_BUSFLAG_MASTER|SPICOMMON_BUSFLAG_QUAD;
 auto result=spi_bus_initialize(SPI2_HOST,&bus,SPI_DMA_CH_AUTO);
 Serial.printf("QSPI bus: %s\n",esp_err_to_name(result));
 if(result!=ESP_OK){free(stripe_);stripe_=nullptr;return false;}
 // CS es manual porque permanece activo durante el flujo completo de píxeles.
 spi_device_interface_config_t dev={};dev.clock_speed_hz=40000000;dev.mode=0;dev.spics_io_num=-1;
 dev.flags=SPI_DEVICE_HALFDUPLEX;dev.queue_size=1;
 result=spi_bus_add_device(SPI2_HOST,&dev,&device_);
 Serial.printf("QSPI device: %s\n",esp_err_to_name(result));
 if(result!=ESP_OK){spi_bus_free(SPI2_HOST);free(stripe_);stripe_=nullptr;return false;}
 // La tabla no se modifica: procede del driver oficial Freenove para ST77922.
 for(const auto& c:kLcdInit){if(!command(c.cmd,c.data,c.len))return false;if(c.delayMs)delay(c.delayMs);}
 ready_=true;return true;
}
bool ST77922Panel::present(const uint16_t* source){
 if(!ready_||!source)return false;
 // Ventana completa: 320 columnas (0..319) por 480 filas (0..479).
 uint8_t xs[]={0,0,1,0x3f},ys[]={0,0,1,0xdf};
 if(!command(0x2a,xs,4)||!command(0x2b,ys,4)){ready_=false;return false;}
 // Manual CS stays asserted throughout the full pixel stream. DMA always reads
 // the fixed internal stripe buffer, never external PSRAM directly.
 digitalWrite(BoardPins::kLcdCs,LOW);bool ok=true;
 for(int row=0;row<480;row+=stripeRows){
  // Convierte la orientación del sprite horizontal al orden físico vertical.
  PanelGeometry::rotateStripe(source,stripe_,row,stripeRows);
  spi_transaction_ext_t t={};t.base.flags=SPI_TRANS_MODE_QIO|SPI_TRANS_VARIABLE_CMD|SPI_TRANS_VARIABLE_ADDR|SPI_TRANS_VARIABLE_DUMMY;
  // Primera franja: 0x32 inicia escritura QIO con subcomando RAMWR (0x3C).
  // Las siguientes continúan la misma transferencia con CS aún bajo.
  if(row==0){t.base.cmd=0x32;t.base.addr=0x3c00;t.command_bits=8;t.address_bits=24;}
  t.base.tx_buffer=stripe_;t.base.length=stripeBytes*8;
  if(spi_device_polling_transmit(device_,reinterpret_cast<spi_transaction_t*>(&t))!=ESP_OK){ok=false;break;}
 }
 digitalWrite(BoardPins::kLcdCs,HIGH);ready_=ok;return ok;
}

