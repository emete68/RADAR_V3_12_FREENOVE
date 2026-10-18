# Cableado y primera puesta en marcha

Referencia: esquema oficial Freenove de 3,5 pulgadas FNK0104N (19/05/2026), incluido en esta carpeta. Conector **P2 / UART**, cuatro contactos de señal/alimentación; las patillas 5 y 6 del símbolo son anclajes de masa.

| P2 Freenove, según esquema | Señal / GPIO | Conexión al breakout GPS |
|---|---|---|
| 1 | Alimentación derivada de +5 V | VCC, solo si el breakout admite 5 V |
| 2 | GND | GND |
| 3 | TXD0 → GPIO43 | RX del GPS, opcional; puede dejarse sin conectar |
| 4 | RXD0 → GPIO44 | TX del GPS, necesario |

Este firmware asigna expresamente **UART1 RX=44, TX=43**, respetando el esquema FNK0104N. Son GPIO distintos de los utilizados en la versión FNK0104B anterior. Los contactos de señal pasan por resistencias serie de 100 Ω. Comprueba la revisión real de tu placa por continuidad; una vista frontal del conector puede invertir visualmente el orden. No se asignan colores al mazo.

El conector de expansión IO es otro conector; no conectes allí el GPS siguiendo esta numeración. El USB utiliza GPIO19/20 y permanece disponible para programación/diagnóstico.

El ATGM336H-5N **módulo desnudo** y un **breakout con regulador** no tienen por qué admitir la misma tensión. Alimenta a 5 V únicamente un breakout especificado para ello. Verifica que su TX sea de 3,3 V; no apliques UART de 5 V al ESP32. Si tu GPS solo admite 3,3 V de alimentación, usa una alimentación regulada compatible y masa común. La propuesta económica presupone breakout con regulador, no el receptor desnudo.

## Secuencia de banco

1. Con la placa apagada, identifica P2 y GND por continuidad. Con USB conectado y sin GPS, mide la alimentación del contacto 1 respecto al 2. Desconecta antes de soldar.
2. Conecta antena al GPS, VCC/GND y TX del GPS a P2-4. Deja RX del GPS sin conectar para la primera prueba. El firmware espera NMEA a **9600, 8N1**.
3. Conecta el altavoz suministrado al puerto SPEAKER. Sus dos salidas son diferenciales: ninguna se une a masa.
4. Graba con `tools/flash.ps1`; este realiza copia del firmware de fábrica antes de escribir.
5. Comprueba pantalla horizontal 480×320, INFO y BRILLO. Pulsa cada botón, suéltalo y vuelve a pulsarlo; mantenerlo pulsado no debe repetir la acción. En INFO, TEST AUDIO debe sonar. El estado «OK» indica inicialización correcta del driver, no que se haya medido una salida sonora o táctil real.
6. Con antena al aire libre, el contador NMEA debe aumentar. La primera posición puede tardar varios minutos. La pantalla no debe mostrar velocidad válida antes de adquirir posición.
7. Comprueba MUTE y reactivación. Si no hay tacto, los comandos USB permiten diagnosticar audio/brillo. Si NMEA permanece a cero, revisa alimentación, masas, TX/RX y velocidad UART.

No hay modificación de la placa, batería, PPS, microSD ni cargador externo necesarios para esta configuración. La carga de batería integrada no se usa ni se ha validado para una celda concreta. La función principal usa GPS y base local sin Internet.

## Pines internos reservados

Pantalla QSPI: D0=11, D1=13, D2=14, D3=9, SCLK=12, CS=10, retroiluminación=41; reset LCD ligado a EN. El pin TE=42 no se utiliza. Táctil ST77922: SDA=38, SCL=39, RESET=48, IRQ=47, dirección I²C=0x55. Codec ES8311 comparte I²C, dirección0x18; MCLK=17, BCLK=18, WS=21, datos MCU→codec=15, amplificador=1 activo bajo. ADC batería=8 (no se usa sin batería). No reutilizar estos GPIO para accesorios.
