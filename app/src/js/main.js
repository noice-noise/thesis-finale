const pumpButton = document.getElementById('pumpButton');
const valveButton = document.getElementById('valveButton');
const mode = document.getElementById('mode');
const txInput = document.getElementById('txInput');
const rxConsole = document.getElementById('rxConsole');
const dataForm = document.getElementById('dataForm');

const sensorDistance = document.getElementById('sensorDistance');
const activeLedColor = document.getElementById('activeLedColor');
const pumpState = document.getElementById('pumpState');
const currentMode = document.getElementById('currentMode');
const tankHeight = document.getElementById('tankHeight');
const lowLevelThreshold = document.getElementById('lowLevelThreshold');
const highLevelThreshold = document.getElementById('highLevelThreshold');

let systemStateJSON = `{
  "sensorDistance": 88,
  "activeLedColor": 66,
  "pumpState": 0,
  "valveState": 0,
  "currentMode": 67,
  "currentLevel": 3,
  "tankHeight": 30,
  "lowLevelThreshold": 10,
  "highLevelThreshold": 5
}`;

let systemState = JSON.parse(systemStateJSON);
console.log(systemState);
console.log(systemState.currentMode);
console.log({ systemState });

var Socket;

const updateStateData = () => {
  sensorDistance.textContent = systemState.sensorDistance;
  activeLedColor.textContent = systemState.activeLedColor;
  currentMode.textContent = systemState.currentMode;
  currentLevel.textContent = systemState.currentLevel;
  tankHeight.textContent = systemState.tankHeight;
  lowLevelThreshold.textContent = systemState.lowLevelThreshold;
  highLevelThreshold.textContent = systemState.highLevelThreshold;
};

updateStateData();

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
