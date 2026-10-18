/** @file ProjectConfig.h Ajustes de producto sin lógica ni E/S. */
#pragma once
namespace ProjectConfig {
constexpr char kProductName[]="RADAR V3.12";
constexpr char kFirmwareVersion[]="3.12.2-fnk0104n";
// Mantener estos valores de marcador desactiva Wi-Fi. Solo establecer datos
// reales si se desea red/OTA; la base de radares funciona completamente offline.
constexpr char kWifiSsid[]="CONFIGURAR_SSID";
constexpr char kWifiPassword[]="CONFIGURAR_PASSWORD";
constexpr char kOtaPassword[]="";
// Parámetros que equilibran aviso temprano y falsos avisos por dirección.
constexpr unsigned long kGnssBaud=9600;
constexpr unsigned long kUiRefreshMs=250, kSearchRefreshMs=1000;
constexpr float kAlertDistanceMeters=1000, kUrgentDistanceMeters=300;
constexpr float kHeadingToleranceDegrees=55, kMinimumSpeedKph=5;
// El cargador integrado no se ha validado con una celda en este proyecto.
constexpr bool kBatteryInstalled=false;
}
