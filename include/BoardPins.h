/** @file BoardPins.h Mapa único de señales de la Freenove FNK0104N. */
#pragma once
namespace BoardPins {
// Entradas/servicios de placa: no reutilizar estos GPIO para accesorios.
constexpr int kBoot=0,kBatterySense=8,kTftBacklight=41;
// I2C compartido por táctil ST77922 y códec ES8311.
constexpr int kI2cSda=38,kI2cScl=39,kTouchReset=48,kTouchIrq=47;
// Audio: enable es activo bajo. DOUT=15 es ESP32 -> códec, no intercambiar.
constexpr int kAudioEnable=1,kI2sMclk=17,kI2sBclk=18,kI2sWs=21,kI2sOut=15;
// FNK0104N schematic: P2-4 RXD0=GPIO44; P2-3 TXD0=GPIO43.
constexpr int kGnssRx=44,kGnssTx=43;
// Bus QSPI exclusivo de la pantalla ST77922.
constexpr int kLcdCs=10,kLcdClock=12,kLcdD0=11,kLcdD1=13,kLcdD2=14,kLcdD3=9;
}
