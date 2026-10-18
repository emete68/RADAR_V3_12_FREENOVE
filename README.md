# RADAR V3.12 — Freenove FNK0104N táctil

> **Avisador GPS basado en información pública.** Este proyecto utiliza
> bases de datos oficiales de radares publicadas por organismos de tráfico.
> Se ha diseñado para ser lo más sencillo posible de montar, grabar, usar y
> actualizar, siempre respetando la normativa aplicable durante la conducción.

> **Copia comentada (18/10/2026):** este directorio incluye explicaciones
> detalladas del código propio y un binario recompilado que coincide con la
> fuente incluida. Empieza por `docs/GUIA_CODIGO_COMENTADO.md`.

Proyecto para la **Freenove ESP32-S3 3,5 pulgadas capacitiva FNK0104N**, pantalla ST77922 320×480, presentada horizontalmente a 480×320, 16 MB de flash y 8 MB de PSRAM. Firmware 3.12.2-fnk0104n. Específico para FNK0104N: no cargar en FNK0104B de 2,8 pulgadas, FNK0104A sin táctil ni otras placas.

Incluye fuentes PlatformIO, binarios de grabación, base de 1.075 puntos oficiales, originales descargados, auditoría y pruebas. El proyecto anterior de PCB propia y la versión FNK0104B se conservan en sus carpetas originales.

**Estado:** compilado y comprobado por software. El 16/09/2026 se corrigieron el modo de grabación del cargador y los comandos QSPI sin datos. La placa real arranca e informa pantalla=1, táctil=1, audio=1 y 1.075 registros. El usuario confirma que la pantalla funciona. También se corrigió el temporizador que podía cancelar inmediatamente TEST AUDIO. El usuario confirma que TEST AUDIO suena tras la corrección. Pendiente prueba del GPS.

## Montaje

Material: FNK0104N con altavoz y cable USB; módulo GPS ATGM336H-5N con antena y breakout compatible con alimentación de 5 V y UART de 3,3 V; cable de cuatro vías UART incluido con la pantalla. Alimentación inicial mediante USB-C. No hace falta fabricar la PCB anterior, añadir botones, tarjeta microSD ni conectar batería.

Ver [cableado y puesta en marcha](docs/CABLEADO.md). Antes de conectar, identifica por continuidad los contactos del conector: no te guíes por los colores del cable.

## Grabar

Descomprime todo el ZIP. Conecta únicamente la Freenove por USB-C. Si no aparece un puerto, mantén BOOT pulsado, pulsa RESET, suelta RESET y después BOOT. Consulta el COM en el Administrador de dispositivos.

Desde PowerShell, situado en esta carpeta:

```powershell
.\tools\flash.ps1 -Port COM5
```

Sustituye COM5 por tu puerto. El script utiliza PlatformIO local, copia primero los 16 MB de flash a `backups/`, y solo si la copia termina correctamente graba los binarios. No requiere recompilar. Al finalizar pulsa RESET si la placa permanece en modo descarga. Conserva la copia para recuperar el programa de fábrica.

Corrección del 16/09/2026: el script conserva los parámetros de los binarios (`--flash_mode keep`). El cargador arranca en DIO y habilita QIO después. Forzar QIO durante la grabación modificaba su cabecera y causaba reinicios `ets_loader.c 78` antes de ejecutar el programa. No se ha cambiado la aplicación ni la base de radares para resolver este fallo.

## Uso

- **MUTE 5m / SONIDO:** silenciar cinco minutos o reactivar; BOOT también alterna silencio.
- **BRILLO:** tres niveles de retroiluminación.
- **INFO:** recepción NMEA, posición, número de puntos y estado de inicialización de táctil/audio. **TEST AUDIO** genera un tono breve, incluso si está silenciado.
- Por USB a 115200: `m` silencio, `a` prueba audio, `i` información, `b` brillo.

Avisos por proximidad a 1.000 m, más frecuentes a 300 m; requieren posición y rumbo GPS recientes y velocidad ≥5 km/h. Las distancias son en línea recta. El filtro de rumbo de ±55° reduce avisos por detrás, pero **no identifica la carretera ni el carril**: puede avisar de vías paralelas. El siguiente punto puede mostrarse aunque todavía no cumpla la condición de aviso.

Los tramos se muestran como puntos publicados; no se calcula velocidad media. No incluye radares móviles en tiempo real. No muestra límites inventados: `--` significa desconocido. La base es una descarga concreta, no una garantía de cobertura completa o de actualidad.

## Datos oficiales

| Fuente | Puntos incluidos | Observaciones |
|---|---:|---|
| DGT | 746 | XML publicado 18/12/2025; no contiene límites de velocidad |
| Generalitat de Catalunya, Servei Català de Trànsit | 231 | ETRS89 UTM31N; 17 filas anómalas excluidas |
| Gobierno Vasco, Dirección de Tráfico | 98 | Coordenadas del mapa oficial, ETRS89 UTM30N |
| Total | 1.075 | Después de retirar 38 duplicados de posición y atributos |

Descarga: 14/09/2026. En los recursos SCT/Euskadi usados no se ha obtenido una fecha de actualización del contenido verificable. Las fechas del catálogo o de descarga no se presentan como fecha de cada radar. Consulta [fuentes y condiciones](docs/FUENTES.md), `database/official.csv` y `database/audit.json`. La tabla cuenta puntos, no necesariamente cinemómetros físicos; un tramo puede aportar dos extremos.

## Compilar y actualizar

La pantalla utiliza el controlador QSPI local y la tabla de inicialización oficial de Freenove. TFT_eSPI se usa únicamente para dibujar en memoria; no inicializa el bus de la pantalla. La rotación horizontal se realiza en franjas con un búfer DMA fijo de 5.120 bytes, sin reservar memoria en cada refresco. Frecuencia QSPI: 40 MHz. Actualización de interfaz: cada 250 ms; no es una interfaz de vídeo y la ausencia de tearing debe comprobarse físicamente.

PlatformIO 6.1.19, plataforma espressif32 7.1.3, Arduino ESP32 2.0.17, TFT_eSPI 2.5.43 y TinyGPSPlus 1.1.0. Se descargan las dependencias fijadas al compilar:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -t buildfs
```

Los binarios recién compilados están en `.pio/build/freenove_fnk0104n/`. El script `flash.ps1` graba los entregados en `bin/` y verifica sus hashes; después de recompilar usa `platformio run -t upload --upload-port COM5` y `platformio run -t uploadfs --upload-port COM5`.

Para actualizar la base desde el PC, con Python 3.11 y pyproj 3.7.2:

```powershell
python -m pip install pyproj==3.7.2
python tools/import_official.py --download
python tools/build_radar_db.py database/official.csv
```

Revisa el nuevo `database/audit.json`, compila LittleFS y grábalo mediante `uploadfs`. El importador no repara cifras dudosas. Las pruebas incluyen valores de esta descarga de referencia; una actualización legítima puede requerir revisar las expectativas de recuento y fecha.

WiFi está desactivado por defecto. Para habilitarlo configura `include/ProjectConfig.h` y recompila. OTA solo se inicia si se configura además una contraseña no vacía. No hay descarga automática de radares desde el dispositivo.

## Validación entregada

`validation/build.log`, `validation/filesystem-build.log`, `validation/tests.log`, auditoría de fuentes y hashes. Pruebas de conversión, rechazo de coordenadas inválidas, correspondencia CSV/binario, CRC32, identificación de puntos y conservación del firmware anterior. Además se ejecuta el código C++ real de geometría: 153.600 posiciones, inversión táctil, franjas de rotación, límites de botones y pulsaciones únicas/múltiples. `validation/panel-tests.log` contiene el resultado. No equivalen a ensayos del hardware ni a una prueba de circulación.


## Actualizar la base en el ordenador y cargar por USB

Usa esta carpeta FNK0104N, no la carpeta de la versión de 2,8 pulgadas. Instala `pyproj==3.7.2` en el Python con el que ejecutes los scripts. Conserva una copia de `data/` y `database/` antes de descargar.

```powershell
$pythonRadar = "$env:USERPROFILE\.platformio\penv\Scripts\python.exe"
$pioRadar = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
& $pythonRadar -m pip install pyproj==3.7.2
& $pythonRadar tools/import_official.py --download
# Si termina bien, revisa database/audit.json antes del paso siguiente.
& $pythonRadar tools/build_radar_db.py database/official.csv
& $pioRadar run -t uploadfs --upload-port COM5
```

COM5 es un ejemplo. `uploadfs` construye y carga los datos nuevos sin reemplazar el firmware. Si un paso falla, no continúes con la grabación. `flash.ps1` sirve para instalar los binarios entregados, no para subir una base recién actualizada. El servicio de descarga WiFi sigue aplazado.
