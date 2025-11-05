const DEFAULT_BUTTON_BACKGROUND_COLOR = "rgba(255, 255, 255, 0.3)";
const ACTIVE_BUTTON_BACKGROUND_COLOR = "rgba(200, 200, 200, 0.3)";
const GRAY = "rgba(240, 240, 240, 1)";
const GREEN = "rgba(0, 255, 0, 1)";
const RED = "rgba(255, 0, 0, 1)";
const ROOM_TEMP_UPPER_LIMIT = 50;
const ROOM_TEMP_LOWER_LIMIT = -50;
const AC_TEMP_UPPER_LIMIT = 30;
const AC_TEMP_LOWER_LIMIT = 18;
const VALID_MODES = ['cool', 'heat', 'fan', 'dry'];

let powerState;
let activeModeButton;
let activeFanButton;

function validateAutomationInput() {
	const roomTemperatureInputValue = document.getElementById('automation-room-temp-input').value;
	const acTemperatureInputValue = document.getElementById('automation-ac-temp-input').value;
	const acModeInputValue = document.getElementById('automation-ac-mode-input').value;
	
	const roomTemp = parseInt(roomTemperatureInputValue);
	const acTemp = parseInt(acTemperatureInputValue);
	const acMode = acModeInputValue.trim();
	
	if(isNaN(roomTemp) || roomTemp < ROOM_TEMP_LOWER_LIMIT || roomTemp > ROOM_TEMP_UPPER_LIMIT) {
		alert('Please enter a valid room temperature (-50 to 50°C)');
		return false;
	}
	
	if(isNaN(acTemp) || acTemp < AC_TEMP_LOWER_LIMIT || acTemp > AC_TEMP_UPPER_LIMIT) {
		alert('Please enter a valid AC temperature (18 to 30°C)');
		return false;
	}
	
	if(!VALID_MODES.includes(acMode)) {
		alert('Please select a valid AC mode');
		return false;
	}
	
	return true;
}

function updateTempDisplay(value) {
    document.getElementById('display').textContent = value;
    document.getElementById('temp').value = value;
}

function setRoomTempDisplay(roomTemperature) {
	document.getElementById('lbl-room-temp').textContent = roomTemperature + "℃";
}

function setHumidityDisplay(humidity) {
	document.getElementById('lbl-room-humidity').textContent = humidity + "%";
}

function setPower(btnPower) {
	const powerLabelElement = document.getElementById("lbl-power-text");
	const powerButtonElement = document.getElementById("btn-power");
	if(btnPower)  {
		powerLabelElement.style.color = GREEN;
		powerLabelElement.textContent = "On";
		powerButtonElement.style.backgroundColor = ACTIVE_BUTTON_BACKGROUND_COLOR;
	}
	else {
		powerLabelElement.style.color = RED;
		powerLabelElement.textContent = "Off";
		powerButtonElement.style.backgroundColor = DEFAULT_BUTTON_BACKGROUND_COLOR;
	}
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

function setAutomationLabel(automationLabelText) {
	document.getElementById('lbl-automation-active-status').textContent = automationLabelText;
}

function setAutomationLabelColor(automationLabelColor){
	document.getElementById('lbl-automation-active-status').style.color = automationLabelColor;
}

async function setAutomation(automationMode) {
	if(automationMode !== 'off') {
		if(!validateAutomationInput()) {
			return;
		}
		const roomTemp = document.getElementById('automation-room-temp-input').value;
		const acTemp = document.getElementById('automation-ac-temp-input').value;
		const acMode = document.getElementById('automation-ac-mode-input').value;
		
		await sendApiCall(`/set-automation?automation_mode=${automationMode}&room_temp=${roomTemp}&ac_temp=${acTemp}&ac_mode=${acMode}`);
		setAutomationLabel('Active');
		setAutomationLabelColor(GREEN);
	} else {
		await sendApiCall(`/set-automation?automation_mode=off`);
		setAutomationLabel('Inactive');
		setAutomationLabelColor(GRAY);
	}
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
			if(state.type == "stateUpdate") {
				console.log(state.temp);
				updateTempDisplay(state.temp);
				setActiveFanButton(state.fan);
				setActiveModeButton(state.mode);
				state.power ? setPower(true):setPower(false);
			} else if(state.type == "sht31Update") {
				setRoomTempDisplay(state.roomTemperature);
				setHumidityDisplay(state.humidity);
			}
			
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
