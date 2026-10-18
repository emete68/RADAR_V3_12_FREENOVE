# Guía del código comentado — RADAR V3.12 Freenove

Esta copia es la versión didáctica del firmware probado sobre la **Freenove
FNK0104N**, pantalla táctil ST77922 de 3,5 pulgadas, GPS ATGM336H y altavoz
incluido. El código ejecutable conserva el comportamiento de la entrega final;
se han añadido comentarios en español y esta guía. No es una nueva versión de
firmware ni modifica los binarios incluidos en `bin/`.

## Recorrido de ejecución

`src/main.cpp` crea los servicios y, en `setup()`, habilita USB, I2C, pantalla,
táctil, audio, GPS, conectividad y base local. En `loop()` realiza siempre esta
secuencia: vaciar NMEA, gestionar Wi‑Fi, leer un toque, atender el botón BOOT,
aceptar comandos USB, buscar el punto próximo, actualizar el tono y pintar la
UI. Los dos ritmos configurables están en `include/ProjectConfig.h`: búsqueda
cada 1000 ms y pantalla cada 250 ms.

```
GPS UART ──> GnssService ──┐
Base LittleFS ─> RadarDatabase ─> candidato ─> Buzzer / DisplayUi
Táctil I2C ───────────────────────────────────────> acciones de UI
```

## Módulos propios

| Ruta | Responsabilidad |
|---|---|
| `src/main.cpp` | Coordina periféricos, alertas, botones y diagnóstico USB. |
| `src/GnssService.cpp` | Recibe NMEA a 9600 8N1 en GPIO44 y expone datos frescos. |
| `src/RadarDatabase.cpp` | Valida CRC y carga `/radars.bin` en PSRAM; busca el candidato. |
| `src/GeoMath.cpp` | Distancia Haversine, rumbo inicial y diferencia angular. |
| `src/DisplayUi.cpp` | Dibuja en sprite RGB565 horizontal y muestra estados. |
| `src/ST77922Panel.cpp` | Transforma el sprite y lo transmite por QSPI al panel. |
| `src/TouchInput.cpp` | Lee el táctil 0x55 y genera una acción por toque. |
| `src/Buzzer.cpp` | Genera tono PCM en una tarea FreeRTOS para el ES8311. |
| `src/Connectivity.cpp` | Wi‑Fi/OTA opcional, desactivado con valores de marcador. |
| `src/PowerMonitor.cpp` | Base para batería futura; no se usa con la configuración actual. |

`lib/es8311/` es el driver del códec, procedente de la fuente indicada por su
licencia. Se conserva como código de tercero: se documenta desde `Buzzer.cpp`
en lugar de editar masivamente el proveedor y dificultar comparaciones futuras.
La tabla `include/ST77922Init.h` también se conserva literal porque cada byte
configura el panel y procede del fabricante.

## Pantalla y orientación

La pantalla física es 320×480, pero el firmware muestra 480×320. `TFT_eSPI`
se utiliza solamente como lienzo y motor de fuentes, sin llamar a `init()`.
`ST77922Panel` es el único dueño de los pines QSPI. Para no enviar PSRAM por
DMA, copia ocho filas rotadas a una franja de 5120 bytes en RAM interna y la
transmite. `PanelGeometry.h` comparte la misma transformación para imagen y
táctil; modificar una sin la otra invierte o desplaza los botones.

## Avisos

La búsqueda no hace ajuste a carretera. Primero descarta por una ventana
rectangular, después calcula distancia/ruta para los candidatos cercanos.
Solo se activa tono cuando hay posición y rumbo de menos de tres segundos,
velocidad igual o superior a 5 km/h, un punto a menos de 1000 m y el punto está
en el sentido de marcha. Los puntos tipo `tramo` se identifican, pero esta
versión no calcula velocidad media: faltaría representar entrada, salida y
recorrido del tramo como una entidad completa.

## Base de radares y actualización por USB

El flujo es deliberadamente revisable:

1. `tools/import_official.py` descarga o reutiliza las fuentes oficiales de
   DGT, Servei Català de Trànsit y Euskadi; normaliza coordenadas y escribe
   `database/official.csv` más `audit.json`.
2. `tools/build_radar_db.py database/official.csv` valida columnas, empaqueta
   registros de 28 bytes y crea `data/radars.bin` con CRC32.
3. PlatformIO empaqueta `data/` como LittleFS. `tools/flash.ps1 -Port COM6`
   realiza primero una copia de 16 MB de la placa y después escribe bootloader,
   particiones, firmware y LittleFS.

La base actual tiene 1.075 registros. Revisa `database/audit.json` y los datos
normalizados antes de sustituirla. El script no debe inventar límites, rumbos
ni reparar coordenadas de una fuente oficial.

## Diagnóstico de banco

El USB imprime un resumen cada cinco segundos. Los comandos son `m` para
silenciar/reanudar, `a` para tono de prueba, `i` para información y `b` para
brillo. El contador NMEA aumenta antes de que exista posición; una posición y
rumbo se consideran caducados tras tres segundos sin datos.

## Cambios seguros y zonas sensibles

- Cambiar constantes de UI, distancias o Wi‑Fi en `ProjectConfig.h`.
- Mantener `BoardPins.h` vinculado al esquema FNK0104N; no reutilizar GPIO QSPI,
  I2C, I2S ni UART GPS.
- No forzar `--flash_mode qio` al grabar. El ROM del ESP32-S3 carga el
  bootloader con DIO; éste ya cambia el modo necesario después.
- No cambiar los bytes de `ST77922Init.h` sin contrastarlos con el driver y
  documentación de Freenove.
- `partitions.csv` reserva dos aplicaciones OTA de 3 MB y LittleFS desde
  0x610000. Alterarlo invalida los offsets usados por el paquete de grabación.

## Verificación incluida

`validation/build.log`, `tests.log` y `panel-tests.log` son evidencias de la
entrega original. Para recompilar con PlatformIO local:

```powershell
pio run -d C:\ruta\RADAR_V3_12_FREENOVE_COMENTADO
```

Para grabar, usar solo el script `tools/flash.ps1`, indicando el puerto real.
Su copia previa de flash permite recuperar la placa de fábrica.
