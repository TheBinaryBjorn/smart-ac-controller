function updateTempDisplay(value) {
    document.getElementById('display').textContent = value;
    document.getElementById('temp').value = value;
}

async function setFan(fan) {
	await sendApiCall('/set-fan?fan=' + fan);
}

async function setTemp(temp) {
    await sendApiCall('/set-temp?temp=' + temp);
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
			//updateFanDisplay(state.fan);
		} catch (error) {
			console.log("Invalid message from server: ", even.data);
		}
    };

    ws.onclose = () => {
        console.log("WebSocket closed. Reconnecting in 2s...");
        setTimeout(initWebSocket, 2000);
    };
}

// Initialize WebSocket connection
initWebSocket();
