// Create WebSocket connection.
const socket = new WebSocket('ws://192.168.0.31:9876');
const textarea = document.getElementById("textarea");

// Connection opened
socket.addEventListener('open', function (event) {
    socket.send('Hello Server!');
});

// Listen for messages
socket.addEventListener('message', function (event) {
    console.log('Message from server ', event.data);
	// textarea. 
});
