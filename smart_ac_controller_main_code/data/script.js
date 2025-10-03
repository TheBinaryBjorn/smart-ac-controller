function updateTempDisplay(value) {
    document.getElementById('display').textContent = value;
    document.getElementById('temp').value = value;
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
        const temp = event.data;
        updateTempDisplay(temp);
    };

    ws.onclose = () => {
        console.log("WebSocket closed. Reconnecting in 2s...");
        setTimeout(initWebSocket, 2000);
    };
}

// Initialize WebSocket connection
initWebSocket();
