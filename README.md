# Nodo LoRaWAN Maestro (Heltec V3) - Transmisor de Presión

Este repositorio contiene la aplicación de un **Heltec WiFi LoRa 32 (V3)** que actúa como el **"Maestro"** en un sistema de monitorización de presión. Su función principal es coordinarse con un sensor esclavo (ESP32-S3), pedirle los datos mediante comunicación serie (UART) de forma sincronizada y subir esos datos a un servidor LoRaWAN (ChirpStack).

*(💡 **Nota:** Todo este sistema está construido sobre un port de la librería RadioLib para ESP-IDF. Si buscas la documentación original de la librería base y su configuración de pines SPI/OLED, cambia a la etiqueta **`v0.0`** en el historial de este repositorio).*

---

## 📡 1. Lógica del Sistema Maestro-Esclavo

Para evitar que el módulo de radio LoRa colapse al recibir datos mientras está transmitiendo, hemos implementado una arquitectura sincronizada:

1. **Conexión LoRaWAN:** El Heltec arranca y se conecta a la red LoRaWAN (Banda EU868) mediante OTAA.
2. **Handshake (Apretón de Manos):** El Heltec toma el control de los tiempos. Cada 30 segundos, envía un comando (el carácter `'G'`) al microcontrolador esclavo por el puerto UART para "despertarlo" o darle permiso para hablar.
3. **Recepción y Transmisión:** Al recibir el comando, el esclavo le devuelve el dato de presión inmediatamente (ej. `P:10.07`). El Heltec lo lee, limpia la consola y lo transmite al instante al Gateway LoRaWAN.

---

## 🔌 2. Conexiones Físicas (Protoboard)

Para que el sistema se sincronice correctamente, debes conectar este Heltec V3 al ESP32-S3 (Esclavo). Utiliza los **números impresos en color blanco** en los bordes de ambas placas:

| Pin en Heltec V3 (Maestro) | Pin en ESP32-S3 (Esclavo) | Función del Cable |
| :--- | :--- | :--- |
| **GND** | **GND** | **Masa Común:** ¡Vital! Cierra el circuito para evitar símbolos raros y ruido. |
| **33** (TX) | **18** (RX) | **Comando de Petición:** El Heltec envía la orden `'G'` por aquí. |
| **35** (RX) | **17** (TX) | **Recepción de Datos:** El Heltec recibe el valor de presión (`P:XX.XX\n`) por aquí. |

---

## 🛠️ 3. Configuración en ChirpStack (Codec)

Los datos llegan a ChirpStack en texto plano codificado. Para extraer el número exacto y poder graficarlo o enviarlo a otras plataformas, debes configurar un **Codec en JavaScript**.

Ve a **Device Profiles** > Tu Perfil > **Codec** y pega este script:

```javascript
function decodeUplink(input) {
    // Convertir los bytes a texto
    var texto = String.fromCharCode.apply(null, input.bytes);
    
    // Filtrar caracteres extraños por si hay ruido eléctrico en el cable
    var limpio = texto.replace(/[^\x20-\x7E]/g, '');
    
    // Extraer solo los números (incluyendo negativos y decimales)
    var match = limpio.match(/[-+]?[0-9]*\.?[0-9]+/);
    
    if (match) {
        return { 
            data: { 
                presion_mb: parseFloat(match[0]), 
                texto_recibido: limpio 
            } 
        };
    }
    return { data: { error: "Dato corrupto", raw: texto } };
}