// Create WebSocket connection.
var socket = new WebSocket('ws://192.168.0.31:9876');
var textarea;
var btn;

function init() {
	textarea = document.getElementById("textarea");
	btn = document.getElementById("btn");
	btn.addEventListener("click", function(event) {
		socket.send('Hello Server!');
	});
}

// Connection opened
socket.addEventListener('open', function (event) {
    socket.send('Hello Server!');
});

// Listen for messages
socket.addEventListener('message', function (event) {
    //console.log('Message from server ', event.data);
	var div = document.createElement("div");
	div.innerText = event.data;
	textarea.appendChild(div);
});
