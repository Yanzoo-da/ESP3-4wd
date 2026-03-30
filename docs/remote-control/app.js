const els = {
  feedback: document.getElementById('feedback'),
  brokerChip: document.getElementById('brokerChip'),
  roverChip: document.getElementById('roverChip'),
  driveChip: document.getElementById('driveChip'),
  speedChip: document.getElementById('speedChip'),
  networkTile: document.getElementById('networkTile'),
  sensorTile: document.getElementById('sensorTile'),
  messageTile: document.getElementById('messageTile'),
  hintTile: document.getElementById('hintTile'),
  host: document.getElementById('host'),
  topic: document.getElementById('topic'),
  wsPort: document.getElementById('wsPort'),
  wsPath: document.getElementById('wsPath'),
  username: document.getElementById('username'),
  password: document.getElementById('password'),
  connectBtn: document.getElementById('connectBtn'),
  disconnectBtn: document.getElementById('disconnectBtn'),
  refreshBtn: document.getElementById('refreshBtn'),
  speed: document.getElementById('speed'),
  speedValue: document.getElementById('speedValue'),
  showButtons: document.getElementById('showButtons'),
  showJoystick: document.getElementById('showJoystick'),
  buttonsPanel: document.getElementById('buttonsPanel'),
  joystickPanel: document.getElementById('joystickPanel'),
  joystick: document.getElementById('joystick'),
  stick: document.getElementById('stick'),
  ledColor: document.getElementById('ledColor'),
  applyLed: document.getElementById('applyLed'),
  autoBtn: document.getElementById('autoBtn'),
  manualBtn: document.getElementById('manualBtn'),
  stopAutoBtn: document.getElementById('stopAutoBtn'),
  policeBtn: document.getElementById('policeBtn'),
  staticBtn: document.getElementById('staticBtn'),
  stop: document.getElementById('stop')
};

const settingsKey = 'esp32-rover-remote-settings';

const state = {
  client: null,
  connected: false,
  driveUi: 'buttons',
  holds: {},
  joystickActive: false,
  joystickTimer: null,
  joystickCommand: 'stop',
  joystickSpeed: 0
};

function feedback(message, isError = false) {
  els.feedback.textContent = message || '';
  els.feedback.style.color = isError ? '#ff8f8f' : '#84f5a8';
}

function trimSlashes(value) {
  return String(value || '').replace(/^\/+|\/+$/g, '');
}

function baseTopic() {
  return trimSlashes(els.topic.value.trim());
}

function mqttTopic(leaf) {
  return `${baseTopic()}/${trimSlashes(leaf)}`;
}

function brokerUrl() {
  const host = els.host.value.trim();
  const port = els.wsPort.value.trim() || '8884';
  const wsPath = els.wsPath.value.trim() || '/mqtt';
  const normalizedPath = wsPath.startsWith('/') ? wsPath : `/${wsPath}`;
  return `wss://${host}:${port}${normalizedPath}`;
}

function saveSettings() {
  const payload = {
    host: els.host.value.trim(),
    topic: els.topic.value.trim(),
    wsPort: els.wsPort.value.trim(),
    wsPath: els.wsPath.value.trim(),
    username: els.username.value.trim()
  };
  localStorage.setItem(settingsKey, JSON.stringify(payload));
}

function loadSettings() {
  try {
    const raw = localStorage.getItem(settingsKey);
    if (!raw) {
      els.wsPort.value = '8884';
      els.wsPath.value = '/mqtt';
      return;
    }

    const payload = JSON.parse(raw);
    els.host.value = payload.host || '';
    els.topic.value = payload.topic || '';
    els.wsPort.value = payload.wsPort || '8884';
    els.wsPath.value = payload.wsPath || '/mqtt';
    els.username.value = payload.username || '';
    els.password.value = '';
  } catch (error) {
    console.error(error);
  }
}

function publish(topicName, payload) {
  if (!state.connected || !state.client) {
    feedback('MQTT broker is not connected.', true);
    return false;
  }

  state.client.publish(topicName, payload);
  return true;
}

function requestStatus() {
  publish(mqttTopic('cmd/status'), 'refresh');
}

function sendDrive(command, speed) {
  const chosenSpeed = speed === undefined ? Number(els.speed.value) : speed;
  publish(mqttTopic('cmd/drive'), `${command}|${chosenSpeed}`);
}

function sendMode(mode) {
  if (mode === 'auto') {
    publish(mqttTopic('cmd/mode'), `auto|${els.speed.value}`);
    return;
  }

  publish(mqttTopic('cmd/mode'), 'manual');
}

function sendLedColor(color) {
  publish(mqttTopic('cmd/led/color'), color);
}

function sendLedBehavior(mode) {
  publish(mqttTopic('cmd/led/behavior'), mode);
}

function handleStatusMessage(message) {
  let data;
  try {
    data = JSON.parse(message);
  } catch (error) {
    feedback('Invalid rover status JSON.', true);
    return;
  }

  els.roverChip.textContent = `Rover: ${data.mqttConnected ? 'mqtt online' : data.mode || 'online'}`;
  els.driveChip.textContent = `Drive: ${data.driveCommand || 'stop'}`;
  els.speedChip.textContent = `Manual ${data.manualSpeed} | Auto ${data.autoSpeed}`;
  els.networkTile.textContent = `Broker ${els.host.value.trim() || '--'} | Topic ${data.mqttTopic || baseTopic() || '--'} | Rover IP ${data.stationIp || data.apIp || '--'}`;
  els.sensorTile.textContent = `Front ${data.frontDistanceCm} cm | Left ${data.leftDistanceCm} cm | Right ${data.rightDistanceCm} cm`;
  els.messageTile.textContent = data.message || '';
  els.hintTile.textContent = data.remoteHint || 'Cloud control is active through MQTT.';
  if (data.selectedColor) {
    els.ledColor.value = data.selectedColor;
  }
}

function attachClientEvents(client) {
  client.on('connect', () => {
    state.connected = true;
    els.brokerChip.textContent = 'Broker: connected';
    feedback('Broker connected.');
    client.subscribe(mqttTopic('state/status'));
    client.subscribe(mqttTopic('state/availability'));
    requestStatus();
  });

  client.on('reconnect', () => {
    els.brokerChip.textContent = 'Broker: reconnecting';
  });

  client.on('close', () => {
    state.connected = false;
    els.brokerChip.textContent = 'Broker: disconnected';
  });

  client.on('offline', () => {
    state.connected = false;
    els.brokerChip.textContent = 'Broker: offline';
  });

  client.on('error', error => {
    feedback(error.message || 'MQTT connection error.', true);
  });

  client.on('message', (topicName, payloadBuffer) => {
    const payload = payloadBuffer.toString();
    if (topicName === mqttTopic('state/status')) {
      handleStatusMessage(payload);
      return;
    }

    if (topicName === mqttTopic('state/availability')) {
      els.roverChip.textContent = `Rover: ${payload}`;
    }
  });
}

function disconnectBroker(showMessage = true) {
  if (state.client) {
    state.client.end(true);
    state.client = null;
  }
  state.connected = false;
  els.brokerChip.textContent = 'Broker: disconnected';
  if (showMessage) {
    feedback('Broker disconnected.');
  }
}

function connectBroker() {
  const host = els.host.value.trim();
  const topic = baseTopic();
  if (!host || !topic) {
    feedback('Broker host and topic base are required.', true);
    return;
  }

  saveSettings();
  disconnectBroker(false);

  const options = {
    username: els.username.value.trim() || undefined,
    password: els.password.value || undefined,
    clientId: `rover-web-${Math.random().toString(16).slice(2, 10)}`,
    reconnectPeriod: 3000,
    connectTimeout: 30000,
    keepalive: 20,
    clean: true
  };

  els.brokerChip.textContent = 'Broker: connecting';
  state.client = mqtt.connect(brokerUrl(), options);
  attachClientEvents(state.client);
}

function bindHold(id, command) {
  const button = document.getElementById(id);
  const start = event => {
    event.preventDefault();
    if (state.holds[id]) return;
    sendDrive(command);
    state.holds[id] = setInterval(() => sendDrive(command), 250);
  };
  const stop = event => {
    if (event) event.preventDefault();
    if (state.holds[id]) {
      clearInterval(state.holds[id]);
      delete state.holds[id];
    }
    sendDrive('stop', 0);
  };

  ['mousedown', 'touchstart'].forEach(name => button.addEventListener(name, start, { passive: false }));
  ['mouseup', 'mouseleave', 'touchend', 'touchcancel'].forEach(name => button.addEventListener(name, stop, { passive: false }));
}

function setDriveUi(kind) {
  state.driveUi = kind;
  const buttonsMode = kind === 'buttons';
  els.buttonsPanel.classList.toggle('hidden', !buttonsMode);
  els.joystickPanel.classList.toggle('hidden', buttonsMode);
  els.showButtons.classList.toggle('active', buttonsMode);
  els.showButtons.classList.toggle('gray', !buttonsMode);
  els.showJoystick.classList.toggle('active', !buttonsMode);
  els.showJoystick.classList.toggle('gray', buttonsMode);
  if (buttonsMode) {
    stopJoystick(true);
  }
}

function resetStick() {
  els.stick.style.left = '50%';
  els.stick.style.top = '50%';
}

function classifyJoystick(nx, ny) {
  const magnitude = Math.min(1, Math.hypot(nx, ny));
  if (magnitude < 0.18) return { cmd: 'stop', speed: 0 };
  const maxSpeed = Number(els.speed.value);
  const speed = Math.max(90, Math.round(maxSpeed * Math.max(0.45, magnitude)));

  if (ny < -0.35) {
    if (nx < -0.28) return { cmd: 'forward-left', speed };
    if (nx > 0.28) return { cmd: 'forward-right', speed };
    return { cmd: 'forward', speed };
  }
  if (ny > 0.35) {
    if (nx < -0.28) return { cmd: 'reverse-left', speed };
    if (nx > 0.28) return { cmd: 'reverse-right', speed };
    return { cmd: 'reverse', speed };
  }
  if (nx < 0) return { cmd: 'left', speed };
  return { cmd: 'right', speed };
}

function updateJoystickCommand(cmd, speed) {
  if (cmd === state.joystickCommand && Math.abs(speed - state.joystickSpeed) < 8) {
    return;
  }
  state.joystickCommand = cmd;
  state.joystickSpeed = speed;
  sendDrive(cmd, speed);
}

function updateJoystickFromPoint(clientX, clientY) {
  const rect = els.joystick.getBoundingClientRect();
  const centerX = rect.left + (rect.width / 2);
  const centerY = rect.top + (rect.height / 2);
  let dx = clientX - centerX;
  let dy = clientY - centerY;
  const limit = 72;
  const distance = Math.hypot(dx, dy);
  if (distance > limit) {
    const ratio = limit / distance;
    dx *= ratio;
    dy *= ratio;
  }
  els.stick.style.left = `calc(50% + ${dx}px)`;
  els.stick.style.top = `calc(50% + ${dy}px)`;
  const next = classifyJoystick(dx / limit, dy / limit);
  updateJoystickCommand(next.cmd, next.speed);
}

function startJoystick(event) {
  if (state.driveUi !== 'joystick') return;
  event.preventDefault();
  state.joystickActive = true;
  if (els.joystick.setPointerCapture) {
    els.joystick.setPointerCapture(event.pointerId);
  }
  if (state.joystickTimer) {
    clearInterval(state.joystickTimer);
  }
  state.joystickTimer = setInterval(() => {
    if (state.joystickActive) {
      sendDrive(state.joystickCommand, state.joystickSpeed);
    }
  }, 250);
  updateJoystickFromPoint(event.clientX, event.clientY);
}

function moveJoystick(event) {
  if (!state.joystickActive || state.driveUi !== 'joystick') return;
  event.preventDefault();
  updateJoystickFromPoint(event.clientX, event.clientY);
}

function stopJoystick(sendStop) {
  const hadCommand = state.joystickActive || state.joystickCommand !== 'stop';
  state.joystickActive = false;
  if (state.joystickTimer) {
    clearInterval(state.joystickTimer);
    state.joystickTimer = null;
  }
  state.joystickCommand = 'stop';
  state.joystickSpeed = 0;
  resetStick();
  if (sendStop && hadCommand) {
    sendDrive('stop', 0);
  }
}

function bindUi() {
  els.connectBtn.addEventListener('click', connectBroker);
  els.disconnectBtn.addEventListener('click', () => disconnectBroker());
  els.refreshBtn.addEventListener('click', requestStatus);
  els.speed.addEventListener('input', event => {
    els.speedValue.textContent = event.target.value;
    els.speedChip.textContent = `Speed: ${event.target.value}`;
  });
  els.showButtons.addEventListener('click', () => setDriveUi('buttons'));
  els.showJoystick.addEventListener('click', () => setDriveUi('joystick'));
  els.stop.addEventListener('click', () => sendDrive('stop', 0));
  els.autoBtn.addEventListener('click', () => sendMode('auto'));
  els.manualBtn.addEventListener('click', () => sendMode('manual'));
  els.stopAutoBtn.addEventListener('click', () => {
    sendMode('manual');
    sendDrive('stop', 0);
  });
  els.applyLed.addEventListener('click', () => sendLedColor(els.ledColor.value));
  els.policeBtn.addEventListener('click', () => sendLedBehavior('police'));
  els.staticBtn.addEventListener('click', () => sendLedBehavior('static'));

  bindHold('forwardLeft', 'forward-left');
  bindHold('forward', 'forward');
  bindHold('forwardRight', 'forward-right');
  bindHold('left', 'left');
  bindHold('right', 'right');
  bindHold('reverse', 'reverse');

  els.joystick.addEventListener('pointerdown', startJoystick);
  els.joystick.addEventListener('pointermove', moveJoystick);
  els.joystick.addEventListener('pointerup', () => stopJoystick(true));
  els.joystick.addEventListener('pointercancel', () => stopJoystick(true));
  els.joystick.addEventListener('pointerleave', event => {
    if (state.joystickActive && event.pointerType === 'mouse') {
      stopJoystick(true);
    }
  });
}

loadSettings();
bindUi();
setDriveUi('buttons');
resetStick();
feedback('Enter your broker details, connect, then control the rover from anywhere.');
