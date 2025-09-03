# ESP32TFTOscilloscope
ESP32 Oscilloscope for 320x240 TFT LCD and wireless WEB display

<img src="ESP32WEBLCD.png">

Warning!!!
Use old esp32 by Espressif Systems version 2.0.17. 
New version 3.0 and later does NOT support backward compatibility.

This displays an oscilloscope screen both on a 320x240 TFT LCD and also on the WEB page simultaneusly.
The settings are controled on the touch screen of the TFT LCD and also on the WEB page.
You can view the oscilloscope screen on the WEB browser of the PC or the tablet or the smartphone.

Specifications:
<li>Dual input channel</li>
<li>Input voltage range 0 to 3.3V</li>
<li>12 bit ADC 250 ksps single channel, 10 ksps dual channel</li>
<li>Measures minimum, maximum and average values</li>
<li>Measures frequency and duty cycle</li>
<li>Spectrum FFT analysis</li>
<li>Sampling rate selection</li>
<li>Built in Pulse Generator</li>
<li>Built in DDS Function Generator</li>
<br>
<p>
Develop environment is:<br>
Arduino IDE 1.8.19<br>
esp32 by Espressif Systems version 2.0.11<br>
CPU speed 240 MHz<br>
</p>

Libraries:<br>
TFT_eSPI 2.5.0<br>
arduinoFFT by Enrique Condes 2.0.0<br>
arduinoWebSockets from https://github.com/Links2004/arduinoWebSockets<br>

You need to customize the TFT_espi library by referring to the TFT_espi folder here.

10usec/div range is 10 times magnification at 250ksps.<br>
20usec/div range is 5 times magnification at 250ksps.<br>
The magnification applies sin(x)/x interpolation.

For WEB operations, edit the source code WebTask.ino and replace your Access Point and the password.
<pre>
Edit:
const char* ssid = "XXXX";
const char* pass = "YYYY";
To:
const char* ssid = "Your Access Point";
const char* pass = "Your Password";
</pre>

For WEB only display, in case no LCD display is connected, un-comment
<pre>
//#define NOLCD
</pre>
in the file GOscillo.ino.

Schematics:<br>
<img src="ESP32TFTGOscillo.png">
