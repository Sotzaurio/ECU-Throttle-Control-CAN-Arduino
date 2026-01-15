ECU-Throttle-Control-CAN-Arduino
Descripción general

Este proyecto presenta una simulación educativa de una ECU (Electronic Control Unit) orientada al control electrónico del cuerpo de aceleración, inspirada en arquitecturas automotrices modernas. El sistema fue desarrollado utilizando Arduino Nano y comunicación CAN bus, replicando de forma didáctica el principio de operación de un sistema Drive-By-Wire.

La ECU simulada emplea una arquitectura distribuida, donde un nodo principal se encarga de la adquisición de señales, el procesamiento lógico y el control del actuador, mientras que un segundo nodo funciona como módulo receptor y de visualización, similar a un tablero de instrumentos.

El objetivo del proyecto es demostrar el funcionamiento básico de una ECU automotriz, el uso de CAN como red de comunicación y la integración de sensores, modos de operación y control electrónico del acelerador.

Arquitectura del sistema

El sistema está compuesto por dos nodos electrónicos, ambos basados en Arduino Nano y conectados mediante módulos CAN MCP2515:

Nodo transmisor (ECU principal)

Este nodo simula el comportamiento central de una ECU y realiza las siguientes funciones:

Lectura de un potenciómetro, que representa la consigna de aceleración.

Gestión de modos de conducción ECO y SPORT, seleccionados mediante botones físicos.

Adquisición de temperatura y humedad ambiental mediante un sensor DHT11.

Generación de una señal de control para el cuerpo de aceleración, accionado a través de un puente H.

Empaquetado y transmisión de datos a través del bus CAN.

Nodo receptor (módulo de visualización)

Este nodo actúa como un módulo remoto similar a un tablero automotriz y se encarga de:

Recepción de mensajes CAN provenientes del nodo ECU.

Decodificación de los datos recibidos.

Visualización del modo de conducción, porcentaje de apertura del acelerador, temperatura y humedad en una pantalla LCD I2C 16x2.

Comunicación CAN

La comunicación entre nodos se realiza mediante el protocolo CAN (Controller Area Network), ampliamente utilizado en aplicaciones automotrices por su robustez y confiabilidad.

Especificación del mensaje CAN

Identificador: 0x100

Longitud: 6 bytes

Payload:

Byte	Descripción
0–1	Temperatura (valor entero escalado ×10)
2–3	Humedad (valor entero escalado ×10)
4	Porcentaje de apertura del acelerador (0–100 %)
5	Modo de conducción (0 = ECO, 1 = SPORT)

El uso de valores escalados permite transmitir información con un decimal sin necesidad de tipos de datos flotantes en el bus.

Modos de operación

Modo ECO:
Diseñado para simular una respuesta más suave del acelerador, priorizando el control progresivo y la eficiencia.

Modo SPORT:
Simula una respuesta más directa del acelerador, permitiendo una apertura más agresiva ante la misma entrada del potenciómetro.

La selección del modo se transmite al nodo receptor para su visualización en tiempo real.

Lista de materiales (BOM)
Cantidad	Componente
2	Arduino Nano
2	Módulo CAN MCP2515
1	Sensor de temperatura y humedad DHT11
1	Pantalla LCD 16x2 con interfaz I2C
1	Potenciómetro
2	Botones pulsadores (ECO / SPORT)
1	Puente H para control del cuerpo de aceleración
2	Protoboard
—	Cables jumper macho-macho y macho-hembra
1	Cuerpo de aceleración (mariposa motorizada o equivalente)
