# Teensy MIDI controller for Katana 100
## Warning Beta code 

In this version of the sketch the LCD and LEDs are updated from Katana's USB MIDI and some CC and PC messages can be sent from an external MIDI device to the Teensy via the serial MIDI IN port.

Gumtown from the Vguitar forum rewrote tons of the code in this sketch.


Included:

Tap tempo: global or patch ot (experimental MIDI Clock IN).

Selectable mode for footswitches along the bottom either one or both FX/MOD Delay 1/FX, and Reverb/Delay 2.

Selectable Katana color function for red ,green, or yellow for footswitches along the bottom either.

Midi IN - control Katana patch and FX from an external MIDI controller.

LCD displays FX status, amp name, tempo, loop status, bank/fx mode, and channel number.

An expression pedal can be connected to an external MIDI controller to control 1 effect only. this is experimental and requires some coding on your part.

Expression pedal jack input directly connected to controller and external MIDI clock IN is being worked on but not yet complete, no ETA.
