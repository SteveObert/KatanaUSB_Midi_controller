# A USB MIDI foot controller/translator
# for Boss Katana amplifiers.

![alt text](https://raw.githubusercontent.com/SteveObert/KatanaUSB_Midi_controller/master/images/IMG_2515.JPG)

This is working prototype of a MIDI foot controller for a BOSS Katana using a Teensy 3.2. The foot controller sends sysex messages to the USB port on the Katana. The code emulates a GA-FC without a Tap Tempo switch and without additional footswitch connections (although there doesn’t seem to be any reason that’s not possible). The controller can also listen to a 5 pin MIDI IN port and can be programed to translate MIDI messages (PC, CC, etc.) to sysex and forward those to a Katana. However, you must know the Katana sysex. Sending MIDI OUT from the footswitch is also possible.

It seems reliable to me; however, it's not been tested by anyone but me. I'm not an experienced programmer so there's lots of room for optimization. 

There are two versions:

In the Simple sketch version, you will need to code the LCD and LEDs to work for your needs. Right now, they are setup for the way I use the Katana - when I switch amp channels I have all effects set to off. So, you'll need to change the programming if you use it differently.

In the Katana_USB_MIDI_auto version LCD and LEDs are updated from the Katana's USB MIDI messages. Meaning, they should take of themselves. Gumtown from the Vguitar forum deserves recognition for witing the LCD and LED updates working.

**Consider this unsupported beta software.** If you get stuck I may not be able to help you.

* Note I had to compile the sketch with the "fast + LTO" setting (debug also works), otherwise the USB mini host shield wouldn't work. You must select the Teensy option "MIDI" when powered externally (not connected to a computer). Also, you should use the modified versions of the MS3 and LiquidCrystal_I2C libraries included above (top of page).

Along with the Teensy 3.2, I use a Mini USB Host shield like this one: https://www.circuitsathome.com/usb-host-shield-hardware-manual/ along with the host shield library: https://github.com/felis/USB_Host_Shield_2.0.

Roland/Boss guitar gear uses a non-class compliant USB device for midi control. Additionally, there is a checksum included in each sysex message. I found this library made for the BOSS MS3:  https://github.com/MrHaroldA/MS3. Note, this library is included in the files above (top of page) and has been modified to work with the Katana instead of the MS3.
   
Here is some excellent information about sysex messages and Katana amps: https://github.com/snhirsch/katana-midi-bridge/blob/master/doc/katana_sysex.txt


The working prototype uses the library https://github.com/felis/USB_Host_Shield_2.0 and a USB host shield
(https://www.amazon.com/gp/product/B0777DR3T6/ref=ppx_yo_dt_b_asin_title_o03__o00_s00?ie=UTF8&psc=1)

I used a MIDI break out board but there no reason not to build your own MIDI in/out circuit. https://www.amazon.com/ubld-it-MIDI-Breakout-Board/dp/B00YDLVLVO/ref=sr_1_fkmr0_2?ie=UTF8&qid=1549302443&sr=8-2-fkmr0&keywords=midi+host+shield

What got me started on this project is a forum thread created by the author of the MS3 library: 
https://www.vguitarforums.com/smf/index.php?topic=21864.0


![alt text](https://raw.githubusercontent.com/SteveObert/KatanaUSB_Midi_controller/master/images/wiring.png)

* Also, I originally tested this project with an Arduino Nano and I "cut the trace inside VBUS jumper" on the host shield as mentioned in the second paragraph under the Power Options part of this page: https://www.circuitsathome.com/usb-host-shield-hardware-manual/ shown here: https://www.pjrc.com/teensy/td_libs_USBHostShield.html. I googled these pictures that show the modification for 5v microcntrollers: https://geekhack.org/index.php?PHPSESSID=jnim6u2dcno62u8vpbm8sl9ias67hb3o&action=dlattach;topic=80421.0;attach=130856;image
and https://geekhack.org/index.php?PHPSESSID=jnim6u2dcno62u8vpbm8sl9ias67hb3o&action=dlattach;topic=80421.0;attach=130858;image

I haven't tested the current sketch with anything other than a Teensy 3.2.


Inside view with messy wiring and extra holes from a previous project:
![alt text](https://raw.githubusercontent.com/SteveObert/KatanaUSB_Midi_controller/master/images/IMG_2526.JPG)

