function updateTempDisplay(value) {
    document.getElementById('display').textContent = value;
    document.getElementById('temp').value = value;
}

const DEFAULT_BUTTON_BACKGROUND_COLOR = "rgba(255, 255, 255, 0.3)";
const ACTIVE_BUTTON_BACKGROUND_COLOR = "rgba(200, 200, 200, 0.3)";
let powerState;
let activeModeButton;
let activeFanButton;

function setPower(btnPower) {
	if(btnPower) 
		document.getElementById("btn-power").style.backgroundColor = ACTIVE_BUTTON_BACKGROUND_COLOR;
	else
		document.getElementById("btn-power").style.backgroundColor = DEFAULT_BUTTON_BACKGROUND_COLOR;
}

function setActiveModeButton(value) {
	if(activeModeButton)
		document.getElementById(activeModeButton).style.backgroundColor = DEFAULT_BUTTON_BACKGROUND_COLOR;
	switch(value) {
		case "cool":
			activeModeButton = "btn-mode-cool";
			break;
		case "heat":
			activeModeButton = "btn-mode-heat";
			break;
		case "fan":
			activeModeButton = "btn-mode-fan";
			break;
		case "dry":
			activeModeButton = "btn-mode-dry";
			break;
	}
	document.getElementById(activeModeButton).style.backgroundColor = ACTIVE_BUTTON_BACKGROUND_COLOR;
}

function setActiveFanButton(fanMode) {
	if(activeFanButton)
		document.getElementById(activeFanButton).style.backgroundColor = DEFAULT_BUTTON_BACKGROUND_COLOR;
	switch(fanMode) {
		case "auto":
			activeFanButton = "btn-fan-auto";
			break;
		case "low":
			activeFanButton = "btn-fan-low";
			break;
		case "med":
			activeFanButton = "btn-fan-med";
			break;
		case "high":
			activeFanButton = "btn-fan-high";
			break;
	}
	document.getElementById(activeFanButton).style.backgroundColor = ACTIVE_BUTTON_BACKGROUND_COLOR;
}

async function setFan(fan) {
	await sendApiCall('/set-fan?fan=' + fan);
	setActiveFanButton(fan);
}

async function setTemp(temp) {
    await sendApiCall('/set-temp?temp=' + temp);
}

async function setMode(mode) {
	await sendApiCall('/set-mode?mode=' + mode);
	setActiveModeButton(mode);
}

async function sendApiCall(endpoint) {
    const statusDiv = document.getElementById('status');
    statusDiv.textContent = 'Sending...';
    statusDiv.className = 'status';

    try {
        const response = await fetch(endpoint);
        if(response.ok) {
            const message = await response.text();
            statusDiv.textContent = message;
            statusDiv.className = 'status success';
        } else {
            statusDiv.textContent = 'Error: ' + response.status;
            statusDiv.className = 'status error';
        } 
    } catch (error) {
        statusDiv.textContent = 'Error: ' + error;
        statusDiv.className = 'status error';
    }
}

// ----------------------------
// WebSocket real-time updates
// ----------------------------
let ws;

function initWebSocket() {
    ws = new WebSocket(`ws://${window.location.hostname}/ws`);

    ws.onopen = () => {
        console.log("WebSocket connected");
    };

    ws.onmessage = (event) => {
		try {
			console.log(event.data);
			const state = JSON.parse(event.data);
			console.log(state);
			console.log(state.temp);
			updateTempDisplay(state.temp);
			setActiveFanButton(state.fan);
			setPower(state.power);
			
		} catch (error) {
			console.log("Invalid message from server: ", event.data);
		}
    };

    ws.onclose = () => {
        console.log("WebSocket closed. Reconnecting in 2s...");
        setTimeout(initWebSocket, 2000);
    };
}

// Initialize WebSocket connection
initWebSocket();
