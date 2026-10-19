#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <ThingSpeak.h>

// =====================================================
// CONFIGURAÇÕES
// =====================================================

// Wi-Fi
const char* WIFI_NOME = "SEU_SSID";
const char* WIFI_SENHA = "SUA_SENHA_AQUI";

// ThingSpeak
unsigned long CHANNEL_ID = 0;
const char* WRITE_API_KEY = "SUA_WRITE_API_KEY";

// Cofre
const String SENHA_COFRE = "SUA_SENHA_DO_COFRE";

const int PINO_SERVO = 18;

const int POSICAO_ABERTO = 0;
const int POSICAO_TRANCADO = 90;

// =====================================================
// OBJETOS
// =====================================================

WebServer server(80);
Servo servoCofre;
WiFiClient client;

// =====================================================
// ESTADO DO COFRE
// =====================================================

bool cofreAberto = false;

int tentativasErradas = 0;
int totalAberturas = 0;

// =====================================================
// THINGSPEAK
// =====================================================

unsigned long ultimoEnvio = 0;

const unsigned long INTERVALO_THINGSPEAK = 15000;

bool eventoPendente = false;

int eventoAtual = 0;

/*
EVENTOS:

0 = inicialização
1 = abriu
2 = trancou
3 = senha incorreta
*/

// =====================================================
// PÁGINA WEB
// =====================================================

const char PAGINA[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="pt-BR">

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width, initial-scale=1.0">

<title>Smart Safe ESP32</title>

<style>

* {
    box-sizing: border-box;
}

body {

    margin: 0;

    min-height: 100vh;

    display: flex;

    justify-content: center;

    align-items: center;

    font-family: Arial, sans-serif;

    background:
        linear-gradient(
            135deg,
            #020617,
            #111827,
            #1f2937
        );

    color: white;
}

.container {

    width: 90%;

    max-width: 420px;

    padding: 35px 25px;

    background: #111827;

    border: 1px solid #374151;

    border-radius: 22px;

    text-align: center;
}

h1 {
    margin-top: 0;
}

.status {

    margin: 25px 0;

    font-size: 24px;

    font-weight: bold;

    color: #ef4444;
}

input {

    width: 100%;

    padding: 16px;

    margin-bottom: 12px;

    border: 2px solid #374151;

    border-radius: 10px;

    background: #1f2937;

    color: white;

    text-align: center;

    font-size: 22px;
}

button {

    width: 100%;

    padding: 16px;

    margin-top: 10px;

    border: none;

    border-radius: 10px;

    color: white;

    font-size: 18px;

    font-weight: bold;

    cursor: pointer;
}

.abrir {
    background: #22c55e;
}

.trancar {
    background: #ef4444;
}

#mensagem {

    margin-top: 20px;

    min-height: 25px;

    font-weight: bold;
}

</style>

</head>

<body>

<div class="container">

    <h1>🔐 SMART SAFE</h1>

    <div
        id="status"
        class="status"
    >
        Verificando...
    </div>

    <input
        id="senha"
        type="password"
        inputmode="numeric"
        placeholder="Digite a senha"
    >

    <button
        class="abrir"
        onclick="abrirCofre()"
    >
        ABRIR
    </button>

    <button
        class="trancar"
        onclick="trancarCofre()"
    >
        TRANCAR
    </button>

    <div id="mensagem"></div>

</div>

<script>

function verificarStatus() {

    fetch("/status")

    .then(response => response.text())

    .then(data => {

        const status =
            document.getElementById("status");

        if (data === "ABERTO") {

            status.innerText =
                "🔓 COFRE ABERTO";

            status.style.color =
                "#22c55e";

        } else {

            status.innerText =
                "🔒 COFRE TRANCADO";

            status.style.color =
                "#ef4444";
        }

    });

}

function abrirCofre() {

    const senha =
        document.getElementById("senha").value;

    if (senha.length === 0) {

        document.getElementById("mensagem")
            .innerText =
            "Digite a senha.";

        return;
    }

    fetch(
        "/abrir?senha=" +
        encodeURIComponent(senha)
    )

    .then(response => response.text())

    .then(data => {

        const mensagem =
            document.getElementById("mensagem");

        if (data === "OK") {

            mensagem.innerText =
                "✅ Cofre aberto.";

        }

        else if (data === "ERRADO") {

            mensagem.innerText =
                "❌ Senha incorreta.";

        }

        else {

            mensagem.innerText =
                "Erro na requisição.";
        }

        document.getElementById("senha")
            .value = "";

        verificarStatus();

    });

}

function trancarCofre() {

    fetch("/trancar")

    .then(response => response.text())

    .then(data => {

        document.getElementById("mensagem")
            .innerText =
            "🔒 Cofre trancado.";

        verificarStatus();

    });

}

document.getElementById("senha")
.addEventListener(
    "keydown",
    function(event) {

        if (event.key === "Enter") {
            abrirCofre();
        }

    }
);

verificarStatus();

</script>

</body>

</html>

)rawliteral";

// =====================================================
// REGISTRAR EVENTO
// =====================================================

void registrarEvento(int evento) {

    eventoAtual = evento;

    eventoPendente = true;
}

// =====================================================
// ENVIAR PARA THINGSPEAK
// =====================================================

void enviarThingSpeak() {

    if (!eventoPendente) {
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (
        millis() - ultimoEnvio
        < INTERVALO_THINGSPEAK
    ) {
        return;
    }

    ThingSpeak.setField(
        1,
        cofreAberto ? 1 : 0
    );

    ThingSpeak.setField(
        2,
        tentativasErradas
    );

    ThingSpeak.setField(
        3,
        totalAberturas
    );

    ThingSpeak.setField(
        4,
        eventoAtual
    );

    int resposta =
        ThingSpeak.writeFields(
            CHANNEL_ID,
            WRITE_API_KEY
        );

    Serial.print(
        "ThingSpeak: "
    );

    Serial.println(
        resposta
    );

    if (resposta == 200) {

        eventoPendente = false;

        ultimoEnvio = millis();

        Serial.println(
            "Dados enviados com sucesso."
        );

    } else {

        Serial.println(
            "Falha no envio. Tentará novamente."
        );

    }

}

// =====================================================
// PÁGINA PRINCIPAL
// =====================================================

void paginaInicial() {

    server.send(
        200,
        "text/html",
        PAGINA
    );

}

// =====================================================
// ABRIR COFRE
// =====================================================

void abrirCofre() {

    if (!server.hasArg("senha")) {

        server.send(
            400,
            "text/plain",
            "ERRO"
        );

        return;
    }

    String senhaRecebida =
        server.arg("senha");

    if (
        senhaRecebida
        == SENHA_COFRE
    ) {

        servoCofre.write(
            POSICAO_ABERTO
        );

        cofreAberto = true;

        totalAberturas++;

        registrarEvento(1);

        Serial.println(
            "Cofre aberto."
        );

        server.send(
            200,
            "text/plain",
            "OK"
        );

    }

    else {

        tentativasErradas++;

        registrarEvento(3);

        Serial.println(
            "Senha incorreta."
        );

        server.send(
            200,
            "text/plain",
            "ERRADO"
        );

    }

}

// =====================================================
// TRANCAR COFRE
// =====================================================

void trancarCofre() {

    servoCofre.write(
        POSICAO_TRANCADO
    );

    cofreAberto = false;

    registrarEvento(2);

    Serial.println(
        "Cofre trancado."
    );

    server.send(
        200,
        "text/plain",
        "OK"
    );

}

// =====================================================
// STATUS
// =====================================================

void statusCofre() {

    if (cofreAberto) {

        server.send(
            200,
            "text/plain",
            "ABERTO"
        );

    } else {

        server.send(
            200,
            "text/plain",
            "TRANCADO"
        );

    }

}

// =====================================================
// SETUP
// =====================================================

void setup() {

    Serial.begin(115200);

    delay(1000);

    // Servo
    servoCofre.setPeriodHertz(50);

    servoCofre.attach(
        PINO_SERVO,
        500,
        2400
    );

    // Sempre inicia trancado
    servoCofre.write(
        POSICAO_TRANCADO
    );

    cofreAberto = false;

    // =================================================
    // WI-FI
    // =================================================

    WiFi.mode(WIFI_STA);

    WiFi.begin(
        WIFI_NOME,
        WIFI_SENHA
    );

    Serial.println();
    Serial.println(
        "Conectando ao Wi-Fi..."
    );

    unsigned long inicio =
        millis();

    while (
        WiFi.status()
        != WL_CONNECTED
    ) {

        delay(500);

        Serial.print(".");

        if (
            millis() - inicio
            > 20000
        ) {

            Serial.println();
            Serial.println(
                "Falha ao conectar ao Wi-Fi."
            );

            return;
        }

    }

    Serial.println();
    Serial.println(
        "Wi-Fi conectado!"
    );

    Serial.print(
        "IP DO COFRE: http://"
    );

    Serial.println(
        WiFi.localIP()
    );

    Serial.print(
        "RSSI: "
    );

    Serial.print(
        WiFi.RSSI()
    );

    Serial.println(
        " dBm"
    );

    // =================================================
    // THINGSPEAK
    // =================================================

    ThingSpeak.begin(client);

    // =================================================
    // ROTAS
    // =================================================

    server.on(
        "/",
        paginaInicial
    );

    server.on(
        "/abrir",
        abrirCofre
    );

    server.on(
        "/trancar",
        trancarCofre
    );

    server.on(
        "/status",
        statusCofre
    );

    // =================================================
    // SERVIDOR
    // =================================================

    server.begin();

    Serial.println(
        "Servidor iniciado."
    );

    // Evento inicial
    registrarEvento(0);
}

// =====================================================
// LOOP
// =====================================================

void loop() {

    server.handleClient();

    enviarThingSpeak();
}
