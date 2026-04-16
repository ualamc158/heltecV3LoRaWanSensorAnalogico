# Heltec V3 LoRaWAN Bridge - Transmisor de Presión

Este repositorio contiene el firmware para el **Heltec WiFi LoRa 32 (V3)** configurado como un **Nodo de Transmisión (Bridge)**. Su función principal es actuar como puente transparente: recibe datos de presión vía UART desde un microcontrolador externo y los sube inmediatamente a la red LoRaWAN (ChirpStack/TTN) utilizando la librería **RadioLib**.

> [!IMPORTANT]
> **Dependencia de Sistema:** Este nodo no genera las lecturas de presión por sí mismo. Requiere estar conectado físicamente al **[Nodo Sensor (ESP32-S3)](https://github.com/ualamc158/esp32lectorsensoranalogicopresion)**, el cual se encarga de la lectura analógica, calibración y filtrado.

## 📡 Lógica de Funcionamiento (Arquitectura Push)

En esta versión (rama `v2_lorawan-node`), el sistema utiliza una comunicación proactiva por parte del sensor para optimizar la respuesta:

1.  **Conexión LoRaWAN:** El Heltec inicia la sesión mediante OTAA en la banda **EU868**. Si la conexión se pierde, el dispositivo está programado para intentar un reinicio limpio.
2.  **Modo Escucha:** El Heltec permanece a la escucha en su puerto UART (Serial). No solicita datos activamente, simplemente procesa lo que llega por el cable.
3.  **Transmisión Inmediata:** Cada 30 segundos, el Nodo Sensor envía la cadena de texto (ej. `P:10.50\n`). El Heltec captura el mensaje y utiliza la función `node.sendReceive` de RadioLib para enviarlo a la red LoRaWAN al instante.
4.  **Sincronización de Logs:** Los mensajes por consola incluyen marcas de tiempo reales sincronizadas con el momento de la compilación.

## 🔌 Conexiones Físicas

Para que la comunicación serie sea estable, es obligatorio conectar ambos dispositivos a una **masa común**.

| Heltec V3 (Bridge LoRa) | ESP32-S3 (Nodo Sensor) | Función |
| :--- | :--- | :--- |
| **GND** | **GND** | **Masa Común** (Indispensable) |
| **GPIO 35** (RX) | **GPIO 17** (TX) | **Bus de Datos:** Entrada de lecturas de presión |
| **GPIO 33** (TX) | **GPIO 18** (RX) | Canal de control (Reservado) |

### Especificaciones de Radio (SX1262 Interno)
* **Pines de Control:** NSS (8), IRQ (14), RST (12), BUSY (13).
* **Bus SPI:** SCK (9), MISO (11), MOSI (10).

## 🛠️ Configuración del Payload (ChirpStack)

Dado que los datos se envían como texto plano para facilitar el diagnóstico, utiliza este **Payload Codec** en JavaScript dentro de tu Device Profile para extraer el valor numérico:

```javascript
function decodeUplink(input) {
    // Convertir bytes a texto
    var texto = String.fromCharCode.apply(null, input.bytes);
    
    // Extraer el número (soporta decimales y signos)
    var match = texto.match(/[-+]?[0-9]*\.?[0-9]+/);
    
    if (match) {
        return {
            data: {
                presion_mb: parseFloat(match[0]),
                unidad: "mB",
                raw_text: texto.trim()
            }
        };
    }
    return { data: { error: "Formato de cadena inválido", raw: texto } };
}