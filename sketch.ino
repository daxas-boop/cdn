#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* wifiUser = "POCO F3";
const char* wifiPassword = "GrupoSinNombre";

int ledVerdeCalle = 4;
int ledAmarilloCalle = 5;
int ledRojoCalle = 16;
int ledVerdeAvenida = 15;
int ledAmarilloAvenida = 2;
int ledRojoAvenida = 0;
int pinBoton = 13;
int trigPin = 14;
int echoPin = 12;

bool enCiclo = false;
unsigned long inicioCiclo = 0;
unsigned long tiempoDeteccion = 0;
unsigned long tiempoSalidaCoche = 0;
bool cocheDetectado = false;
const int limiteSensor = 15;

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  setupPins();
  setupServer();
  ponerAvenidaVerde();
}

void loop() {
  server.handleClient();
  chequearBoton();
  chequearUltrasonico();
  actualizarSemaforo();
}

void setupPins() {
  pinMode(ledVerdeCalle, OUTPUT);
  pinMode(ledAmarilloCalle, OUTPUT);
  pinMode(ledRojoCalle, OUTPUT);
  pinMode(ledVerdeAvenida, OUTPUT);
  pinMode(ledAmarilloAvenida, OUTPUT);
  pinMode(ledRojoAvenida, OUTPUT);
  pinMode(pinBoton, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void setupServer() {
  WiFi.begin(wifiUser, wifiPassword);
  Serial.print("Conectando");

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Conectado IP:");
  Serial.println(WiFi.localIP());

  server.on("/", traerPaginaPrincipal);
  server.on("/estado", traerEstado);
  server.on("/activar", activarCiclo);
  server.begin();
}

// Cuando se apreta el boton inicia el ciclo del semaforo
void chequearBoton() {
  bool estadoBoton = digitalRead(pinBoton);

  if (estadoBoton == LOW && !enCiclo) {
    enCiclo = true;
  }
}

// Chequea si hay alguien esperando para cruzar
// Requiere detectar por 5 segundos para activar el ciclo
void chequearUltrasonico() {
  float distancia = medirDistancia();
  bool hayAlguien = distancia < limiteSensor && distancia != 0;
  cocheDetectado = hayAlguien;

  if (!hayAlguien) {
    tiempoDeteccion = 0;
    return;
  }

  if (hayAlguien && tiempoDeteccion == 0) {
    tiempoDeteccion = millis();
  }

  bool pasaron5Segundos = (tiempoDeteccion != 0) && (millis() - tiempoDeteccion >= 5000);

  if (hayAlguien && pasaron5Segundos && !enCiclo) {
    enCiclo = true;
    tiempoDeteccion = 0;
  }
}

// Maneja el estado de los semaforos
void actualizarSemaforo() {
  if (!enCiclo) return; // Si no hay ciclo activo, salir

  // Inicializar timestamp solo la primera vez que arranca el ciclo
  if (inicioCiclo == 0) {
    inicioCiclo = millis();
  }

  long t = millis() - inicioCiclo;

  if (t < 3000) ponerAvenidaAmarillo();
  else if (t < 5000) ponerCalleAmarillo();
  else if (t < 20000) {
    ponerCalleVerde();
    mantenerCalleVerdeSiCocheDetectado();
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

void mantenerCalleVerdeSiCocheDetectado() {
  if (tiempoSalidaCoche == 0) {
    if (cocheDetectado) {
      inicioCiclo = millis() - 5000;  // Mantener pausado mientras hay coche detectado
    } else {
      tiempoSalidaCoche = millis();  // Marcar cuando se fue
    }
  }
}

float medirDistancia() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duracion = pulseIn(echoPin, HIGH, 10000);
  float distancia = duracion * 0.034 / 2;

  return distancia;
}

void ponerCalleVerde() {
  digitalWrite(ledVerdeCalle, HIGH);
  digitalWrite(ledAmarilloCalle, LOW);
  digitalWrite(ledRojoCalle, LOW);
  digitalWrite(ledVerdeAvenida, LOW);
  digitalWrite(ledAmarilloAvenida, LOW);
  digitalWrite(ledRojoAvenida, HIGH);
}

void ponerAvenidaVerde() {
  digitalWrite(ledVerdeCalle, LOW);
  digitalWrite(ledAmarilloCalle, LOW);
  digitalWrite(ledRojoCalle, HIGH);
  digitalWrite(ledVerdeAvenida, HIGH);
  digitalWrite(ledAmarilloAvenida, LOW);
  digitalWrite(ledRojoAvenida, LOW);
}

void ponerAvenidaAmarillo() {
  digitalWrite(ledVerdeCalle, LOW);
  digitalWrite(ledAmarilloCalle, LOW);
  digitalWrite(ledRojoCalle, HIGH);
  digitalWrite(ledVerdeAvenida, LOW);
  digitalWrite(ledAmarilloAvenida, HIGH);
  digitalWrite(ledRojoAvenida, LOW);
}

void ponerCalleAmarillo() {
  digitalWrite(ledVerdeCalle, LOW);
  digitalWrite(ledAmarilloCalle, HIGH);
  digitalWrite(ledRojoCalle, LOW);
  digitalWrite(ledVerdeAvenida, LOW);
  digitalWrite(ledAmarilloAvenida, LOW);
  digitalWrite(ledRojoAvenida, HIGH);
}

void activarCiclo() {
  if (!enCiclo) enCiclo = true;
  server.send(200, "text/plain", "OK");
}

// Devuelve HTML con la pagina principal. Actualiza cada 500ms al estado del semaforo
void traerPaginaPrincipal() {
  String html = R"(
    <html>
      <head>
        <title>Semaforos</title>
        <link rel='stylesheet' href='https://cdn.jsdelivr.net/gh/daxas-boop/cdn/styles.css' >
        <script>
          function actualizarEstado() {
            fetch('/estado').then(r => r.text()).then(data => {
              document.getElementById('status').innerHTML = data;
            });
          }
          function activarCiclo() {
            fetch('/activar');
          }
          setInterval(actualizarEstado, 500);
        </script>
      </head>
      <body>
        <h1>Cruce Calle/Avenida</h1>
        <div id='intersection'>
          <div class='avenida'></div>
          <div class='calle'></div>
          <div id='status'></div>
        </div>
        <button onclick='activarCiclo()' style='margin: 20px; padding: 10px 20px; font-size: 16px; cursor: pointer;'>Activar semaforo de calle</button>
      </body>
    </html>
  )";

  server.send(200, "text/html", html);
}

// Devuelve HTML con el estado actual de cada LED
void traerEstado() {
  long transcurrido = enCiclo ? millis() - inicioCiclo : 0;
  bool calleEnVerde = enCiclo && transcurrido >= 5000 && transcurrido < 20000;
  bool avenidaEnRojo = enCiclo && transcurrido >= 3000 && transcurrido < 23000;

  String timerCalle = calleEnVerde ? String((20000 - transcurrido) / 1000) : "";
  String timerAvenida = avenidaEnRojo ? String((23000 - transcurrido) / 1000) : "";

  bool mostrarTimerCalle = calleEnVerde && tiempoSalidaCoche != 0;
  bool mostrarTimerAvenida = avenidaEnRojo && (!cocheDetectado || tiempoSalidaCoche != 0);

  String estado = "<div class='semaforo'>"
    "<h2>Calle</h2>"
    "<div class='luz rojo " + String(digitalRead(ledRojoCalle) ? "on" : "") + "'></div>"
    "<div class='luz amarillo " + String(digitalRead(ledAmarilloCalle) ? "on" : "") + "'></div>"
    "<div class='luz verde " + String(digitalRead(ledVerdeCalle) ? "on" : "") + "'></div>"
    "<div style='height:35px; line-height:35px; text-align:center; font-size:24px; color:#0f0; font-weight:bold; " + String(mostrarTimerCalle ? "" : "display:none;") + "'>" + timerCalle + "</div>"
    "</div>"
    "<div class='semaforo'>"
    "<h2>Avenida</h2>"
    "<div class='luz rojo " + String(digitalRead(ledRojoAvenida) ? "on" : "") + "'></div>"
    "<div class='luz amarillo " + String(digitalRead(ledAmarilloAvenida) ? "on" : "") + "'></div>"
    "<div class='luz verde " + String(digitalRead(ledVerdeAvenida) ? "on" : "") + "'></div>"
    "<div style='height:35px; line-height:35px; text-align:center; font-size:24px; color:#f00; font-weight:bold; " + String(mostrarTimerAvenida ? "" : "display:none;") + "'>" + timerAvenida + "</div>"
    "</div>" +
    (cocheDetectado ? "<div style='font-size:100px; position:absolute; bottom:20px; width:100%; text-align:center;'>🚘</div>" : "");

  server.send(200, "text/html", estado);
}
