# Procedencia y condiciones

Datos normalizados para este avisador, sin relación de patrocinio o certificación de los organismos. No se han usado bases comerciales, colaborativas ni registros sintéticos en `data/radars.bin`.

- **Dirección General de Tráfico (DGT)**: [catálogo NAP](https://nap.dgt.es/dataset/radares-fijos-dgt), [recurso DATEX2](https://infocar.dgt.es/datex2/dgt/PredefinedLocationsPublication/radares/content.xml). Catálogo con licencia Creative Commons Attribution; [aviso legal DGT](https://www.dgt.es/contenido/aviso-legal/). Publicación interna del XML: 18/12/2025. Incluye puntos de cabinas y extremos de tramos. Los límites no aparecen en el XML y se mantienen desconocidos.
- **Generalitat de Catalunya. Departament d'Interior i Seguretat Pública. Servei Català de Trànsit**: [radars.txt](https://transit.gencat.cat/web/.content/documents/seguretat_viaria/radars.txt), [catálogo](https://datos.gob.es/ca/catalogo/a09002970-radares-fijos-de-cataluna), [condiciones de reutilización](https://web.gencat.cat/ca/generalitat/dades-indicadors/dades-obertes/llicencies). Conversión de ETRS89 UTM zona31N a longitud/latitud WGS84 con PROJ/pyproj. No se han reparado errores tipográficos de coordenadas. El recurso no declara fecha interna de actualización.
- **Gobierno Vasco. Departamento de Seguridad. Dirección de Tráfico**: [cabinas de radar fijo](https://apps.trafikoa.euskadi.eus/lfr/es/web/trafikoa/cabinas-de-radar-fijo). Coordenadas y atributos de los puntos del mapa, no geocodificación de nombres. [Constante oficial de proyección](https://apps.trafikoa.euskadi.eus/gis/bisorearina/js/constantes.js) define ETRS89 como EPSG25830. [Información legal](https://www.euskadi.eus/informacion-legal/web01-sede/es/). No se atribuye a este recurso una licencia CC no declarada. El recurso no declara fecha interna de actualización.

Descarga de los tres recursos: 14/09/2026. Archivos fuente, SHA256 y exclusiones en `validation/official/` y `database/audit.json`. Transformaciones: conversión de coordenadas, selección de campos, IDs compactos, retirada de duplicados y descarte de posiciones fuera de rango. Se conservan carretera, PK, nombre e identificación originales en el CSV. Un PK de DGT se conserva en metros tal como aparece en DATEX2; el de SCT/Euskadi conserva el texto original. No se deduce rumbo de nombres de destinos o sentido creciente/decreciente.

## Hardware y bibliotecas

Documentación oficial: [Freenove ESP32-S3 Display](https://github.com/Freenove/Freenove_ESP32_S3_Display), commit `90004f874ed567b3c09d534d021172cb716e61a1`. Esquema y ejemplos bajo las condiciones originales incluidas en `Freenove-LICENSE.txt`. Driver ES8311 de los ejemplos, con cabeceras SPDX Apache-2.0; cambios locales: adaptación de includes a Arduino2/IDF4, reloj de16kHz y comprobación de memoria. No se presenta como código original de este proyecto.

TFT_eSPI 2.5.43 (Bodmer), TinyGPSPlus 1.1.0 (Mikal Hart), Arduino ESP32/Espressif y PlatformIO conservan sus respectivas licencias. Copias de licencias de bibliotecas y núcleo en `docs/licenses/`. Las dependencias de compilación se obtienen mediante PlatformIO, no están vendidas ni sublicenciadas como propiedad de este proyecto.


## Adaptación FNK0104N

Esquema específico: [3.5inch_ESP32-S3_Display_Schematic.pdf](https://github.com/Freenove/Freenove_ESP32_S3_Display/blob/90004f874ed567b3c09d534d021172cb716e61a1/Schematic/3.5inch_ESP32-S3_Display_Schematic.pdf), fechado 19/05/2026.

La tabla de `include/ST77922Init.h` procede, byte por byte, de `ST77922.cpp` dentro de `Libraries/FNK0104N/TFT_eSPI_v2.5.43.zip` en ese commit. Se conserva la licencia Freenove. El transporte QSPI local inicializa en `setup()`, informa de fallos, mantiene CS durante el envío y rota en franjas DMA; no usa los constructores globales ni las reservas por fotograma del ejemplo.

Táctil: protocolo ST77922 TDDI Interface Protocol V01.00, registros I²C de 16 bits, dirección0x55. Se leen todos los contactos soportados para reconocer el informe; se limita a10 y la espera de arranque tiene tiempo máximo. No se reutiliza FT6336 de la placa anterior.

La base oficial se conserva exactamente como en la entrega anterior. Esta adaptación de hardware no constituye una actualización de las fuentes de radares.
