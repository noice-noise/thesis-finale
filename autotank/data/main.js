const pumpButton = document.getElementById('pumpButton');
const valveButton = document.getElementById('valveButton');
const mode = document.getElementById('mode');
const txInput = document.getElementById('txInput');
const rxConsole = document.getElementById('rxConsole');
const dataForm = document.getElementById('dataForm');

var Socket;

const init = () => {
  Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
  Socket.onopen = function (event) {
    console.log('WebSocket connection open.');
    console.log(event.data);
  };

  Socket.onerror = function (event) {
    console.error('WebSocket error:');
    console.log(event.data);
  };

  Socket.onmessage = function (event) {
    console.log('WebSocket message:');
    console.log(event.data);
    rxConsole.value = event.data;
  };

  Socket.onopen = function (event) {
    console.log('WebSocket connection close.');
    rxConsole.value = 'Connection disconnected.';
  };
};

const togglePump = () => {
  Socket.send('3');
};

const toggleValve = () => {
  Socket.send('4');
};

const transmitText = (event) => {
  if (event.keyCode == 13) {
    const data = txInput.value;
    Socket.send(data);
    txInput.value = '';
  }
};

const onSocketMessage = (event) => {
  console.log(event.data);
  rxConsole.value += event.data;
};

const onFormSubmit = (event) => {
  event.preventDefault();

  const formData = new FormData(event.target);
  const jsonData = JSON.stringify(Object.fromEntries(formData));

  Socket.send(jsonData);
};

const onModeSelect = (event) => {
  Socket.send(mode.value);
};

pumpButton.addEventListener('click', togglePump);
valveButton.addEventListener('click', toggleValve);
txInput.addEventListener('keydown', transmitText);
dataForm.addEventListener('submit', onFormSubmit);
mode.addEventListener('change', onModeSelect);
