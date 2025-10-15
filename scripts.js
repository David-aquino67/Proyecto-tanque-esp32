var clientID = 'client_id_' + Math.floor((Math.random() * 1000000) + 1);
var client = new Paho.MQTT.Client("broker.emqx.io", 8084, clientID);

var accelerationInterval;
var currentSpeed = 30;
var maxSpeed = 255;
var accelerationStep = 5;
let commandCooldown = false; // Variable para controlar el debounce


client.onConnectionLost = onConnectionLost;
client.onMessageArrived = onMessageArrived;

var options = {
    useSSL: true,
    userName: "",
    password: "",
    onSuccess: onConnect,
    onFailure: doFail
};

client.connect(options);

function onConnect() {
    console.log("Conexión exitosa");
    client.subscribe("david");
    client.subscribe("servo/david");
}

function onMessageArrived(message) {
    console.log("Mensaje recibido:", message.payloadString);
    var displayDiv = document.getElementById('slider');
     // Mostrar el mensaje en el HTML
     var alertaDiv = document.getElementById('alerta');
     alertaDiv.textContent = message.payloadString; // Actualizar el contenido del div con el mensaje recibido
}

function doFail(e) {
    console.log("Error de conexión:", e);
}

function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        console.log("Conexión perdida:", responseObject.errorMessage);
    }
}
function command(value) {
    console.log("Enviando comando para el servo:", value);
    let messageContent = "servo_" + value; // Agregar el prefijo esperado por el Arduino
    let message = new Paho.MQTT.Message(messageContent);
    message.destinationName = "david"; // Usar el mismo tópico
    client.send(message);
    console.log("Mensaje enviado:", messageContent);
}

function changeDirection(direction) {
    var messageContent = "motor_" + direction; // Agrega el prefijo motor_
    var message = new Paho.MQTT.Message(messageContent);
    message.destinationName = "david"; // Usa el tópico único
    client.send(message);
    console.log("Dirección enviada:", messageContent);
}


function changeSpeed(direction) {
    if (commandCooldown) {
        console.log("Comando bloqueado por debounce:", direction);
        return; // Ignorar si está en cooldown
    }

    commandCooldown = true; // Activar cooldown
    sendDirection(direction); // Enviar la dirección inmediatamente

    if (accelerationInterval) {
        clearInterval(accelerationInterval);
    }

    accelerationInterval = setInterval(() => {
        if (currentSpeed < maxSpeed) {
            currentSpeed += accelerationStep;
            if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;
            sendSpeed(currentSpeed); // Enviar velocidad actualizada
        }
    }, 100);

    // Desactivar cooldown después de 300ms
    setTimeout(() => {
        commandCooldown = false;
    }, 300);
}


function sendSpeed(speed) {
    if (client.isConnected()) {
        var messageContent = "motor_" + speed; // Agregar prefijo motor_
        var message = new Paho.MQTT.Message(messageContent);
        message.destinationName = "david";
        client.send(message);
        console.log("Velocidad enviada:", messageContent);
    } else {
        console.log("Cliente MQTT desconectado. No se pudo enviar la velocidad.");
    }
}


function sendDirection(direction) {
    if (client.isConnected()) {
        var messageContent = "motor_" + direction; // Agregar prefijo motor_
        var message = new Paho.MQTT.Message(messageContent);
        message.destinationName = "david";
        client.send(message);
        console.log("Dirección enviada:", messageContent);
    } else {
        console.log("Cliente MQTT desconectado. No se pudo enviar la dirección.");
    }
}



function stopGradually() {
    if (accelerationInterval) {
        clearInterval(accelerationInterval); // Detener cualquier aceleración previa
        accelerationInterval = null;
    }

    accelerationInterval = setInterval(() => {
        if (currentSpeed > 30) { // Reducir la velocidad hasta 30
            currentSpeed -= accelerationStep;
            if (currentSpeed < 30) currentSpeed = 30; // Asegurar que no baje de 30
            sendSpeed(currentSpeed); // Enviar la velocidad actualizada
        } else {
            clearInterval(accelerationInterval); // Detener el intervalo
            accelerationInterval = null; // Limpiar la referencia
            console.log("Velocidad mínima alcanzada: 30");
        }
    }, 100); // Reducir la velocidad cada 100 ms
}


$(document).ready(function () {
    // Manejo de "Avanzar"
    $("#btn-adelante")
        .on("mousedown", function () {
            console.log("Botón adelante presionado (mousedown)");
            changeSpeed("adelante");
        })
        .on("mouseup", function () {
            console.log("Botón adelante liberado (mouseup)");
            stopGradually();
        });

    // Manejo de "Retroceder"
    $("#btn-atras")
        .on("mousedown", function () {
            console.log("Botón atrás presionado");
            changeSpeed("atras");
        })
        .on("mouseup", function () {
            console.log("Botón atrás liberado");
            stopGradually();
        });

         // Manejo de "Izquierda"
    // Manejo de "Derecha"
    $("#btn-derecha")
        .on("mousedown", function () {
            console.log("Botón derecho presionado");
            changeSpeed("derecha"); // Cambiar la dirección a izquierda con velocidad inicial
        })
        .on("mouseup", function () {
            console.log("Botón derecho liberado");
            stopGradually(); // Reducir la velocidad gradualmente
        });

          // Manejo de "Izquierda"
    $("#btn-izquierda")
    .on("mousedown", function () {
        console.log("Botón izquierdo presionado");
        changeSpeed("izquierda"); // Cambiar la dirección a izquierda con velocidad inicial
    })
    .on("mouseup", function () {
        console.log("Botón izquierdo liberado");
        stopGradually(); // Reducir la velocidad gradualmente
    });
    $("#btn-stop")
    .on("mousedown", function () {
        console.log("Botón detener presionado");
        sendDirection("stop"); // Cambiar la dirección a izquierda con velocidad inicial
    })
    .on("mouseup", function () {
        console.log("Botón stop liberado");
        stopGradually(); // Reducir la velocidad gradualmente
    });
});


