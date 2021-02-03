# loopor
Looper plugin for LV2, specifically for the Mod Devices pedal board. Tested on Mod Duo and Mod Dwarf.

NOTE: This is a very early version, no warranty for anything is given! Use at your own risk (see the license).

Features:
* Stereo inputs and outputs
* Compiled in max number of overdubs (currently 128), a compiled max overall recording time (currently 6 minutes)
* Configurable input threshold; when starting the recording it can wait until a certain threshold is reached.
* Record / Play, Undo, Redo, Reset and Dub buttons
* No clicks even when sounds is still playing at loop end
* NEW: Configurable amount of dry signal routed to the outputs (added in version 4)

Usage:
* Adjust the "Threshold" to only start recording once playing has started. If set to the lowest value, recording will start immediately.
  Otherwise it will start recording when the first sound comes in. The threshold can be used to filter out noise. 
* Adjust the "Dry Amount" to reduce the volume of the input signal directly routed to the output. Setting it to 0 means you will only hear
  any looped sounds, no direct sound.
* Press the "Activate" button to start recording the first dub. Press again to stop recording. The first dub's length will define the length
  of all loops. Recording of all but the first dubs will stop when the loop length is reached. There is one exception: When the threshold
  is configured and no audio was recorded yet, then recording will not stop at the end of the loop.
* Double press the "Activate" button to reset the looper, clearing all loops.
* Press the "Undo" button to go back one dub. Undoing the first dub will also stop playing.
* Press the "Redo" button to redo a dub. Redoing is possible as many times as undo was used before. Redoing the first dub will start playing 
  again. Redoing is no longer possible when the next dub record is started! This will invalidate all undone dubs!
* Press the "Reset" button to stop recording if it is recording. Otherwise do the same as the "Undo" button.
* Double press the "Reset" button to reset the looper, clearing all loops.
* Press the "Dub" button to start recording a dub. When pressed while recording, the last dub is finished and immediately it starts a new 
  one.
* Double press the "Dub" button to reset the looper, clearing all loops.
* Note that any of those buttons can be assigned to the hardware buttons of the Mod board! Thus you can select which functionality you need.
