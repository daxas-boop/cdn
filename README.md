# Sistema de Semáforos para Cruce Rural

## Descripción del Proyecto

Sistema de control de semáforos para un cruce entre una **calle pequeña en zona rural** y una **avenida/ruta muy transitada**.

### Contexto

Los habitantes de la calle rural necesitan cruzar la ruta/avenida de manera segura. El semáforo se activa de dos formas:
1. **Botón pulsador**: Activación manual inmediata
2. **Sensor ultrasónico**: Detección automática de personas esperando por 5 segundos continuos

## Funcionamiento

### Estado Normal (por defecto)
- 🟢 **Avenida**: VERDE (tránsito libre)
- 🔴 **Calle**: ROJO (espera)
- Permanece así indefinidamente hasta activación

### Activación del Ciclo

**Opción 1: Botón**
- Presionar el botón → inicia ciclo inmediatamente

**Opción 2: Sensor Ultrasónico**
- Persona se para a menos de 15cm del sensor
- Debe permanecer 5 segundos continuos
- Después de 5 segundos → inicia ciclo
- Si se aleja antes de los 5 segundos → se cancela

### Secuencia del Ciclo

**Ciclo completo: 25 segundos**

1. **Avenida Amarillo** (0-3s)
   - 🟡 Avenida: AMARILLO
   - 🔴 Calle: ROJO

2. **Calle Amarillo** (3-5s) - Transición estilo Argentina
   - 🔴 Avenida: ROJO
   - 🟡 Calle: AMARILLO

3. **Calle Verde** (5-20s) - Paso peatonal
   - 🔴 Avenida: ROJO
   - 🟢 Calle: VERDE
   - ⏱️ Contador regresivo visible

4. **Calle Amarillo** (20-23s)
   - 🔴 Avenida: ROJO
   - 🟡 Calle: AMARILLO

5. **Avenida Amarillo** (23-25s) - Transición estilo Argentina
   - 🟡 Avenida: AMARILLO
   - 🔴 Calle: ROJO

6. **Vuelta a Normal** (25s+)
   - 🟢 Avenida: VERDE
   - 🔴 Calle: ROJO

## Hardware

### Componentes
- 1x ESP32
- 6x LEDs (2 verdes, 2 amarillos, 2 rojos)
- 6x Resistencias 220-330Ω
- 1x Pulsador (botón)
- 1x Sensor HC-SR04 (ultrasónico)
- WiFi

### Pines

| Componente | GPIO |
|------------|------|
| Calle Verde | 16 |
| Calle Amarillo | 17 |
| Calle Rojo | 18 |
| Avenida Verde | 33 |
| Avenida Amarillo | 25 |
| Avenida Rojo | 26 |
| Botón | 15 (INPUT_PULLUP) |
| HC-SR04 Trigger | 22 |
| HC-SR04 Echo | 21 |

### Conexión

```
ESP32          LEDs/Sensores
GPIO 16 --[R]-- LED Verde Calle
GPIO 17 --[R]-- LED Amarillo Calle
GPIO 18 --[R]-- LED Rojo Calle
GPIO 33 --[R]-- LED Verde Avenida
GPIO 25 --[R]-- LED Amarillo Avenida
GPIO 26 --[R]-- LED Rojo Avenida
GPIO 15 --[Botón]-- GND
GPIO 22 -- HC-SR04 Trigger
GPIO 21 -- HC-SR04 Echo
```

## Interfaz Web

### Características
- **Visualización gráfica**: Intersección de calles con semáforos posicionados
- **Actualización en tiempo real**: request cada 500ms
- **Contadores regresivos**:
  - Calle: muestra tiempo restante durante fase verde en color verde (5-20s)
  - Avenida: muestra tiempo de espera en rojo en color rojo (3-23s)
- **Indicador de detección**: Emoji 🚶 que aparece en la parte inferior cuando el sensor ultrasónico detecta una persona (< 15cm)
- **Botón web**: Permite activar el ciclo remotamente desde la página
- **Diseño visual**:
  - Caminos grises con líneas amarillas
  - Fondo verde simulando zona rural
  - Semáforos con luces circulares
  - Efecto brillante en luces encendidas
  - CSS externo desde CDN

### Acceso
1. Conectar el ESP32 (ver IP en monitor serial)
2. Abrir navegador: `http://[IP_DEL_ESP32]`
3. Usar botón "Activar semáforo de calle" o esperar detección física

## Código

### Funciones Principales

```cpp
ponerAvenidaVerde()      // Avenida verde, calle rojo
ponerAvenidaAmarillo()   // Avenida amarillo, calle rojo
ponerCalleVerde()        // Calle verde, avenida rojo
ponerCalleAmarillo()     // Calle amarillo, avenida rojo
chequearBoton()          // Detecta botón e inicia ciclo
chequearUltrasonico()    // Detecta presencia 5s e inicia ciclo
medirDistancia()         // Lee sensor HC-SR04
actualizarSemaforo()     // Maneja transiciones con millis()
traerPaginaPrincipal()   // Sirve HTML principal
traerEstado()            // Endpoint con estado actual
activarCiclo()           // Endpoint para activar ciclo desde web
```

### Sistema de Timing

El código usa timestamps (millis()) para manejar tiempos sin bloquear:

**Control del Ciclo de Semáforo:**
- `enCiclo` - Bandera que indica si el ciclo está activo o en estado normal
- `inicioCiclo` - Momento en que se activó el ciclo (timestamp)
- El ciclo usa el tiempo transcurrido (`millis() - inicioCiclo`) para saber en qué fase está

**Control de Detección Ultrasónica:**
- `tiempoDeteccion` - Timestamp de cuando empezó la detección (0 = no detectando)
- `personaDetectada` - Bandera que indica si hay alguien a menos de 15cm
- Requiere 5 segundos continuos (`millis() - tiempoDeteccion >= 5000`) para activar
- Se resetea a 0 si la persona se aleja antes de completar los 5 segundos

Este sistema permite que el ESP32 atienda el servidor web, lea sensores y controle LEDs simultáneamente sin trabarse.

### Lógica Simple

```cpp
void actualizarSemaforo() {
  if (!enCiclo) return;

  if (inicioCiclo == 0) {
    inicioCiclo = millis();
  }

  unsigned long transcurrido = millis() - inicioCiclo;

  if (transcurrido < 3000) ponerAvenidaAmarillo();
  else if (transcurrido < 5000) ponerCalleAmarillo();
  else if (transcurrido < 20000) ponerCalleVerde();
  else if (transcurrido < 23000) ponerCalleAmarillo();
  else if (transcurrido < 25000) ponerAvenidaAmarillo();
  else {
    ponerAvenidaVerde();
    enCiclo = false;
    inicioCiclo = 0;
  }
}
```

## Configuración WiFi

En el código:

```cpp
char* wifiUser = "TU_SSID";
char* wifiPassword = "TU_PASSWORD";
```

## Tecnología

- **Plataforma**: ESP32 (Arduino Framework)
- **Lenguaje**: C++
- **Librerías**: WiFi.h, WebServer.h
- **Frontend**: HTML5, CSS3, JavaScript (AJAX)
- **Sin dependencias externas**

## Posibles mejoras

### 1. Display 7 Segmentos (I2C o GPIO)
**Objetivo:** Mostrar timer físico para peatones

**Implementación:**
- Módulo TM1637 o display directo por GPIO
- Protocolo I2C para comunicación
- Muestra cuenta regresiva durante fase verde de calle

**Beneficios para Sistemas Embebidos:**
- Demuestra interfaz con hardware externo
- Implementación de protocolo I2C
- Manejo de dispositivos de salida

### 2. Interrupciones en Botón
**Objetivo:** Reemplazar polling por interrupciones

**Implementación:**
```cpp
volatile bool botonPresionado = false;

void IRAM_ATTR ISR_boton() {
  unsigned long ahora = millis();
  if (ahora - ultimaInterrupcion > 200) {  // Debounce
    botonPresionado = true;
    ultimaInterrupcion = ahora;
  }
}

void setup() {
  attachInterrupt(digitalPinToInterrupt(pinBoton), ISR_boton, FALLING);
}
```

**Beneficios para Sistemas Embebidos:**
- Mejor práctica para entrada de usuario
- Respuesta instantánea
- Mayor eficiencia de CPU
- Demuestra manejo correcto de interrupciones

### 3. Watchdog Timer
**Objetivo:** Aumentar robustez del sistema

**Implementación:**
```cpp
#include "esp_task_wdt.h"

void setup() {
  esp_task_wdt_init(10, true);  // 10 segundos timeout
  esp_task_wdt_add(NULL);
}

void loop() {
  esp_task_wdt_reset();  // Feed the watchdog
  // resto del código...
}
```

**Beneficios para Sistemas Embebidos:**
- Protección contra bloqueos del sistema
- Reinicio automático en caso de fallo
- Demuestra consideraciones de confiabilidad
- Esencial en sistemas críticos

