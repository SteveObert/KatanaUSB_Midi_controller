/**
 * This is a simple library to control the Boss MS-3.
 *
 * Check README.md or visit https://github.com/MrHaroldA/MS3 for more information.
 *
 * Debug information:
 * - Define MS3_DEBUG_MODE in your sketch before including this library.
 *
 * Dependencies
 * - An Arduino with a USB Host Shield
 * - https://github.com/felis/USB_Host_Shield_2.0
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MS3_h
#define MS3_h

#include "Arduino.h"

/**
 * Overridable config.
 *
 * MS3_WRITE_INTERVAL_MSEC: The delay before a new message is sent after a write action.
 * MS3_READ_INTERVAL_MSEC: The delay before a new message is sent after a read action.
 * MS3_RECEIVE_INTERVAL_MSEC: The delay after receiving data from the MS-3.
 * MS3_HEADER: The manufacturer and device id header.
 * MS3_QUEUE_SIZE: The maximum number of items in the send queue.
 */
#ifndef MS3_WRITE_INTERVAL_MSEC
const byte MS3_WRITE_INTERVAL_MSEC = 4;
#endif

#ifndef MS3_READ_INTERVAL_MSEC
const byte MS3_READ_INTERVAL_MSEC = 25;
#endif

#ifndef MS3_RECEIVE_INTERVAL_MSEC
const byte MS3_RECEIVE_INTERVAL_MSEC = 4;
#endif

// Changed last from 0x3b to 0x33 for katana
#ifndef MS3_HEADER
const byte MS3_HEADER[6] = {0x41, 0x00, 0x00, 0x00, 0x00, 0x33};
#endif

#ifndef MS3_QUEUE_SIZE
const byte MS3_QUEUE_SIZE = 20;
#endif

/**
 * The configuration options below are internal and should not be changed.
 */
#ifdef MS3_DEBUG_MODE
    #define MS3_DEBUG(x) Serial.print(x)
    #define MS3_DEBUGLN(x) Serial.println(x)
    #define MS3_DEBUG_AS(x, y) Serial.print(x, y)
#else
    #define MS3_DEBUG(x) (void)(x)
    #define MS3_DEBUGLN(x) (void)(x)
    #define MS3_DEBUG_AS(x, y) (void)(x)
#endif

#include "usbh_midi.h"
#include "Queue.h"

// General configuration.
const unsigned int INIT_DELAY_MSEC = 100;
const byte MS3_WRITE = 0x12;
const byte MS3_READ = 0x11;

// Return values.
const int8_t MS3_NOT_READY = 0;
const int8_t MS3_READY = 1;
const int8_t MS3_DATA_SENT = 2;
const int8_t MS3_DATA_RECEIVED = 3;
const int8_t MS3_NOTHING_HAPPENED = 4;
const int8_t MS3_ALMOST_IDLE = 5;
const int8_t MS3_IDLE = 6;

// Fixed data.
const byte SYSEX_START = 0xF0;
const byte SYSEX_END = 0xF7;
const byte HANDSHAKE[15] = {0xF0, 0x7E, 0x00, 0x06, 0x02, 0x41, 0x3B, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF7};
const unsigned long P_EDIT = 0x7F000001;

// Load the Queue class.
Queue Queue;

class MS3 : public USBH_MIDI {
    private:
        USB Usb;
        byte lastState = 0;
        bool ready = false;
        unsigned long nextMessage = 0;

        /**
         * The last bit of the data sent to the MS-3 contains a checksum of the parameter and data.
         */
        byte checksum(byte const *data, byte dataLength) {
            byte sum = 0, i;

            for (i = 8; i < 12 + dataLength; i++) {
                sum = (sum + data[i]) & 0x7F;
            }

            return (128 - sum) & 0x7F;
        }

        /**
         * Construct and send a full SysEx message.
         */
        void send(const unsigned long address, byte *data, byte dataLength, byte action) {
            byte sysex[14 + dataLength] = {0};

            memcpy(sysex + 1, MS3_HEADER, 7);
            sysex[8] = (byte) (address >> 24);
            sysex[9] = (byte) (address >> 16);
            sysex[10] = (byte) (address >> 8);
            sysex[11] = (byte) (address);
            memcpy(sysex + 12, data, dataLength);

            sysex[0] = SYSEX_START;
            sysex[7] = action;
            sysex[12 + dataLength] = checksum(sysex, dataLength);
            sysex[13 + dataLength] = SYSEX_END;

            MS3::send(sysex);
        }

        /**
         * Send the data to the MS-3.
         */
        void send(byte *data) {
            byte result, dataLength = MS3::countSysExDataSize(data);

            MS3_DEBUG(F("TX:"));
            MS3::printSysEx(data, dataLength);
            if ((result = MS3::SendSysEx(data, dataLength)) != 0) {
                MS3_DEBUG(F(" *** Transfer error: "));
                MS3_DEBUG(result);
            }
            MS3_DEBUGLN(F("."));
        }

        /**
         * MS3_DEBUG printer for the received SysEx data.
         */
        void printSysEx(byte *data, byte dataLength) {
            char buf[10];

            for (byte i = 0; i < dataLength; i++) {
                sprintf(buf, " %02X", data[i]);
                MS3_DEBUG(buf);
            }
            MS3_DEBUG(F(" ("));
            MS3_DEBUG(dataLength);
            MS3_DEBUG(F(")"));
        }

        /**
         * Check if we've received any data.
         */
        bool receive(unsigned long &parameter, byte &dataOut) {
            byte
                incoming[MIDI_EVENT_PACKET_SIZE] = {0},
                data[MIDI_EVENT_PACKET_SIZE] = {0},
                dataLength = 0,
                i;

            uint16_t rcvd;

            if (MS3::RecvData(&rcvd, incoming) == 0) {
                if (rcvd == 0) {
                    return false;
                }

                byte *p = incoming;
                for (i = 0; i < MIDI_EVENT_PACKET_SIZE; i += 4) {
                    if (*p == 0 && *(p + 1) == 0) {
                        break;
                    }

                    byte chunk[3] = {0};
                    if (MS3::extractSysExData(p, chunk) != 0) {
                        for (byte part : chunk) {
                            data[dataLength] = part;
                            dataLength++;

                            if (part == SYSEX_END) {
                                break;
                            }
                        }
                        p += 4;
                    }
                }
                MS3_DEBUG(F("RX:"));
                MS3::printSysEx(data, dataLength);
                MS3_DEBUGLN(F("."));

                // Return values.
                parameter = 0;
                for (i = 0; i < 4; i++) {
                    parameter += (unsigned long) data[8 + i] << (3 - i) * 8;
                }
                dataOut = (byte) data[dataLength - 3];

                // If the data is one byte longer, add 128 to the return value for a full byte range.
                if (dataLength == 16 && data[dataLength - 4] == 0x01) {
                    dataOut += 128;
                }

                return true;
            }

            return false;
        }

        /**
         * Check if the USB layer is ready.
         */
        bool isReady() {
            Usb.Task();
            if (Usb.getUsbTaskState() == USB_STATE_RUNNING) {
                return true;
            } else if (MS3::lastState != Usb.getUsbTaskState()) {
                MS3_DEBUG(F("*** USB task state: "));
                MS3_DEBUG_AS(MS3::lastState = Usb.getUsbTaskState(), HEX);
                MS3_DEBUGLN(F("."));
                MS3::ready = false;
            }

            return false;
        }

    public:

        /**
         * Constructor.
         */
        MS3() : USBH_MIDI(&Usb) {}

        /**
         * Set up the USB layer.
         */
        bool begin() {
            if (Usb.Init() == -1) {
                MS3_DEBUG(F("*** USB init error! ***"));
                return false;
            }

            return true;
        }

        /**
         * Init the editor mode.
         */
        void setEditorMode() {
            MS3::send((byte *) HANDSHAKE);
            delay(MS3_WRITE_INTERVAL_MSEC);
            MS3::send((byte *) HANDSHAKE);
            delay(MS3_WRITE_INTERVAL_MSEC);
            byte data[1] = {0x01};
            MS3::send(P_EDIT, data, 1, MS3_WRITE);
            delay(INIT_DELAY_MSEC);
            MS3_DEBUGLN(F("*** Up and ready!"));
        }

        /**
         * This is the main function for both receiving and sending data when
         * there's nothing to receive.
         */
        byte update(unsigned long &parameter, byte &data) {

            // Are we ready?
            if (MS3::isReady()) {
                if (!MS3::ready) {
                    MS3::ready = true;
                    return MS3_READY;
                }
            } else {
                return MS3_NOT_READY;
            }

            // Is there data waiting to be picked up?
            if (MS3::receive(parameter, data)) {
                MS3::nextMessage = millis() + MS3_RECEIVE_INTERVAL_MSEC;
                return MS3_DATA_RECEIVED;
            }

            // Check if we need to send out a queued item.
            queueItem item = {};
            if (MS3::nextMessage <= millis() && Queue.read(item)) {

                // Construct the data to send to the MS-3.
                byte input[item.dataLength] = {0};
                input[item.dataLength - 1] = item.data % 128;
                if (item.dataLength >= 2) {
                    input[item.dataLength - 2] = (item.data >= 128) ? 1 : 0;
                }

                // Send the queue item to the MS-3.
                MS3::send(
                    item.address,
                    input,
                    item.dataLength,
                    item.operation
                );

                // Store when the next message may be sent.
                MS3::nextMessage = millis() + (item.operation == MS3_READ ? MS3_READ_INTERVAL_MSEC : MS3_WRITE_INTERVAL_MSEC);
                return MS3_DATA_SENT;
            }

            // Are we done? Did we wait for the last message to finish?
            if (Queue.isEmpty()) {
                return (MS3::nextMessage > millis()) ? MS3_ALMOST_IDLE : MS3_IDLE;
            }

            // Nothing interesting happened.
            return MS3_NOTHING_HAPPENED;
        }

        /**
         * Set this single byte parameter on the MS-3. Optionally pad it with leading zero-bytes with a dataLength >= 1.
         */
        void write(const unsigned long address, byte data, byte dataLength = 1) {
            Queue.write(address, data, dataLength, MS3_WRITE);
        }

        /**
         * Tell the MS-3 to send us the value of this parameter.
         */
        void read(const unsigned long address, byte data) {
            Queue.write(address, data, 4, MS3_READ);
        }

        /**
         * Flush the queue if it's not empty.
         */
        void flushQueue() {
            if (!Queue.isEmpty()) {
                Queue.flush();
            }
        }
};

#endif
