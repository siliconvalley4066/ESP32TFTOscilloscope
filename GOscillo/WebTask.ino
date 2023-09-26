#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h> // arduinoWebSockets library
#include <ESPmDNS.h>

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// WiFi information
//#define WIFI_ACCESS_POINT
#ifndef WIFI_ACCESS_POINT
const char* ssid = "XXXX";
const char* pass = "YYYY";
#else
const char* apssid = "ESP32OSCILLO";
const char* password = "12345678";
const IPAddress ip(192, 168, 4, 1);
const IPAddress subnet(255, 255, 255, 0);
#endif

// Initial Screen
void handleRoot(void) {
//  xTaskCreatePinnedToCore(index_html, "IndexProcess", 4096, NULL, 1, NULL, PRO_CPU_NUM); //Core 0でタスク開始

  if (server.method() == HTTP_POST) {
    Serial.println(server.argName(0));
    handle_rate();
    handle_range1();
    handle_range2();
    handle_trigger_mode();
    handle_trig_ch();
    handle_trig_edge();
    handle_trig_level();
    handle_run_hold();
    handle_ch1_mode();
    handle_ch_offset1();
    handle_ch2_mode();
    handle_ch_offset2();
    handle_wave_fft();
    handle_pwm_onoff();
    handle_dds_onoff();
    handle_wave_select();
    handle_dds_freq();
    handle_pwm_duty();
    handle_pwm_freq();
    saveTimer = 5000;     // set EEPROM save timer to 5 secnd
    return;
  }
  index_html(NULL);
}

void handle_ch1_mode() {
  String val = server.arg("ch1_mode");
  if (val != NULL) {
    Serial.println(val);
    if (val == "chon") {
      ch0_mode = MODE_ON;       // CH1 ON
    } else if (val == "chinv") {
      ch0_mode = MODE_INV;      // CH1 INV
    } else if (val == "choff") {
      ch0_mode = MODE_OFF;      // CH1 OFF
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_ch2_mode() {
  String val = server.arg("ch2_mode");
  if (val != NULL) {
    Serial.println(val);
    if (val == "chon") {
      ch1_mode = MODE_ON;       // CH2 ON
    } else if (val == "chinv") {
      ch1_mode = MODE_INV;      // CH2 INV
    } else if (val == "choff") {
      ch1_mode = MODE_OFF;      // CH2 OFF
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_rate() {
  String val = server.arg("rate");
  if (val != NULL) {
    int nrate = rate;
    Serial.println(val);
    if (val == "1") {
      wrate = 3;    // fast
      if (rate > RATE_MIN) nrate = rate - 1;
    } else if (val == "0") {
      wrate = 7;    // slow
      if (rate < RATE_MAX) nrate = rate + 1;
    }
    String strrate;
    strrate = Rates[nrate];
    server.send(200, "text/html", strrate+((nrate > RATE_DMA)?"":" DMA"));  // response 200, send OK
  }
}

void handle_range1() {
  String val = server.arg("range1");
  if (val != NULL) {
    Serial.println(val);
    if (val == "1") {
      updown_ch0range(3);  // range1 up
    } else if (val == "0") {
      updown_ch0range(7);  // range1 down
    }
    String ch1acdc;
    if (digitalRead(CH0DCSW) == LOW)    // DC/AC input
      ch1acdc = "AC ";
    else
      ch1acdc = "DC ";
    server.send(200, "text/html", ch1acdc + Ranges[range0]);  // response 200, send OK
  }
}

void handle_range2() {
  String val = server.arg("range2");
  if (val != NULL) {
    Serial.println(val);
    if (val == "1") {
      updown_ch1range(3);  // range2 up
    } else if (val == "0") {
      updown_ch1range(7);  // range2 down
    }
    String ch2acdc;
    if (digitalRead(CH1DCSW) == LOW)    // DC/AC input
      ch2acdc = "AC ";
    else
      ch2acdc = "DC ";
    server.send(200, "text/html", ch2acdc + Ranges[range1]);  // response 200, send OK
  }
}

void handle_trigger_mode() {
  String val = server.arg("trigger_mode");
  if (val != NULL) {
    Serial.println(val);
    if (val == "0") {
      trig_mode = 0;  // Auto
    } else if (val == "1") {
      trig_mode = 1;  // Normal
    } else if (val == "2") {
      trig_mode = 2;  // Scan
    } else if (val == "3") {
      trig_mode = 3;  // Once
    }
    if (trig_mode != TRIG_ONE)
      Start = true;
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_trig_ch() {
  String val = server.arg("trig_ch");
  if (val != NULL) {
    Serial.println(val);
    if (val == "ch1") {
      trig_ch = ad_ch0;
    } else if (val == "ch2") {
      trig_ch = ad_ch1;
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_trig_edge() {
  String val = server.arg("trig_edge");
  if (val != NULL) {
    Serial.println(val);
    if (val == "down") {
      trig_edge = TRIG_E_DN;  // trigger fall
    } else if (val == "up") {
      trig_edge = TRIG_E_UP;  // trigger rise
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_trig_level() {
  String val = server.arg("trig_lvl");
  if (val != NULL) {
    Serial.println(val);
    trig_lv = val.toInt();
    set_trigger_ad();
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_run_hold() {
  String val = server.arg("run_hold");
  if (val == "run") {
    Serial.println(val);
    Start = true;
    server.send(200, "text/html", "OK");  // response 200, send OK
  } else if (val == "hold") {
    Serial.println(val);
    Start = false;
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_ch_offset1() {
  if (server.hasArg("reset1")) {
    Serial.println("reset1");
    if (server.arg("reset1").equals("1")) {
      if (digitalRead(CH0DCSW) == LOW)    // DC/AC input
        ch0_off = ac_offset[range0];
      else
        ch0_off = 0;
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  } else if (server.hasArg("offset1")) {
    String val = server.arg("offset1");
    if (val != NULL) {
//      Serial.println(val);
      long offset = val.toInt();
      ch0_off = (4096 * offset)/VREF[range0];
      if (digitalRead(CH0DCSW) == LOW)    // DC/AC input
        ch0_off += ac_offset[range0];
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_ch_offset2() {
  if (server.hasArg("reset2")) {
    Serial.println("reset2");
    if (server.arg("reset2").equals("2")) {
      if (digitalRead(CH1DCSW) == LOW)    // DC/AC input
        ch1_off = ac_offset[range1];
      else
        ch1_off = 0;
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  } else if (server.hasArg("offset2")) {
    String val = server.arg("offset2");
    if (val != NULL) {
//      Serial.println(val);
      long offset = val.toInt();
      ch1_off = (4096 * offset)/VREF[range1];
      if (digitalRead(CH1DCSW) == LOW)    // DC/AC input
        ch1_off += ac_offset[range1];
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_wave_fft() {
  String val = server.arg("wavefft");
  if (val != NULL) {
    Serial.println(val);
    if (val == "wave") {
      wfft = false;
    } else if (val == "fft") {
      wfft = true;
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_pwm_onoff() {
  String val = server.arg("pwm_on");
  if (val != NULL) {
    Serial.println(val);
    if (val == "on") {
      update_frq(0);
      pulse_start();
      pulse_mode = true;
    } else if (val == "off") {
      pulse_close();
      pulse_mode = false;
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_dds_onoff() {
  String val = server.arg("dds_on");
  if (val != NULL) {
    Serial.println(val);
    if (val == "on") {
      wdds = true;
    } else if (val == "off") {
      wdds = false;
    }
    server.send(200, "text/html", "OK");  // response 200, send OK
  }
}

void handle_wave_select() {
  String val = server.arg("wave_select");
  if (val != NULL) {
    Serial.println(val);
    server.send(200, "text/html", "OK");  // response 200, send OK
    set_wave(val.toInt());
  }
}

void handle_dds_freq() {
  String val = server.arg("dfreq");
  if (val != NULL) {
    Serial.println(val);
    server.send(200, "text/html", String(set_freq((float)val.toFloat()), 2)); // response 200, send OK
  }
}

void handle_pwm_duty() {
  String val = server.arg("duty");
  if (val != NULL) {
    Serial.println(val);
    duty = constrain(round(val.toFloat() * 2.56), 0, 255);
    setduty();
    server.send(200, "text/html", String(duty*100.0/256.0, 1)); // response 200, send OK
  }
}

void handle_pwm_freq() {
  String val = server.arg("wfreq");
  if (val != NULL) {
    Serial.println(val);
    set_pulse_frq(val.toFloat());
    server.send(200, "text/html", String(pulse_frq())); // response 200, send OK
  }
}

void index_html(void * pvParameters) {
String html;

//<meta http-equiv="refresh" content="1; URL=">
  html = R"=====(
<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>ESP32 Web Oscilloscope</title>
<style>body { background: #fafafa; }
canvas { display:block; background:#ffe; margin:0 auto; }</style>
<script type="text/javascript">
var ws = new WebSocket('ws://' + window.location.hostname + ':81/');
ws.binaryType = 'arraybuffer';
ws.onmessage = function(evt) {
  var wsRecvMsg = evt.data;
  var datas = new Int16Array(wsRecvMsg);
  var cv1 = document.getElementById('cvs1');
  var ctx = cv1.getContext('2d');
  if (!cv1||!cv1.getContext) { return false; }
  var groundW= cv1.getAttribute('width');
  var groundH= cv1.getAttribute('height');
  ctx.fillStyle = "rgb(0,0,0)";
  ctx.fillRect(0,0,groundW,groundH);
  var groundX0= 0; var groundY0= groundH;
  var pichX = 50;
  var cnstH= groundH/4096;
  var pichH = groundH/8;
  const fftsamples = 128;
  const displng = 300;
  ctx.beginPath();
  ctx.save();
  ctx.strokeStyle = "rgb(128,128,128)";
  ctx.lineWidth = 1;
  for (var i = 0; i < groundW/pichX; i++){
    ctx.moveTo(groundX0+i*pichX, groundY0);
    ctx.lineTo(groundX0+i*pichX, 0);
    for (var j=10; j<50; j=j+10){
      ctx.moveTo(groundX0+i*pichX+j, 4*pichX-5);
      ctx.lineTo(groundX0+i*pichX+j, 4*pichX+5);
    }
  }
  ctx.moveTo(groundX0+groundW-1, groundY0);
  ctx.lineTo(groundX0+groundW-1, 0);
  for (var i = 0; i < groundH/pichH; i++){
    ctx.moveTo(groundX0, groundY0-i*pichH);
    ctx.lineTo(groundW, groundY0-i*pichH);
    for (var j=10; j<50; j=j+10){
      ctx.moveTo(6*pichX-5, groundY0-i*pichH-j);
      ctx.lineTo(6*pichX+5, groundY0-i*pichH-j);
    }
  }
  ctx.moveTo(groundX0, groundY0-i*pichH);
  ctx.lineTo(groundW, groundY0-i*pichH);
  ctx.stroke();
  ctx.restore();
  ctx.save();
  ctx.beginPath();
  ctx.strokeStyle = "rgb(0,255,0)";
  if (datas.length == (fftsamples + 2)) {
    var base = groundY0-cnstH*512;
    for (var i = 1; i < fftsamples; i++){
      ctx.moveTo(groundX0+4*i, base);
      ctx.lineTo(groundX0+4*i, base-cnstH*datas[i]);
    }
    ctx.stroke();
    ctx.strokeStyle = "rgb(192,192,192)";
    ctx.moveTo(0, base); ctx.lineTo(0, base+8);
    ctx.moveTo(8*32, base); ctx.lineTo(8*32, base+8);
    ctx.moveTo(8*64, base); ctx.lineTo(8*64, base+8);
    ctx.textAlign = "left";
    ctx.textBaseline = "bottom";
    ctx.font = "12pt Arial";
    ctx.fillStyle = "rgb(192,192,192)";
    for (var j = 10; j < 80; j += 10){
      ctx.fillText(String(-j)+"dB", 555, 5 * j);
    }
    var nyquist = 1000 * datas[fftsamples] + datas[fftsamples+1];
    ctx.textBaseline = "top";
    ctx.fillText("0Hz", 0, base + 10);
    ctx.textAlign = "center";
    if ((nyquist/2) < 1000)
      ctx.fillText(String(nyquist/2)+"Hz", 8*32, base + 10);
    else
      ctx.fillText(String(nyquist/2000)+"kHz", 8*32, base + 10);
    if (nyquist < 1000)
      ctx.fillText(String(nyquist)+"Hz", 8*64, base + 10);
    else
      ctx.fillText(String(nyquist/1000)+"kHz", 8*64, base + 10);
  }
  if (datas.length >= displng && datas[0] >= 0) {
    ctx.moveTo(groundX0, groundY0-cnstH*datas[0]);
    for (var i = 1; i < displng; i++){
      ctx.lineTo(groundX0+2*i, groundY0-cnstH*datas[i]);
    }
  }
  ctx.stroke();
  if (datas.length == (displng+displng) && datas[displng] >= 0) {
    ctx.beginPath();
    ctx.strokeStyle = "rgb(255,255,0)";
    ctx.moveTo(groundX0, groundY0-cnstH*datas[displng]);
    for (var i = 1; i < displng; i++){
      ctx.lineTo(groundX0+2*i, groundY0-cnstH*datas[i+displng]);
    }
    ctx.stroke();
  }
  ctx.restore();
};
</script>
</head>
<script type="text/javascript">
window.addEventListener("load", function () {
  const value_trig_ch = '%TRIGCH%'
  const value_trig_edge = '%TRIGEDGE%'
  const value_ch1_mode = '%CH1MODE%'
  const value_ch2_mode = '%CH2MODE%'
  const value_fft_mode = '%WAVEFFT%'
  const value_run_hold = '%RUNHOLD%'
  const value_pulse_onoff = '%PULSEONOFF%'
  const value_dds_onoff = '%DDSONOFF%'
  var elements = document.getElementsByName("trig_ch");
  var len = elements.length;
  for (let i = 0; i < len; i++){
    console.log(elements.item(i).value);
    if (elements.item(i).value == value_trig_ch){
      elements.item(i).checked = true;
    }
  }
  elements = document.getElementsByName("trig_edge");
  len = elements.length;
  for (let i = 0; i < len; i++){
    console.log(elements.item(i).value);
    if (elements.item(i).value == value_trig_edge){
      elements.item(i).checked = true;
    }
  }
  elements = document.getElementsByName("ch1_mode");
  len = elements.length;
  for (let i = 0; i < len; i++){
    console.log(elements.item(i).value);
    if (elements.item(i).value == value_ch1_mode){
      elements.item(i).checked = true;
    }
  }
  elements = document.getElementsByName("ch2_mode");
  len = elements.length;
  for (let i = 0; i < len; i++){
    console.log(elements.item(i).value);
    if (elements.item(i).value == value_ch2_mode){
      elements.item(i).checked = true;
    }
  }
  elements = document.getElementsByName("wavefft");
  len = elements.length;
  for (let i = 0; i < len; i++){
    console.log(elements.item(i).value);
    if (elements.item(i).value == value_fft_mode){
      elements.item(i).checked = true;
    }
  }
  elements = document.getElementsByName("run_hold");
  len = elements.length;
  for (let i = 0; i < len; i++){
    console.log(elements.item(i).value);
    if (elements.item(i).value == value_run_hold){
      elements.item(i).checked = true;
    }
  }
  elements = document.getElementsByName("pwm_on");
  len = elements.length;
  for (let i = 0; i < len; i++){
    console.log(elements.item(i).value);
    if (elements.item(i).value == value_pulse_onoff){
      elements.item(i).checked = true;
    }
  }
  elements = document.getElementsByName("dds_on");
  len = elements.length;
  for (let i = 0; i < len; i++){
    console.log(elements.item(i).value);
    if (elements.item(i).value == value_dds_onoff){
      elements.item(i).checked = true;
    }
  }
});
async function postform(frameid) {
  try {
    const form = new FormData(document.getElementById(frameid));
    const response = await fetch("/", {
      method: "POST",
      body: form
    });
    if (response.ok) {
      const text = await response.text();
      console.log(text);
    }
  } catch (error) { console.log(error); }
}
async function postrate(i) {
  const output = document.getElementById("rate_area");
  try {
    var form = new FormData()
    form.append('rate', i)
    const response = await fetch("/", {
      method: "POST",
      body: form
    });
    if (response.ok) {
      const text = await response.text();
      output.textContent = text;
      console.log(text);
    }
  } catch (error) { console.log(error); }
}
async function postrange1(i) {
  const output = document.getElementById("range1_area");
  try {
    var form = new FormData()
    form.append('range1', i)
    const response = await fetch("/", {
      method: "POST",
      body: form
    });
    if (response.ok) {
      const text = await response.text();
      output.textContent = text;
      console.log(text);
    }
  } catch (error) { console.log(error); }
}
async function postrange2(i) {
  const output = document.getElementById("range2_area");
  try {
    var form = new FormData()
    form.append('range2', i)
    const response = await fetch("/", {
      method: "POST",
      body: form
    });
    if (response.ok) {
      const text = await response.text();
      output.textContent = text;
      console.log(text);
    }
  } catch (error) { console.log(error); }
}
async function postreset(button_id) {
  try {
    const btn = document.getElementById(button_id);
    var form = new FormData()
    form.append(btn.name, btn.value);
    const response = await fetch("/", {
      method: "POST",
      body: form
    });
    if (response.ok) {
      const text = await response.text();
      console.log(text);
      if (btn.value == '1') {
        document.getElementById("ofsval").value = '0';
      } else {
        document.getElementById("ofsva2").value = '0';
      }
    }
  } catch (error) { console.log(error); }
}
async function post_dfreq() {
  const output = document.getElementById("pfreq");
  try {
    var form = new FormData()
    form.append('dfreq', output.value)
    const response = await fetch("/", {method: "POST", body: form});
    if (response.ok) {
      const text = await response.text();
      output.value = text;
      console.log(text);
    }
  } catch (error) { console.log(error); }
}
async function post_wfreq() {
  const output = document.getElementById("mfreq");
  try {
    var form = new FormData()
    form.append('wfreq', output.value)
    const response = await fetch("/", {method: "POST", body: form});
    if (response.ok) {
      const text = await response.text();
      output.value = text;
      console.log(text);
    }
  } catch (error) { console.log(error); }
}
async function post_duty() {
  const output = document.getElementById("pduty");
  try {
    var form = new FormData()
    form.append('duty', output.value)
    const response = await fetch("/", {method: "POST", body: form});
    if (response.ok) {
      const text = await response.text();
      output.value = text;
      console.log(text);
    }
  } catch (error) { console.log(error); }
}
</script>
<body>
<h3>ESP32 Web Oscilloscope ver. 1.31</h3>
<div style='float: left; margin-right: 10px'>
<canvas id='cvs1' width='601' height='401' class='float'></canvas></div>
<form id='rate0'>Rate: <label id="rate_area">%RATE% %REALDMA%</label>
  <button type="button" name='rate' value='1' onclick='postrate(this.value)'>FAST</button>
  <button type="button" name='rate' value='0' onclick='postrate(this.value)'>SLOW</button></form>
<form id='range01'>Range1: <label id="range1_area">%RANGE1% </label>
  <button type="button" name='range1' value='1' onclick='postrange1(this.value)'>UP</button>
  <button type="button" name='range1' value='0' onclick='postrange1(this.value)'>DOWN</button></form>
<form id='range02'>Range2: <label id="range2_area">%RANGE2% </label>
  <button type="button" name='range2' value='1' onclick='postrange2(this.value)'>UP</button>
  <button type="button" name='range2' value='0' onclick='postrange2(this.value)'>DOWN</button></form>
<div class='input-field col s4'>
<form id='trig_mode' onchange='postform(this.id)'><label>Trigger Mode</label>
  <select name='trigger_mode'>
    <option value='0'>Auto</option>
    <option value='1'>Normal</option>
    <option value='2'>Scan</option>
    <option value='3'>Once</option>
  </select></form></div>
<div>
<form id='trigsrc' onchange='postform(this.id)'>
<label>Trigger Source</label>
<label><input type="radio" name="trig_ch" value="ch1">CH1</label>
<label><input type="radio" name="trig_ch" value="ch2">CH2</label>
</form></div>
<div>
<form id='trigedge' onchange='postform(this.id)'>
<label>Trigger Edge</label>
<label><input type="radio" name="trig_edge" value="up">UP</label>
<label><input type="radio" name="trig_edge" value="down">DOWN</label>
</form></div>
<form id='trigger_lvl' oninput='postform(this.id)'><label>Trigger Level</label>
<input type="range" name="trig_lvl" min="0" max="199" step="1" value="%TRIGGLEVEL%"></form>
<div>
<form id='f_run_hold' onchange='postform(this.id)'>
<label><input type="radio" name="run_hold" value="run">RUN</label>
<label><input type="radio" name="run_hold" value="hold">HOLD</label>
</form>
</div>
<hr>
<div>
<form id='waveform1' onchange='postform(this.id)'>
<label>CH1</label>
<label><input type="radio" name="ch1_mode" value="chon">ON</label>
<label><input type="radio" name="ch1_mode" value="chinv">INV</label>
<label><input type="radio" name="ch1_mode" value="choff">OFF</label>
</form>
<form id='offset_1' oninput='postform(this.id)'><label>offset</label>
<input type="range" id='ofsval' name="offset1" min="-100" max="100" step="1" value="%CH1OFFSET%">
<button type="button" id='reset_button1' name='reset1' value='1' onclick='postreset(this.id)'>reset</button></form>
</div>
<div>
<form id='waveform2' onchange='postform(this.id)'>
<label>CH2</label>
<label><input type="radio" name="ch2_mode" value="chon">ON</label>
<label><input type="radio" name="ch2_mode" value="chinv">INV</label>
<label><input type="radio" name="ch2_mode" value="choff">OFF</label>
</form>
<form id='offset_2' oninput='postform(this.id)'><label>offset</label>
<input type="range" id='ofsva2' name="offset2" min="-100" max="100" step="1" value="%CH2OFFSET%">
<button type="button" id='reset_button2' name='reset2' value='2' onclick='postreset(this.id)'>reset</button></form>
</div>
<hr>
<div>
<form id='fft' onchange='postform(this.id)'>
<label></label>
<label><input type="radio" name="wavefft" value="wave">Wave</label>
<label><input type="radio" name="wavefft" value="fft">FFT</label>
</form></div>

<div>
<form id='f_pwm' onchange='postform(this.id)'>
<label>Pulse</label>
<label><input type="radio" name="pwm_on" value="on">on</label>
<label><input type="radio" name="pwm_on" value="off">off</label>
</form>
</div>

<div>
<label>Pulse Frequency 
<input type="number" id='mfreq' name="wfreq" size="8" value="%PULSEFREQ%" onchange='post_wfreq()' />
Hz</label>
</div>

<div>
<label>Pulse Duty 
<input type="number" id='pduty' name="duty" size="4" value="%PULSEDUTY%" onchange='post_duty()' />
%</label>
</div>

<div>
<form id='f_dds' onchange='postform(this.id)'>
<label>Waveform</label>
<label><input type="radio" name="dds_on" value="on">on</label>
<label><input type="radio" name="dds_on" value="off">off</label>
</form>
</div>

<div>
<form id='waveselect' onchange='postform(this.id)'><label>Wave Select</label>
  <select name='wave_select'>
    <option value='0'>sine256</option>
    <option value='1'>saw256</option>
    <option value='2'>revsaw256</option>
    <option value='3'>triangle</option>
    <option value='4'>rect256</option>
    <option value='5'>pulse20</option>
    <option value='6'>pulse10</option>
    <option value='7'>pulse05</option>
    <option value='8'>delta</option>
    <option value='9'>noise</option>
    <option value='10'>gaussian noise</option>
    <option value='11'>ecg</option>
    <option value='12'>sinc5</option>
    <option value='13'>sinc10</option>
    <option value='14'>sinc20</option>
    <option value='15'>sine2 harmonic</option>
    <option value='16'>sine3 harmonic</option>
    <option value='17'>chopped sine</option>
    <option value='18'>sinabs</option>
    <option value='19'>trapezoid</option>
    <option value='20'>step2</option>
    <option value='21'>step4</option>
    <option value='22'>chainsaw</option>
  </select></form></div>

<div>
<label>Wave Frequency 
<input type="number" id='pfreq' name="dfreq" size="8" value="%DDSFREQ%" onchange='post_dfreq()' />
Hz</label>
</div>

</body>
</html>
)=====";

  String ch1acdc, ch2acdc;
  if (digitalRead(CH0DCSW) == LOW)    // DC/AC input
    ch1acdc = "AC ";
  else
    ch1acdc = "DC ";
  if (digitalRead(CH1DCSW) == LOW)    // DC/AC input
    ch2acdc = "AC ";
  else
    ch2acdc = "DC ";

  html.replace("%RATE%", Rates[rate]);
  html.replace("%REALDMA%", (rate > RATE_DMA)?"":"DMA");
  html.replace("%RANGE1%", ch1acdc + Ranges[range0]);
  html.replace("%RANGE2%", ch2acdc + Ranges[range1]);
  html.replace("%TRIGCH%", trig_ch==ad_ch0?"ch1":"ch2");
  html.replace("%WAVEFORM1%", Modes[ch0_mode]);
  html.replace("%WAVEFORM2%", Modes[ch1_mode]);
  html.replace("%TRIGEDGE%", trig_edge==TRIG_E_DN?"down":"up");
  html.replace("%TRIGGLEVEL%", String(trig_lv));
  html.replace("%WAVEFFT%", fft_mode?"fft":"wave");
  html.replace("%RUNHOLD%", Start?"run":"hold");
  html.replace("%PULSEONOFF%", pulse_mode?"on":"off");
  html.replace("%DDSONOFF%", dds_mode?"on":"off");
  html.replace("%DDSFREQ%", String(dds_freq()));
  if (ch0_mode == MODE_ON) {
    html.replace("%CH1MODE%", "chon");
  } else if (ch0_mode == MODE_INV) {
    html.replace("%CH1MODE%", "chinv");
  } else if (ch0_mode == MODE_OFF) {
    html.replace("%CH1MODE%", "choff");
  }
  if (ch1_mode == MODE_ON) {
    html.replace("%CH2MODE%", "chon");
  } else if (ch1_mode == MODE_INV) {
    html.replace("%CH2MODE%", "chinv");
  } else if (ch1_mode == MODE_OFF) {
    html.replace("%CH2MODE%", "choff");
  }
  html.replace("%CH1OFFSET%", String((ch0_off * VREF[range0]) / 4096));
  html.replace("%CH2OFFSET%", String((ch1_off * VREF[range1]) / 4096));
  html.replace("%PULSEDUTY%", String(duty*100.0/256.0, 1));
  html.replace("%PULSEFREQ%", String(pulse_frq()));

  // send the HTML
  server.send(200, "text/html", html);
//  vTaskDelete(NULL);
}

void handleNotFound(void) {
  server.send(404, "text/plain", "Not Found.");
}

void setup1(void * pvParameters) {
  Serial.begin(115200);
//  Serial.printf("CORE0 = %d\n", xPortGetCoreID());
#ifdef WIFI_ACCESS_POINT
  WiFi.disconnect(true);
  delay(1000);
  WiFi.softAP(apssid, password);
  delay(100);
  WiFi.softAPConfig(ip, ip, subnet);
  IPAddress myIP = WiFi.softAPIP();
#else
// Connect to the WiFi access point
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
#endif
  // print out the IP address of the ESP32
  Serial.print("WiFi Connected. IP = "); Serial.println(WiFi.localIP());
//  Serial.print("WiFi Connected. GW = "); Serial.println(WiFi.gatewayIP());
//  Serial.print("WiFi Connected. DNS = "); Serial.println(WiFi.dnsIP());

  if (MDNS.begin("esp32oscillo")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  webSocket.begin();
  while (true) {
    server.handleClient();
    if (xTaskNotifyWait(0, 0, NULL, pdMS_TO_TICKS(0)) == pdTRUE) {
      if (rate < RATE_ROLL && fft_mode) {
        webSocket.broadcastBIN((byte *) payload, FFT_N + 4);
      } else if (rate >= RATE_DUAL) {
        webSocket.broadcastBIN((byte *) payload, SAMPLES * 4);
      } else {
        webSocket.broadcastBIN((byte *) payload, SAMPLES * 2);
      }
    }
    webSocket.loop();
    delay(1);
  }
}
