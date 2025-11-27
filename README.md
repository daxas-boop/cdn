# Sistema de Semáforos para Cruce Rural

## Descripción del Proyecto

Sistema de control de semáforos para un cruce entre una **calle pequeña en zona rural** y una **avenida/ruta muy transitada**.

### Contexto

Los coches de la calle rural necesitan cruzar la ruta/avenida de manera segura. El semáforo se activa de dos formas:

1. **Botón pulsador**: Activación manual inmediata
2. **Sensor ultrasónico**: Detección automática de coches esperando por 5 segundos continuos

## Funcionamiento

### Estado Normal (por defecto)

- 🟢 **Avenida**: VERDE (tránsito libre)
- 🔴 **Calle**: ROJO (espera)
- Permanece así indefinidamente hasta activación

### Activación del Ciclo

**Opción 1: Botón**

- Presionar el botón → inicia ciclo inmediatamente

**Opción 2: Sensor Ultrasónico**

- Coche se detiene a menos de 15cm del sensor
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

3. **Calle Verde** (5-20s) - Paso de coches

   - 🔴 Avenida: ROJO
   - 🟢 Calle: VERDE
   - ⏱️ Contador regresivo visible
   - ⚡ **Extensión automática**: Si hay un coche detectado, el verde se mantiene hasta que el coche cruce

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

- 1x ESP8266
- 6x LEDs (2 verdes, 2 amarillos, 2 rojos)
- 6x Resistencias 220-330Ω
- 1x Pulsador (botón)
- 1x Sensor HC-SR04 (ultrasónico)
- WiFi

### Pines

| Componente       | GPIO                      |
| ---------------- | ------------------------- |
| Calle Verde      | D2 (GPIO 4)               |
| Calle Amarillo   | D1 (GPIO 5)               |
| Calle Rojo       | D0 (GPIO 16)              |
| Avenida Verde    | D8 (GPIO 15)              |
| Avenida Amarillo | D4 (GPIO 2)               |
| Avenida Rojo     | D3 (GPIO 0)               |
| Botón            | D7 (GPIO 13) INPUT_PULLUP |
| HC-SR04 Trigger  | D5 (GPIO 14)              |
| HC-SR04 Echo     | D6 (GPIO 12)              |

### Conexión

```
ESP8266        LEDs/Sensores
GPIO 4  --[R]-- LED Verde Calle
GPIO 5  --[R]-- LED Amarillo Calle
GPIO 16 --[R]-- LED Rojo Calle
GPIO 15 --[R]-- LED Verde Avenida
GPIO 2  --[R]-- LED Amarillo Avenida
GPIO 0  --[R]-- LED Rojo Avenida
GPIO 13 --[Botón]-- GND
GPIO 14 -- HC-SR04 Trigger
GPIO 12 -- HC-SR04 Echo
```

## Interfaz Web

### Características

- **Visualización gráfica**: Intersección de calles con semáforos posicionados
- **Actualización en tiempo real**: request cada 500ms
- **Contadores regresivos**:
  - Calle: muestra tiempo restante durante fase verde en color verde (visible solo cuando el coche ya salió)
  - Avenida: muestra tiempo de espera en rojo en color rojo (pausado mientras hay coche detectado)
- **Indicador de detección**: Emoji 🚘 que aparece cuando el sensor ultrasónico detecta un coche (< 15cm)
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
chequearBoton()                        // Detecta botón e inicia ciclo
chequearUltrasonico()                  // Detecta presencia 5s e inicia ciclo
medirDistancia()                       // Lee sensor HC-SR04
actualizarSemaforo()                   // Maneja transiciones con millis()
mantenerCalleVerdeSiCocheDetectado()   // Pausa el ciclo si hay coche detectado
traerPaginaPrincipal()                 // Sirve HTML principal
traerEstado()                          // Endpoint con estado actual
activarCiclo()                         // Endpoint para activar ciclo desde web
```

### Sistema de Timing

Usa `millis()` para control no bloqueante:

**Variables principales:**

- `enCiclo` - Ciclo activo o normal
- `inicioCiclo` - Timestamp del inicio del ciclo
- `tiempoDeteccion` - Timestamp de detección (requiere 5s continuos)
- `cocheDetectado` - Hay coche a menos de 15cm
- `tiempoSalidaCoche` - Cuando el coche salió (continua el timer)

**Extensión automática:** Mientras la calle este verde (5-20s), `mantenerCalleVerdeSiCocheDetectado()` pausa el timer mientras hay un coche detectado.

### Lógica Simple

```cpp
void actualizarSemaforo() {
  if (!enCiclo) return;

  if (inicioCiclo == 0) {
    inicioCiclo = millis();
  }

  long t = millis() - inicioCiclo;

  if (t < 3000) ponerAvenidaAmarillo();
  else if (t < 5000) ponerCalleAmarillo();
  else if (t < 20000) {
    ponerCalleVerde();
    mantenerCalleVerdeSiCocheDetectado();  // Pausa si hay coche
  }
  else if (t < 23000) ponerCalleAmarillo();
  else if (t < 25000) ponerAvenidaAmarillo();
  else {
    ponerAvenidaVerde();
    enCiclo = false;
    inicioCiclo = 0;
    tiempoSalidaCoche = 0;
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

- **Plataforma**: ESP8266 (Arduino Framework)
- **Lenguaje**: C++
- **Librerías**: ESP8266WiFi.h, ESP8266WebServer.h
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
