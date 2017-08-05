SHA2017 badge Gameboy emulator
===

![Mario on the badge](https://j.gifs.com/y8Z8yw.gif)

This is a hack of the [gnuboy](https://github.com/rofl0r/gnuboy) gameboy
emulator to work with the [SHA2017 badge firmware](https://github.com/SHA2017-badge/Firmware)
(specifically the [components/badge/ director](https://github.com/SHA2017-badge/Firmware/tree/master/components/badge) converted to work with Arduino and the
[ESP32 extensions](https://github.com/espressif/arduino-esp32#installation-instructions).
It is literally an overnight hack, so the code is low quality and there are
frequently problems with the emulation.

*NOTE:* There is no ROM image included in this tree.  You must supply your
own ROM file, named `game.rom` and run this command to translate it into
a compilable `.c` file:

```
xxd -i game.rom | sed 's/unsigned/const unsigned/' > game.c
```


