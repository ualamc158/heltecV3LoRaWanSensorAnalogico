# Nodo LoRaWAN Maestro (Heltec V3) - Transmisor de Presión

Este repositorio contiene la aplicación de un **Heltec WiFi LoRa 32 (V3)** que actúa como el **"Maestro"** en un sistema de monitorización de presión. Su función principal es coordinarse con un sensor esclavo a través de UART, solicitar los datos de forma sincronizada y subirlos a la red LoRaWAN.

> 🔗 **Dependencia del Sistema:** Este nodo requiere estar conectado físicamente a un nodo esclavo para obtener lecturas reales.
> 👉 **[Repositorio del Nodo Esclavo (ESP32-S3)](https://github.com/ualamc158/esp32LectorSensorAnalogicoPresion)**

---

## 🚀 1. Funcionalidades Principales

* **Sincronización Maestro-Esclavo:** Solicita datos cada 30 segundos enviando el comando `'G'` por UART.
* **Conexión LoRaWAN Robusta:** Implementa activación OTAA en la banda EU868. Si la conexión falla, el sistema se reinicia automáticamente tras 5 segundos para intentar un "Join" limpio.
* **Gestión de Tiempo:** Sincroniza el reloj interno con la hora de compilación para generar timestamps en los registros del monitor serie.
* **Optimización de Radio:** Configurado específicamente para el chip SX1262 del Heltec V3, incluyendo el uso de TCXO y control de antena.

---

## 📡 2. Lógica del Protocolo

Para asegurar la integridad de los datos y evitar colisiones, el sistema sigue este flujo:

1.  **Handshake UART:** El Maestro envía el carácter `'G'` (Go).
2.  **Espera Activa:** El Maestro espera hasta 1500ms la respuesta del Esclavo (ej. `P:10.07`).
3.  **Transmisión:** Al recibir el dato, se genera un log con timestamp y se envía inmediatamente al Gateway LoRaWAN.

---

## 🔌 3. Conexiones Físicas (Esquema de Cables)

Utiliza cables puente para conectar ambas placas compartiendo masa común:

| Pin en Heltec V3 (Maestro) | Pin en ESP32-S3 (Esclavo) | Función |
| :--- | :--- | :--- |
| **GND** | **GND** | **Masa Común** (Obligatorio) |
| **33** (TX) | **18** (RX) | **Petición:** Envío de orden `'G'` |
| **35** (RX) | **17** (TX) | **Datos:** Recepción de presión `P:XX.XX` |

---

## 🛠️ 4. Configuración en ChirpStack (Codec)

Utiliza el siguiente decodificador JavaScript en el **Device Profile** para procesar los datos recibidos:

```javascript
function decodeUplink(input) {
    var texto = String.fromCharCode.apply(null, input.bytes);
    var limpio = texto.replace(/[^\x20-\x7E]/g, ''); // Elimina caracteres no deseados
    var match = limpio.match(/[-+]?[0-9]*\.?[0-9]+/);
    
    if (match) {
        return { data: { presion_mb: parseFloat(match[0]), texto_recibido: limpio } };
    }
    return { data: { error: "Dato corrupto", raw: texto } };
}