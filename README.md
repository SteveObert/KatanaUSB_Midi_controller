# A Teensy based USB MIDI foot controller/translator
# for Boss Katana.

![alt text](https://raw.githubusercontent.com/SteveObert/KatanaUSB_Midi_controller/master/images/IMG_2516.JPG)

This is working prototype of a MIDI foot controller for a BOSS Katana using a Teensy 3.2. The foot controller sends sysex messages to the USB port on the Katana. The code emulates a GA-FC without additional footswitch connections (although there doesn’t seem to be any reason that’s not possible). I'm working on an expression pedal input. An expression pedal attached to an external MIDI controller can be used but requires you to adjust/write some code.

This controller can also listen to a 5 pin MIDI IN port and can be programed to translate MIDI messages (PC, CC, etc.) to sysex and forward those to a Katana. However, you must know the Katana sysex. Sending MIDI OUT from the footswitch is also possible.

It seems satable; however, it has only been tested by two people that I am aware of.

**Consider this unsupported beta software. ** If you get stuck, I may not be able to help you.

There are two versions:

In the Katana_USB_MIDI version, LCD and LEDs are updated from the Katana's USB MIDI messages. Meaning, they should take of themselves. Gumtown from the Vguitar forum deserves recognition for rewriting most of the code and adding a great deal of functionality. There is a forum thread reguarding updates and functions: https://www.vguitarforums.com/smf/index.php?topic=25185.0 

Included in the : Katana_USB_MIDI version


* Tap tempo: global or patch.

* Selectable mode for 3 foot switches, toggle either one or both FX/MOD, Delay 1/FX, and Reverb/Delay 2.

* Midi IN - control Katana patch and FX from an external MIDI controller.  *Some* CC and PC messages from the MIDI in port can be received and translated to sysex and forwarded to the Katana. If you’re looking for a more complete external MIDI to Katana you could buy the MIDX-20.

* LCD displays FX status, amp name, tempo, loop status, bank/fx mode, and channel number.

* Select Green, Yellow, or Red effect functions from the three lower foot switches.

* An expression pedal can be connected to an external MIDI controller to control 1 effect only. This is experimental and requires some coding on your part.

* Expression pedal jack input directly connected to controller and external MIDI clock IN is being worked on but not yet complete, *no ETA*.

* There is also a BOM (parts list) in this thread. this version has been tested on a Teensy 3.2 and a Teensy LC.


In the Simple sketch version, you will need to code the LCD and LEDs to work for your needs. The LCD and LEDs are programed to be off when you switch channels, meaning the controller expects all effects to be off. So, you'll need to change the programming if you use it differently. On this version there is no tap-tempo and no expression pedal input. The simple version should run on an Ardiuno Nano or Uno, etc. because that's what I started writing the code for. However, I have not tested it recently.


** Note I had to compile the sketches with the "fast + LTO" setting or debug, otherwise the USB mini host shield would not work. You must select the Teensy option "MIDI" when powered externally (not connected to a computer). Also, you should use the modified versions of the MS3 library included above (top of page).

Along with the Teensy 3.2, I use a Mini USB Host shield like this one: https://www.circuitsathome.com/usb-host-shield-hardware-manual/ along with the host shield library: https://github.com/felis/USB_Host_Shield_2.0.

Roland/Boss guitar gear uses a non-class compliant USB device for midi control. Additionally, there is a checksum included in each sysex message. I found this library made for the BOSS MS3:  https://github.com/MrHaroldA/MS3. Note, this library is included in the files above (top of page) and has been modified to work with the Katana instead of the MS3.
   
Here is some excellent information about sysex messages and Katana amps: https://github.com/snhirsch/katana-midi-bridge/blob/master/doc/katana_sysex.txt

The working prototype uses the library https://github.com/felis/USB_Host_Shield_2.0 and a USB host shield
(https://www.amazon.com/gp/product/B0777DR3T6/ref=ppx_yo_dt_b_asin_title_o03__o00_s00?ie=UTF8&psc=1)

I used a MIDI break out board but there no reason not to build your own MIDI in/out circuit. https://www.amazon.com/ubld-it-MIDI-Breakout-Board/dp/B00YDLVLVO/ref=sr_1_fkmr0_2?ie=UTF8&qid=1549302443&sr=8-2-fkmr0&keywords=midi+host+shield. If you don't need 5 pin MIDI IN/OUT, you can just skip this part.

What got me started on this project is a forum thread created by the author of the MS3 library: 
https://www.vguitarforums.com/smf/index.php?topic=21864.0


![alt text](https://raw.githubusercontent.com/SteveObert/KatanaUSB_Midi_controller/master/wiring_and_Fritzing/wiring.png)
Wiring shown above does not include expression pedals or MIDI through, see images directory for that diagram.


* I don't know if this is necessary but, I originally tested this project with an Arduino Nano and I "cut the trace inside VBUS jumper" on the host shield as mentioned in the second paragraph under the Power Options part of this page: https://www.circuitsathome.com/usb-host-shield-hardware-manual/ shown here: https://www.pjrc.com/teensy/td_libs_USBHostShield.html. I googled these pictures that show the modification for 5v microcntrollers: https://geekhack.org/index.php?PHPSESSID=jnim6u2dcno62u8vpbm8sl9ias67hb3o&action=dlattach;topic=80421.0;attach=130856;image
and https://geekhack.org/index.php?PHPSESSID=jnim6u2dcno62u8vpbm8sl9ias67hb3o&action=dlattach;topic=80421.0;attach=130858;image

These sketches have only been tested with a Teensy 3.2 and a Teensy LC. 


Inside view with messy wiring and extra holes from a previous project:

![alt text](https://raw.githubusercontent.com/SteveObert/KatanaUSB_Midi_controller/master/images/IMG_2526.JPG)
