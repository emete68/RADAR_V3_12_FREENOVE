# Revisión comentada

Fecha de preparación: 18/10/2026.

Esta carpeta es una copia didáctica de `radar_freenove_fnk0104n`, creada para
estudiar y mantener el proyecto. Se han comentado ampliamente los módulos de
autoría del proyecto y se añadió `docs/GUIA_CODIGO_COMENTADO.md`.

Se conservan sin reescribir los componentes de tercero:

- `lib/es8311/`: driver del códec, con su licencia original.
- `include/ST77922Init.h`: tabla de bytes del panel Freenove; cambiarla sin una
  fuente técnica verificable puede dejar la pantalla negra.
- Dependencias gestionadas por PlatformIO y binarios de `bin/`.

La fuente compilable incorpora solamente comentarios; no cambia la lógica del
firmware respecto a la entrega probada. Los binarios originales permanecen como
referencia hasta que se genere un paquete firmado de esta copia comentada.
