// =====================================================================
//   ESP32 AI Keyboard - CONFIGURATION SECTION
// =====================================================================
//  PLEASE CHANGE THE FOLLOWING VALUES BEFORE UPLOADING:
//   - ssid: your WiFi network name (SSID)
//   - password: your WiFi password
//   - apiKey: your API key for the AI service
//   - apiURL: (optional) change only if using a different proxy/endpoint
// =====================================================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32BLECombo.h>

// ====================== CONFIG ======================
// CHANGE THESE VALUES to match your network and API settings
const char* ssid     = "YOUR_WIFI_SSID";      // Replace with your WiFi network name
const char* password = "YOUR_WIFI_PASSWORD";  // Replace with your WiFi password

const char* apiURL = "https://ai.hackclub.com/proxy/v1/chat/completions"; // Replace with your API endpoint if different
const char* apiKey = "YOUR_API_KEY"; // Replace with your actual API key (e.g., sk-...)

#define KEY_CTRL  128
#define KEY_SHIFT 129
#define KEY_ALT   130
#define KEY_GUI   131
// ====================================================

ESP32BLECombo bleKeyboard;
WebServer server(80);

struct ActionStep {
  String type;
  String value;
};

struct Routine {
  String name;
  ActionStep steps[40];
  int stepCount;
};

#define MAX_ACTIONS 60
#define MAX_ROUTINES 8

ActionStep pendingActions[MAX_ACTIONS];
int actionCount = 0;

ActionStep lastActions[MAX_ACTIONS];
int lastActionCount = 0;

Routine routines[MAX_ROUTINES];
int routineCount = 0;

bool stopRequested = false;
bool thinkDeeper = false;
String currentModel = "~google/gemini-pro-latest";
String currentBLEName = "ESP32-AI";
String inputBuffer = "";
bool bleNameSetToIP = false;

// Store the AI-generated dialogue for the overlay
String lastDialogue = "";

void connectWiFi();
void setBLENameToIP();
String queryAI(String userPrompt);
void parseAndQueueActions(const String& aiResponse);
void executeBLEActions();
void processUserInput(String input);
void handleRoot();
void handleAsk();
void handleExecute();
void handleReRun();
void handleStop();
void handleSetThink();
void handleSetModel();
void handleStatus();
void handleSaveRoutine();
void handleGetRoutines();
void handleRunRoutine();
void handleDeleteRoutine();
String escapeJson(const String& s);
bool ensureBLEConnected();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n======================================"));
  Serial.println(F("   ESP32 AI Keyboard                 "));
  Serial.println(F("======================================"));

  ESP32BLEComboConfig config;
  config.deviceName = "ESP32-AI";
  config.manufacturer = "Espressif";
  bleKeyboard.begin(config);

  connectWiFi();
  setBLENameToIP();

  if (MDNS.begin("esp-ai")) {
    Serial.println(F("[mDNS] http://esp-ai.local"));
  }

  server.on("/", handleRoot);
  server.on("/ask", HTTP_POST, handleAsk);
  server.on("/execute", HTTP_POST, handleExecute);
  server.on("/rerun", HTTP_POST, handleReRun);
  server.on("/stop", HTTP_POST, handleStop);
  server.on("/setthink", HTTP_POST, handleSetThink);
  server.on("/setmodel", HTTP_POST, handleSetModel);
  server.on("/status", handleStatus);
  server.on("/saveroutine", HTTP_POST, handleSaveRoutine);
  server.on("/routines", handleGetRoutines);
  server.on("/runroutine", HTTP_POST, handleRunRoutine);
  server.on("/deleteroutine", HTTP_POST, handleDeleteRoutine);
  server.begin();

  Serial.println(F("\n[READY]"));
  Serial.print(F("Web UI → http://"));
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED && !bleNameSetToIP) {
    setBLENameToIP();
  }

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      inputBuffer.trim();
      if (inputBuffer.length() > 0) {
        processUserInput(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
  delay(5);
}

void connectWiFi() {
  Serial.print(F("[WiFi] Connecting"));
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\n[WiFi] Connected"));
    Serial.print(F("[WiFi] IP: "));
    Serial.println(WiFi.localIP());
  }
}

void setBLENameToIP() {
  if (WiFi.status() != WL_CONNECTED) return;

  String ipName = WiFi.localIP().toString();

  bleKeyboard.end();
  delay(400);

  ESP32BLEComboConfig config;
  config.deviceName = ipName.c_str();
  config.manufacturer = "Espressif";
  bleKeyboard.begin(config);

  currentBLEName = ipName;
  bleNameSetToIP = true;

  Serial.print(F("[BLE] Name set to IP: "));
  Serial.println(ipName);
}

// Try to ensure BLE is connected; return true if connected or ready
bool ensureBLEConnected() {
  if (bleKeyboard.isConnected()) return true;
  // Attempt to reconnect by re-initializing? The library may not support reconnection.
  // We'll just return false and let the user know.
  return false;
}

// ====================== FULL UI ======================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>ESP32 AI</title>
<style>
  :root {
    --bg: #000000;
    --card: rgba(255,255,255,0.08);
    --border: rgba(255,255,255,0.12);
    --text: #f5f5f7;
    --text2: #98989d;
    --accent: #0A84FF;
    --green: #30D158;
    --red: #FF453A;
    --radius: 13px;
  }

  * { margin:0; padding:0; box-sizing:border-box; -webkit-tap-highlight-color:transparent; }

  body {
    font-family: -apple-system, BlinkMacSystemFont, "SF Pro Text", "Segoe UI", system-ui, sans-serif;
    background: var(--bg);
    color: var(--text);
    min-height: 100vh;
    overflow-x: hidden;
    -webkit-font-smoothing: antialiased;
    font-size: 15px;
    line-height: 1.45;
    letter-spacing: -0.01em;
  }

  .bg {
    position: fixed;
    inset: 0;
    z-index: 0;
    overflow: hidden;
    pointer-events: none;
  }
  .glow {
    position: absolute;
    border-radius: 50%;
    filter: blur(140px);
    opacity: 0.18;
    animation: drift 24s ease-in-out infinite;
  }
  .g1 { width:340px; height:340px; background:#0A84FF; top:-100px; left:-80px; }
  .g2 { width:280px; height:280px; background:#BF5AF2; bottom:10%; right:-70px; animation-delay:-9s; }
  .g3 { width:200px; height:200px; background:#30D158; bottom:-60px; left:25%; animation-delay:-15s; }

  @keyframes drift {
    0%,100% { transform: translate(0,0) scale(1); }
    50% { transform: translate(40px,-30px) scale(1.06); }
  }

  .container {
    position: relative;
    z-index: 1;
    max-width: 400px;
    margin: 0 auto;
    padding: 28px 18px 60px;
  }

  header {
    text-align: center;
    margin-bottom: 26px;
  }

  .logo {
    width: 60px;
    height: 60px;
    border-radius: 16px;
    margin-bottom: 12px;
    object-fit: contain;
    box-shadow: 0 8px 24px rgba(0,0,0,0.6);
  }

  header h1 {
    font-size: 22px;
    font-weight: 620;
    letter-spacing: -0.4px;
    color: var(--text);
  }
  header p {
    color: var(--text2);
    font-size: 13px;
    font-weight: 400;
    margin-top: 4px;
  }

  /* Thinking Overlay */
  #thinkingOverlay {
    position: fixed;
    inset: 0;
    background: rgba(0,0,0,0.78);
    backdrop-filter: blur(24px);
    -webkit-backdrop-filter: blur(24px);
    display: none;
    align-items: center;
    justify-content: center;
    z-index: 100;
    flex-direction: column;
    gap: 20px;
  }

  #thinkingOverlay.show {
    display: flex;
    animation: fadeIn 0.25s ease;
  }

  #thinkingOverlay img {
    width: 100px;
    height: 100px;
    border-radius: 22px;
    animation: pulse 1.8s ease-in-out infinite;
  }

  #thinkingText {
    color: #e5e5ea;
    font-size: 16px;
    font-weight: 480;
    text-align: center;
    max-width: 260px;
    line-height: 1.35;
    letter-spacing: -0.01em;
  }

  @keyframes pulse {
    0%, 100% { transform: scale(1); }
    50% { transform: scale(1.06); }
  }

  @keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
  }

  /* Task Execution Overlay */
  #taskOverlay {
    position: fixed;
    inset: 0;
    background: rgba(0,0,0,0.88);
    backdrop-filter: blur(30px);
    -webkit-backdrop-filter: blur(30px);
    display: none;
    align-items: center;
    justify-content: center;
    z-index: 200;
    flex-direction: column;
    gap: 24px;
    padding: 24px;
  }

  #taskOverlay.show {
    display: flex;
    animation: fadeIn 0.3s ease;
  }

  #taskImage {
    width: 200px;
    height: 200px;
    border-radius: 28px;
    object-fit: contain;
    background: rgba(0,0,0,0.3);
    box-shadow: 0 20px 40px rgba(0,0,0,0.7);
    transition: opacity 0.4s ease;
  }

  #taskDialogue {
    color: #e5e5ea;
    font-size: 19px;
    font-weight: 510;
    text-align: center;
    max-width: 320px;
    line-height: 1.4;
    min-height: 2.8em;
    transition: opacity 0.3s ease;
    letter-spacing: -0.01em;
  }

  /* Tabs */
  .tabs {
    display: flex;
    background: rgba(255,255,255,0.06);
    border-radius: 12px;
    padding: 3px;
    margin-bottom: 18px;
    border: 1px solid rgba(255,255,255,0.08);
  }
  .tab {
    flex: 1;
    text-align: center;
    padding: 9px 0;
    font-size: 13px;
    font-weight: 540;
    border-radius: 9px;
    cursor: pointer;
    color: var(--text2);
    transition: all 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
    letter-spacing: -0.01em;
  }
  .tab.active {
    background: rgba(255,255,255,0.12);
    color: var(--text);
  }

  .tab-content { display: none; }
  .tab-content.active {
    display: block;
    animation: popIn 0.35s cubic-bezier(0.34, 1.56, 0.64, 1);
  }

  @keyframes popIn {
    0% { opacity: 0; transform: scale(0.98) translateY(6px); }
    100% { opacity: 1; transform: scale(1) translateY(0); }
  }

  .card {
    background: var(--card);
    backdrop-filter: blur(28px);
    -webkit-backdrop-filter: blur(28px);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 16px;
    margin-bottom: 12px;
    transition: transform 0.2s ease;
  }

  textarea {
    width: 100%;
    height: 88px;
    background: rgba(255,255,255,0.06);
    border: 1px solid rgba(255,255,255,0.10);
    border-radius: 12px;
    color: var(--text);
    padding: 12px 14px;
    font-size: 15px;
    resize: none;
    outline: none;
    font-family: inherit;
    letter-spacing: -0.01em;
  }
  textarea:focus {
    border-color: var(--accent);
  }

  .btn-row {
    display: grid;
    grid-template-columns: 1.3fr 1fr 0.85fr 0.85fr;
    gap: 9px;
    margin-top: 12px;
  }

  button {
    border: none;
    border-radius: 12px;
    padding: 12px 6px;
    font-size: 13px;
    font-weight: 590;
    cursor: pointer;
    transition: transform 0.2s cubic-bezier(0.34, 1.56, 0.64, 1), opacity 0.2s;
    letter-spacing: -0.01em;
  }
  button:active { transform: scale(0.94); }
  button:disabled { opacity: 0.4; transform: none; cursor: not-allowed; }

  .btn-blue  { background: var(--accent); color: white; }
  .btn-green { background: var(--green); color: #000; }
  .btn-red   { background: var(--red); color: white; }
  .btn-purple { background: #7D55C7; color: white; }

  .btn-execute {
    background: linear-gradient(145deg, #5E5CE6, #7D55C7);
    color: white;
    font-weight: 630;
    font-size: 13px;
    padding: 12px 8px;
    border-radius: 12px;
    box-shadow: 0 4px 14px rgba(94, 92, 230, 0.5);
    transition: all 0.25s ease;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    letter-spacing: 0.2px;
  }
  .btn-execute:hover {
    transform: scale(1.02);
    box-shadow: 0 6px 18px rgba(94, 92, 230, 0.7);
  }
  .btn-execute:active {
    transform: scale(0.94);
  }
  .btn-execute svg {
    width: 16px;
    height: 16px;
    fill: white;
    flex-shrink: 0;
  }

  #reply {
    margin-top: 14px;
    font-size: 14px;
    line-height: 1.5;
    color: var(--text2);
    min-height: 42px;
    white-space: pre-wrap;
    letter-spacing: -0.01em;
  }

  .status {
    margin-top: 10px;
    font-size: 12px;
    color: var(--text2);
  }

  .badge {
    background: rgba(48,209,88,0.18);
    color: var(--green);
    padding: 3px 9px;
    border-radius: 20px;
    font-size: 11px;
    font-weight: 590;
  }

  select {
    width: 100%;
    background: rgba(255,255,255,0.06);
    border: 1px solid rgba(255,255,255,0.10);
    border-radius: 12px;
    color: var(--text);
    padding: 11px 14px;
    font-size: 14px;
    outline: none;
    margin-top: 8px;
    font-family: inherit;
    letter-spacing: -0.01em;
  }

  .setting-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }
  .setting-row h3 { font-size: 14px; font-weight: 590; }
  .setting-row p { font-size: 11px; color: var(--text2); margin-top: 3px; }

  .switch {
    position: relative;
    width: 45px;
    height: 26px;
  }
  .switch input { opacity:0; width:0; height:0; }
  .slider {
    position: absolute;
    cursor: pointer;
    inset: 0;
    background: rgba(255,255,255,0.16);
    border-radius: 26px;
    transition: 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
  }
  .slider:before {
    content: "";
    position: absolute;
    height: 20px;
    width: 20px;
    left: 3px;
    bottom: 3px;
    background: white;
    border-radius: 50%;
    transition: 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
  }
  input:checked + .slider { background: var(--accent); }
  input:checked + .slider:before { transform: translateX(19px); }

  .routine-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 11px 0;
    border-bottom: 1px solid rgba(255,255,255,0.06);
  }
  .routine-item:last-child { border-bottom: none; }
  .routine-name { font-size: 14px; font-weight: 500; }
  .routine-actions { display: flex; gap: 7px; }
  .routine-actions button {
    padding: 7px 11px;
    font-size: 12px;
    border-radius: 10px;
  }

  .save-row {
    display: flex;
    gap: 9px;
  }
  .save-row input {
    flex: 1;
    background: rgba(255,255,255,0.06);
    border: 1px solid rgba(255,255,255,0.10);
    border-radius: 12px;
    color: var(--text);
    padding: 11px 14px;
    font-size: 14px;
    outline: none;
    font-family: inherit;
  }

  .empty {
    text-align: center;
    color: var(--text2);
    font-size: 13px;
    padding: 18px 0;
  }

  .footer {
    text-align: center;
    margin-top: 18px;
    font-size: 11px;
    color: var(--text2);
    letter-spacing: 0.01em;
  }
</style>
</head>
<body>

  <!-- Thinking Overlay -->
  <div id="thinkingOverlay">
    <img src="https://cdn.hackclub.com/019d730d-3223-7023-a3d3-5b767cf50c61/n9Fdod5XHsv83GJUtOJYoA5gJtHE2jYvon66s4hvo28" alt="Thinking">
    <p id="thinkingText">Thinking...</p>
  </div>

  <!-- Task Execution Overlay -->
  <div id="taskOverlay">
    <img id="taskImage" src="" alt="Task stage">
    <p id="taskDialogue">Starting task...</p>
  </div>

  <!-- Audio for task execution -->
  <audio id="taskAudio" preload="auto">
    <source src="https://raw.githubusercontent.com/shift-xd/Assets-Random-/main/freesound_community-computer-startup-6331.mp3" type="audio/mpeg">
  </audio>

  <!-- Thinking sound -->
  <audio id="thinkingSound" loop preload="auto">
    <source src="https://raw.githubusercontent.com/shift-xd/Assets-Random-/main/freesound_community-sipping-a-juice-box-105614.mp3" type="audio/mpeg">
  </audio>

  <div class="bg">
    <div class="glow g1"></div>
    <div class="glow g2"></div>
    <div class="glow g3"></div>
  </div>

  <div class="container">
    <header>
      <img class="logo" src="https://cdn.hackclub.com/019d730a-b2f2-7daf-832f-90df3b78e4eb/TItCSknK9qP-XN9oCqAz6kkwVdjDarWU8468JgO1osM" alt="Logo">
      <h1>ESP32 AI</h1>
      <p>Desktop Automation</p>
    </header>

    <div class="tabs">
      <div class="tab active" onclick="switchTab('control')">Control</div>
      <div class="tab" onclick="switchTab('routines')">Routines</div>
    </div>

    <!-- CONTROL TAB -->
    <div id="control" class="tab-content active">
      <div class="card">
        <textarea id="prompt" placeholder="What should I do?"></textarea>

        <div class="btn-row">
          <button class="btn-blue" id="askBtn" onclick="askAI()">Ask AI</button>
          <button class="btn-execute" id="execBtn" onclick="runActions()">
            <svg viewBox="0 0 24 24"><polygon points="5,3 19,12 5,21"/></svg>
            EXECUTE
          </button>
          <button class="btn-purple" id="rerunBtn" onclick="rerunActions()" disabled>Re‑run</button>
          <button class="btn-red" onclick="stopActions()">Stop</button>
        </div>

        <div id="reply">Ready.</div>
        <div class="status" id="status"></div>
      </div>

      <div class="card">
        <div class="setting-row">
          <div>
            <h3>Model</h3>
            <p>Choose AI model</p>
          </div>
        </div>
        <select id="modelSelect" onchange="changeModel()">
          <option value="~google/gemini-pro-latest">Gemini Pro</option>
          <option value="google/gemini-3.5-flash-lite">Gemini 3.5 Flash Lite</option>
        </select>
      </div>

      <div class="card">
        <div class="setting-row">
          <div>
            <h3>Think Deeper</h3>
            <p>More careful reasoning</p>
          </div>
          <label class="switch">
            <input type="checkbox" id="thinkToggle" onchange="toggleThink()">
            <span class="slider"></span>
          </label>
        </div>
      </div>
    </div>

    <!-- ROUTINES TAB -->
    <div id="routines" class="tab-content">
      <div class="card">
        <div style="font-size:13px;font-weight:590;margin-bottom:10px;color:var(--text2)">Save current actions</div>
        <div class="save-row">
          <input type="text" id="routineName" placeholder="Routine name">
          <button class="btn-blue" style="padding:10px 14px" onclick="saveRoutine()">Save</button>
        </div>
      </div>

      <div class="card">
        <div style="font-size:13px;font-weight:590;margin-bottom:6px;color:var(--text2)">Saved</div>
        <div id="routineList">
          <div class="empty">No routines yet</div>
        </div>
      </div>
    </div>

    <div class="footer" id="ipInfo">Loading...</div>
  </div>

<script>
  // ============= TASK IMAGES =============
  const taskImages = [
    "https://raw.githubusercontent.com/shift-xd/Assets-Random-/main/tHIKTHAK/download%20(1).jpeg",
    "https://raw.githubusercontent.com/shift-xd/Assets-Random-/main/tHIKTHAK/download.jpeg",
    "https://raw.githubusercontent.com/shift-xd/Assets-Random-/main/tHIKTHAK/download%20(3).jpeg",
    "https://raw.githubusercontent.com/shift-xd/Assets-Random-/main/tHIKTHAK/download%20(4).jpeg"
  ];

  // ============= FALLBACK DIALOGUES (only used if AI doesn't provide one) =============
  const fallbackDialogues = [
    "Let's make some digital magic happen!",
    "Automation with a side of sarcasm…",
    "Hold on, I'm rewriting the laws of physics.",
    "Just a few keystrokes to world domination.",
    "I'd tell you a joke, but I'm busy typing.",
    "Ctrl+Alt+Del your worries away.",
    "Mistakes? I don't know her.",
    "Beep boop – I'm a keyboard, not a magician.",
    "Watch me press buttons like a pro.",
    "This is where the fun begins.",
    "I've got 99 problems but a key ain't one.",
    "Simulating human typing… with extra flair.",
    "No coffee required – I run on electricity.",
    "Let's get this bread (and bytes).",
    "Typing faster than your ex's excuses.",
    "I'm not a robot, I'm just very dedicated.",
    "If at first you don't succeed, blame the keyboard.",
    "Automation: because humans are overrated.",
    "Shhh… I'm listening to the keys.",
    "Prepare for absolute productivity.",
    "I'm not procrastinating – I'm planning.",
    "Just like magic, but with more cables.",
    "One small step for man, one giant leap for automation.",
    "I'm not lazy – I'm efficient.",
    "Who needs fingers when you have AI?"
  ];

  let taskInterval = null;
  let taskRunning = false;
  let currentImageIndex = 0;

  // Store AI-generated dialogue and conversational reply
  window.lastAIDialogue = "";
  window.lastConversationalReply = "";
  window.hasAIDialogue = false;

  function randomFallbackDialogue() {
    return fallbackDialogues[Math.floor(Math.random() * fallbackDialogues.length)];
  }

  // ============= TASK SEQUENCE =============
  function startTaskSequence(staticDialogue) {
    const overlay = document.getElementById('taskOverlay');
    const img = document.getElementById('taskImage');
    const dialogue = document.getElementById('taskDialogue');

    currentImageIndex = Math.floor(Math.random() * taskImages.length);
    img.src = taskImages[currentImageIndex];

    if (staticDialogue && staticDialogue.trim().length > 0) {
      dialogue.textContent = staticDialogue;
    } else {
      dialogue.textContent = randomFallbackDialogue();
    }

    overlay.classList.add('show');

    taskInterval = setInterval(() => {
      currentImageIndex = (currentImageIndex + 1) % taskImages.length;
      img.src = taskImages[currentImageIndex];
      // Keep dialogue static if we have a custom one, else cycle fallback
      if (!staticDialogue || staticDialogue.trim().length === 0) {
        dialogue.textContent = randomFallbackDialogue();
      }
    }, 2000);
  }

  function stopTaskSequence() {
    if (taskInterval) {
      clearInterval(taskInterval);
      taskInterval = null;
    }
    document.getElementById('taskOverlay').classList.remove('show');
    const audio = document.getElementById('taskAudio');
    audio.pause();
    audio.currentTime = 0;
    taskRunning = false;
    document.getElementById('execBtn').disabled = false;
    document.getElementById('rerunBtn').disabled = (window.lastActionCount === 0);
  }

  // ============= THINKING OVERLAY =============
  const thinkingLines = [
    "Sipping on some knowledge...",
    "Crunching the juice box of wisdom...",
    "Almost there...",
    "Thinking real hard...",
    "Consulting the straw of intelligence...",
    "Brewing a reply...",
    "One more sip...",
    "Processing with maximum juice...",
    "Hmm... interesting...",
    "Let me think about that..."
  ];

  function showThinking(show) {
    const el = document.getElementById('thinkingOverlay');
    const sound = document.getElementById('thinkingSound');
    const text = document.getElementById('thinkingText');

    if (show) {
      text.innerText = thinkingLines[Math.floor(Math.random() * thinkingLines.length)];
      el.classList.add('show');
      sound.currentTime = 0;
      sound.play().catch(()=>{});
    } else {
      el.classList.remove('show');
      sound.pause();
    }
  }

  // ============= TABS =============
  let currentTab = 'control';
  function switchTab(tab) {
    if (tab === currentTab) return;
    currentTab = tab;
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
    if (tab === 'control') {
      document.querySelectorAll('.tab')[0].classList.add('active');
      document.getElementById('control').classList.add('active');
    } else {
      document.querySelectorAll('.tab')[1].classList.add('active');
      document.getElementById('routines').classList.add('active');
      loadRoutines();
    }
  }

  // ============= STATUS =============
  async function loadStatus() {
    try {
      const res = await fetch('/status');
      const data = await res.json();
      document.getElementById('ipInfo').innerText = `${data.bleName}  ·  ${data.ip}`;
      document.getElementById('thinkToggle').checked = data.thinkDeeper;
      document.getElementById('modelSelect').value = data.model || '~google/gemini-pro-latest';
      window.lastActionCount = data.lastActions || 0;
      if (data.lastActions > 0) {
        document.getElementById('rerunBtn').disabled = false;
      } else {
        document.getElementById('rerunBtn').disabled = true;
      }
    } catch(e) {}
  }
  loadStatus();

  // ============= ASK AI =============
  async function askAI() {
    const prompt = document.getElementById('prompt').value.trim();
    if (!prompt) return;
    const btn = document.getElementById('askBtn');
    btn.disabled = true;
    showThinking(true);
    try {
      const res = await fetch('/ask', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'prompt=' + encodeURIComponent(prompt)
      });
      const data = await res.json();
      const reply = data.reply || 'No response';
      document.getElementById('reply').innerText = reply;
      // Store conversational reply as fallback dialogue
      window.lastConversationalReply = reply;
      // Store the specific dialogue if provided
      window.lastAIDialogue = data.dialogue || reply;
      window.hasAIDialogue = (data.dialogue && data.dialogue.length > 0) || (reply.length > 0);
      if (data.actions > 0) {
        document.getElementById('status').innerHTML = `<span class="badge">${data.actions} actions</span>`;
        document.getElementById('rerunBtn').disabled = false;
        window.lastActionCount = data.actions;
      } else {
        document.getElementById('status').innerText = 'No actions generated. Check your prompt or AI response.';
        document.getElementById('rerunBtn').disabled = true;
        window.lastActionCount = 0;
      }
    } catch(e) {
      document.getElementById('reply').innerText = 'Connection failed';
      document.getElementById('status').innerText = 'Error: ' + e.message;
    }
    showThinking(false);
    btn.disabled = false;
  }

  // ============= RUN ACTIONS =============
  async function runActions() {
    if (taskRunning) return;
    taskRunning = true;
    document.getElementById('execBtn').disabled = true;
    document.getElementById('rerunBtn').disabled = true;
    // Use the AI-generated dialogue if available, else conversational reply, else fallback
    let dialogue = "";
    if (window.hasAIDialogue) {
      dialogue = window.lastAIDialogue;
    } else if (window.lastConversationalReply.length > 0) {
      dialogue = window.lastConversationalReply;
    }
    startTaskSequence(dialogue);
    const audio = document.getElementById('taskAudio');
    audio.volume = 0.5;
    audio.currentTime = 0;
    audio.play().catch(() => {});
    document.getElementById('status').innerText = 'Running...';
    try {
      const res = await fetch('/execute', { method: 'POST' });
      const data = await res.json();
      document.getElementById('status').innerText = data.message || 'Finished';
      if (data.message && data.message.includes('error')) {
        // Show error, but overlay will close
      }
    } catch(e) {
      document.getElementById('status').innerText = 'Execution error: ' + e.message;
    }
    stopTaskSequence();
    setTimeout(loadStatus, 500);
  }

  // ============= RE-RUN =============
  async function rerunActions() {
    if (taskRunning) return;
    if (window.lastActionCount === 0) {
      document.getElementById('status').innerText = 'No actions to re-run';
      return;
    }
    taskRunning = true;
    document.getElementById('execBtn').disabled = true;
    document.getElementById('rerunBtn').disabled = true;
    let dialogue = "";
    if (window.hasAIDialogue) {
      dialogue = window.lastAIDialogue;
    } else if (window.lastConversationalReply.length > 0) {
      dialogue = window.lastConversationalReply;
    }
    startTaskSequence(dialogue);
    const audio = document.getElementById('taskAudio');
    audio.volume = 0.5;
    audio.currentTime = 0;
    audio.play().catch(() => {});
    document.getElementById('status').innerText = 'Re-running...';
    try {
      const res = await fetch('/rerun', { method: 'POST' });
      const data = await res.json();
      document.getElementById('status').innerText = data.message || 'Re-run finished';
    } catch(e) {
      document.getElementById('status').innerText = 'Re-run error: ' + e.message;
    }
    stopTaskSequence();
    setTimeout(loadStatus, 500);
  }

  // ============= STOP =============
  async function stopActions() {
    await fetch('/stop', { method: 'POST' });
    document.getElementById('status').innerText = 'Stopped';
    if (taskRunning) stopTaskSequence();
    document.getElementById('execBtn').disabled = false;
    document.getElementById('rerunBtn').disabled = (window.lastActionCount === 0);
  }

  // ============= SETTINGS =============
  async function toggleThink() {
    const on = document.getElementById('thinkToggle').checked;
    await fetch('/setthink', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: 'value=' + (on ? '1' : '0')
    });
  }

  async function changeModel() {
    const model = document.getElementById('modelSelect').value;
    await fetch('/setmodel', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: 'model=' + encodeURIComponent(model)
    });
  }

  // ============= ROUTINES =============
  async function loadRoutines() {
    try {
      const res = await fetch('/routines');
      const data = await res.json();
      const list = document.getElementById('routineList');
      if (!data.routines || data.routines.length === 0) {
        list.innerHTML = '<div class="empty">No routines yet</div>';
        return;
      }
      list.innerHTML = '';
      data.routines.forEach((r, i) => {
        const div = document.createElement('div');
        div.className = 'routine-item';
        div.innerHTML = `
          <div class="routine-name">${r.name}</div>
          <div class="routine-actions">
            <button class="btn-green" onclick="runRoutine(${i})">Run</button>
            <button class="btn-red" onclick="deleteRoutine(${i})">Del</button>
          </div>
        `;
        list.appendChild(div);
      });
    } catch(e) {
      document.getElementById('routineList').innerHTML = '<div class="empty">Failed to load</div>';
    }
  }

  async function saveRoutine() {
    const name = document.getElementById('routineName').value.trim();
    if (!name) return;
    const res = await fetch('/saveroutine', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: 'name=' + encodeURIComponent(name)
    });
    const data = await res.json();
    if (data.ok) {
      document.getElementById('routineName').value = '';
      loadRoutines();
    }
  }

  async function runRoutine(index) {
    document.getElementById('status').innerText = 'Running routine...';
    // No AI dialogue for routines, clear flags
    window.hasAIDialogue = false;
    window.lastAIDialogue = "";
    window.lastConversationalReply = "";
    const res = await fetch('/runroutine', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: 'index=' + index
    });
    const data = await res.json();
    document.getElementById('status').innerText = data.message;
  }

  async function deleteRoutine(index) {
    await fetch('/deleteroutine', {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: 'index=' + index
    });
    loadRoutines();
  }
</script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ====================== API HANDLERS ======================
void handleAsk() {
  if (!server.hasArg("prompt")) {
    server.send(400, "application/json", "{\"error\":\"missing prompt\"}");
    return;
  }
  String prompt = server.arg("prompt");
  String reply = queryAI(prompt);
  // Extract dialogue from reply
  String dialogue = "";
  int dStart = reply.indexOf("<DIALOGUE>");
  int dEnd = reply.indexOf("</DIALOGUE>");
  if (dStart >= 0 && dEnd > dStart) {
    dialogue = reply.substring(dStart + 10, dEnd);
    dialogue.trim();
  }
  // Remove <DIALOGUE> tags from the reply before sending to UI
  String cleanReply = reply;
  if (dStart >= 0 && dEnd > dStart) {
    cleanReply = reply.substring(0, dStart) + reply.substring(dEnd + 11);
    cleanReply.trim();
  }
  parseAndQueueActions(reply); // still parse actions from full reply
  String json = "{\"reply\":\"" + escapeJson(cleanReply) + "\",\"dialogue\":\"" + escapeJson(dialogue) + "\",\"actions\":" + String(actionCount) + "}";
  server.send(200, "application/json", json);
}

void handleExecute() {
  if (!bleKeyboard.isConnected()) {
    server.send(200, "application/json", "{\"message\":\"Error: Bluetooth not connected. Please pair your device.\"}");
    return;
  }
  if (actionCount == 0) {
    server.send(200, "application/json", "{\"message\":\"No actions ready. Ask the AI first.\"}");
    return;
  }
  stopRequested = false;
  executeBLEActions();
  server.send(200, "application/json", "{\"message\":\"Finished\"}");
}

void handleReRun() {
  if (!bleKeyboard.isConnected()) {
    server.send(200, "application/json", "{\"message\":\"Error: Bluetooth not connected.\"}");
    return;
  }
  if (lastActionCount == 0) {
    server.send(200, "application/json", "{\"message\":\"No actions to re-run.\"}");
    return;
  }
  actionCount = lastActionCount;
  for (int i = 0; i < actionCount; i++) {
    pendingActions[i] = lastActions[i];
  }
  stopRequested = false;
  executeBLEActions();
  server.send(200, "application/json", "{\"message\":\"Re-run finished\"}");
}

void handleStop() {
  stopRequested = true;
  bleKeyboard.releaseAll();
  actionCount = 0;
  server.send(200, "application/json", "{\"message\":\"Stopped\"}");
}

void handleSetThink() {
  if (server.hasArg("value")) thinkDeeper = (server.arg("value") == "1");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleSetModel() {
  if (server.hasArg("model")) {
    currentModel = server.arg("model");
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleStatus() {
  String json = "{\"ip\":\"" + WiFi.localIP().toString() +
                "\",\"bleName\":\"" + currentBLEName +
                "\",\"thinkDeeper\":" + String(thinkDeeper ? "true" : "false") +
                ",\"model\":\"" + currentModel +
                "\",\"actions\":" + String(actionCount) +
                ",\"lastActions\":" + String(lastActionCount) + "}";
  server.send(200, "application/json", json);
}

void handleSaveRoutine() {
  if (!server.hasArg("name") || actionCount == 0 || routineCount >= MAX_ROUTINES) {
    server.send(200, "application/json", "{\"ok\":false}");
    return;
  }
  String name = server.arg("name");
  name.trim();
  if (name.length() < 1) {
    server.send(200, "application/json", "{\"ok\":false}");
    return;
  }

  routines[routineCount].name = name;
  routines[routineCount].stepCount = min(actionCount, 40);
  for (int i = 0; i < routines[routineCount].stepCount; i++) {
    routines[routineCount].steps[i] = pendingActions[i];
  }
  routineCount++;
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleGetRoutines() {
  String json = "{\"routines\":[";
  for (int i = 0; i < routineCount; i++) {
    if (i > 0) json += ",";
    json += "{\"name\":\"" + escapeJson(routines[i].name) + "\"}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleRunRoutine() {
  if (!server.hasArg("index")) {
    server.send(200, "application/json", "{\"message\":\"Missing index\"}");
    return;
  }
  int idx = server.arg("index").toInt();
  if (idx < 0 || idx >= routineCount) {
    server.send(200, "application/json", "{\"message\":\"Invalid routine\"}");
    return;
  }

  actionCount = routines[idx].stepCount;
  for (int i = 0; i < actionCount; i++) {
    pendingActions[i] = routines[idx].steps[i];
  }

  stopRequested = false;
  executeBLEActions();
  server.send(200, "application/json", "{\"message\":\"Routine finished\"}");
}

void handleDeleteRoutine() {
  if (!server.hasArg("index")) {
    server.send(200, "application/json", "{\"ok\":false}");
    return;
  }
  int idx = server.arg("index").toInt();
  if (idx < 0 || idx >= routineCount) {
    server.send(200, "application/json", "{\"ok\":false}");
    return;
  }

  for (int i = idx; i < routineCount - 1; i++) {
    routines[i] = routines[i + 1];
  }
  routineCount--;
  server.send(200, "application/json", "{\"ok\":true}");
}

String escapeJson(const String& s) {
  String out;
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

// ====================== AI + EXECUTION ======================
void processUserInput(String input) {
  if (input.equalsIgnoreCase("start")) {
    executeBLEActions();
    return;
  }
  String reply = queryAI(input);
  parseAndQueueActions(reply);
}

String queryAI(String userPrompt) {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, apiURL)) return "HTTPS failed";

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + apiKey);
  http.setTimeout(25000);

#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  DynamicJsonDocument doc(2800);
#endif

  doc["model"] = currentModel;

  JsonArray messages = doc.createNestedArray("messages");

  String systemPrompt =
    "You are a helpful AI that controls a computer via Bluetooth keyboard.\n"
    "Reply conversationally first, but also include a <DIALOGUE> tag with a short, funny or interesting sentence that describes what you are doing. For example: <DIALOGUE>Let's hack the mainframe!</DIALOGUE>.\n"
    "Put actions inside <ACTION>...</ACTION>.\n"
    "Prefer full URLs. Use console only when needed to click elements.\n"
    "Brave path: /var/lib/flatpak/exports/bin/com.brave.Browser\n"
    "Tags: KEY:ENTER, TYPE:text, DELAY:ms, HOTKEY:GUI+r, HOTKEY:CTRL+SHIFT+J";

  if (thinkDeeper) systemPrompt += "\nThink step-by-step carefully.";

  JsonObject sys = messages.createNestedObject();
  sys["role"] = "system";
  sys["content"] = systemPrompt;

  JsonObject usr = messages.createNestedObject();
  usr["role"] = "user";
  usr["content"] = userPrompt;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  String result = "";
  if (code == 200) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument resp;
#else
    DynamicJsonDocument resp(8192);
#endif
    if (!deserializeJson(resp, http.getString())) {
      result = resp["choices"][0]["message"]["content"].as<String>();
    }
  } else {
    result = "Error " + String(code);
  }
  http.end();
  return result;
}

void parseAndQueueActions(const String& response) {
  actionCount = 0;
  int start = response.indexOf("<ACTION>");
  int end = response.indexOf("</ACTION>");
  if (start < 0 || end < 0) return;

  String block = response.substring(start + 8, end);
  block.trim();

  int pos = 0;
  while (pos < block.length() && actionCount < MAX_ACTIONS) {
    int nl = block.indexOf('\n', pos);
    if (nl < 0) nl = block.length();
    String line = block.substring(pos, nl);
    line.trim();
    if (line.length() > 0) {
      int colon = line.indexOf(':');
      if (colon > 0) {
        pendingActions[actionCount].type = line.substring(0, colon);
        pendingActions[actionCount].value = line.substring(colon + 1);
        pendingActions[actionCount].type.toUpperCase();
        pendingActions[actionCount].value.trim();
        actionCount++;
      }
    }
    pos = nl + 1;
  }

  lastActionCount = actionCount;
  for (int i = 0; i < lastActionCount; i++) {
    lastActions[i] = pendingActions[i];
  }
}

void executeBLEActions() {
  if (!bleKeyboard.isConnected() || actionCount == 0) return;

  for (int i = 0; i < actionCount; i++) {
    if (stopRequested) break;
    ActionStep s = pendingActions[i];

    if (s.type == "KEY") {
      s.value.toUpperCase();
      if (s.value == "ENTER") bleKeyboard.write('\n');
      else if (s.value == "SPACE") bleKeyboard.write(' ');
      else if (s.value == "TAB") bleKeyboard.write('\t');
      else if (s.value == "ESC") bleKeyboard.write(0x1B);
      else if (s.value == "BACKSPACE") bleKeyboard.write(0x08);
      else if (s.value == "UP") bleKeyboard.press(0xDA);
      else if (s.value == "DOWN") bleKeyboard.press(0xD9);
      else if (s.value == "LEFT") bleKeyboard.press(0xD8);
      else if (s.value == "RIGHT") bleKeyboard.press(0xD7);
      else if (s.value.length() == 1) bleKeyboard.write(s.value.charAt(0));
      delay(45);
      bleKeyboard.releaseAll();
    }
    else if (s.type == "TYPE") {
      for (size_t j = 0; j < s.value.length(); j++) {
        if (stopRequested) break;
        char c[2] = {s.value.charAt(j), 0};
        bleKeyboard.print(c);
        delay(14);
      }
    }
    else if (s.type == "DELAY") {
      int ms = s.value.toInt();
      if (ms > 0 && ms < 12000) delay(ms);
    }
    else if (s.type == "HOTKEY") {
      String left = s.value;
      while (left.length()) {
        int p = left.indexOf('+');
        String token = (p >= 0) ? left.substring(0, p) : left;
        left = (p >= 0) ? left.substring(p + 1) : "";
        token.trim(); token.toUpperCase();

        if (token == "CTRL") bleKeyboard.press(KEY_CTRL);
        else if (token == "SHIFT") bleKeyboard.press(KEY_SHIFT);
        else if (token == "ALT") bleKeyboard.press(KEY_ALT);
        else if (token == "GUI" || token == "WIN") bleKeyboard.press(KEY_GUI);
        else if (token == "ENTER") bleKeyboard.press('\n');
        else if (token.length() == 1) {
          char c = token.charAt(0);
          if (c >= 'A' && c <= 'Z') c += 32;
          bleKeyboard.press(c);
        }
      }
      delay(50);
      bleKeyboard.releaseAll();
      delay(55);
    }
  }

  bleKeyboard.releaseAll();
  actionCount = 0;
  stopRequested = false;
}
