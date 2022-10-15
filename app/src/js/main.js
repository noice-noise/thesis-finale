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
  "sensorDistance": 0,
  "activeLedColor": 0,
  "pumpState": 0,
  "valveState": 0,
  "currentMode": 0,
  "currentLevel": 0,
  "tankHeight": 0,
  "lowLevelThreshold": 0,
  "highLevelThreshold": 0
}`;

let systemState = JSON.parse(systemStateJSON);
console.log(systemState);
console.log(systemState.currentMode);
console.log({ systemState });

var Socket;

const updateStateData = () => {
  try {
    systemState = JSON.parse(systemStateJSON);
    sensorDistance.textContent = systemState.sensorDistance;
    activeLedColor.textContent = systemState.activeLedColor;
    currentMode.textContent = systemState.currentMode;
    currentLevel.textContent = systemState.currentLevel;
    tankHeight.textContent = systemState.tankHeight;
    lowLevelThreshold.textContent = systemState.lowLevelThreshold;
    highLevelThreshold.textContent = systemState.highLevelThreshold;
  } catch {
    console.log('Error updating state data');
  }
};

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
    updateStateData();
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
txInput.addEventListener('keydown', transmitText);
dataForm.addEventListener('submit', onFormSubmit);
mode.addEventListener('change', onModeSelect);

updateStateData();
