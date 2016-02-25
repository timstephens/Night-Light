Introduction
=============

This is code designed to create a fading alarm clock using an Arduino and RTC module. 
Lighting is provided with an RGB LED strip (like the Adafruit Neopixel)
Two buttons are required to adjust the brightness of the lighting (which is a global setting), and the alarm will reset once the wakeup sequence has finished, although the lighting will need to be faded down manually. 

The user interface is intentionally controlled over a serial connection to a computer because this was originally designed for a child's bedroom and it was desireable to make it difficult to set. This is also why there is no display of the time, but this would be "trivial" to add. 

Requirements
============

Arduino 
i2c RTC module
Neopixel LED strip
Power supply
Two momentary pushbutton switches or equivalent
RTCx library (from Arduino Library manager)
Adafruit Neopixel library 

How to use
==========

Connect the components to the relevant pins
Build the code

Setting the clock, alarm and brightnesses
=========================================

Connect to the Arduino with a Serial terminal (like the Serial Monitor window)
- If you see nothing, press a key to send a character over the serial comms (ideally don't choose a number between 1--9 since these invoke commands). 
- The menu will be printed, and show what commands are available.
LEDs are zero-referenced (i.e. the first one is number 0), and the colours are 24bit (i.e. 0-255 for each channel).
No numbers in the interface are validated or bounds checked, so things will work funny if you exceed sensible ranges.
Time is set to the clock instantly, but LED settings must be manually written to EEPROM (since the EEPROM has limited read/writes in an ATxxx microprocessor).

License
=======

This is released under GPL 3.0. See License.txt, which accompanies this document for a full copy of the license terms. 

