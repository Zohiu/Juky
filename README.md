# Juky

<img align="right" src="preview.gif" width="30%" style="margin-left: 1rem">
<img align="left" src="settings.gif" width="30%" style="margin-right: 1rem">



An audio visualizer I started in 2021 for a school project that never
got finished due to Covid. Since I promised my teacher that I'll eventually
finish it, I basically restarted it from scratch.

It uses an Arduino UNO, a normal strip of addressable LEDs 
for the display, an MSGEQ7 chip for audio analysis, an RGB backlit 
LCD screen and a rotary encoder for menu navigation.

There is a built-in menu that allows one to change most aspects about
the visualization. The settings do however get reset when power is lost.
Since the Arduino UNO doesn't support any kind of multithreading
and I didn't want menu navigation to pause the visualization (sending
text to the display takes really long), I added a text buffer that
gets sent over time. It may take a moment before changes appear.

<hr>
