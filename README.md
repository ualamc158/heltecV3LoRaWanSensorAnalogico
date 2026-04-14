# Nodo LoRaWAN Maestro (Heltec V3) - Transmisor de Presión

Este repositorio contiene la aplicación de un **Heltec WiFi LoRa 32 (V3)** que actúa como el **"Maestro"** en un sistema de monitorización de presión. Su función principal es coordinarse con un sensor esclavo, solicitar los datos de forma sincronizada y subirlos a la red LoRaWAN.

> 🔗 **Dependencia del Sistema:** Este nodo requiere estar conectado físicamente a un nodo esclavo para obtener lecturas reales.
> 👉 **[Repositorio del Nodo Esclavo (ESP32-S3)](https://github.com/ualamc158/esp32LectorSensorAnalogicoPresion)**

*(💡 **Nota:** Todo este sistema está construido sobre un port de la librería RadioLib para ESP-IDF. Si buscas la documentación original de la librería base, cambia a la etiqueta **`v0.0`**).*

---

## 📡 1. Lógica del Sistema Maestro-Esclavo

Para evitar colisiones en la radio y asegurar la integridad de los datos, el sistema sigue este protocolo de sincronización:

1. **Conexión LoRaWAN:** El Heltec se conecta a la red (EU868) mediante OTAA.
2. **Handshake (Apretón de Manos):** Cada 30 segundos, el Maestro envía el carácter `'G'` (Go) al Esclavo por UART.
3. **Recepción y Transmisión:** Al recibir la respuesta (ej. `P:10.07`), el Maestro limpia el buffer, muestra el dato en el OLED local y lo transmite inmediatamente al Gateway LoRaWAN.

---

## 🔌 2. Conexiones Físicas (Esquema de Cables)

Utiliza cables puente (jumpers) para conectar ambas placas compartiendo masa común:

| Pin en Heltec V3 (Maestro) | Pin en ESP32-S3 (Esclavo) | Función |
| :--- | :--- | :--- |
| **GND** | **GND** | **Masa Común** (Imprescindible) |
| **33** (TX) | **18** (RX) | **Petición:** Envío de orden `'G'` |
| **35** (RX) | **17** (TX) | **Datos:** Recepción de presión `P:XX.XX` |

---

## 🛠️ 3. Configuración en ChirpStack (Codec)

Para procesar el payload en el servidor, utiliza el siguiente decodificador JavaScript en el **Device Profile**:

```javascript
function decodeUplink(input) {
    var texto = String.fromCharCode.apply(null, input.bytes);
    var limpio = texto.replace(/[^\x20-\x7E]/g, '');
    var match = limpio.match(/[-+]?[0-9]*\.?[0-9]+/);
    
    if (match) {
        return { data: { presion_mb: parseFloat(match[0]), texto_recibido: limpio } };
    }
    return { data: { error: "Dato corrupto", raw: texto } };
}