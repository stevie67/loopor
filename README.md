# loopor
Looper plugin for LV2, specifically for the Mod Duo pedal board. Binary only, currently!

NOTE: This is a very early version, no warranty for anything is taken! Use at your own risk (see the license).

Features:
* Stereo inputs and outputs
* Compiled in max number of overdubs (currently 128), a compiled max overall recording time (currently 3 minutes)
* Configurable input threshold; when starting the recording it can wait until a certain threshold is reached.
* Record / Play, Undo, Redo, Reset and Dub buttons
* No clicks even when sounds is still playing at loop end

Usage:
* Adjust the threshold to only start recording once playing has started. If set to the lowest value, recording will start immediately.
  Otherwise it will start recording when the first sound comes in. The threshold can be used to filter out noise. 
* Press the activate button to start recording the first dub. Press again to stop recording. The first dub's length will define the length
  of all loops. Recording of all but the first dubs will stop when the loop length is reached. There is one exception: When the threshol
  is configured and no audio was recorded yet, then recording will not stop at the end of the loop.
* Press the Undo button to go back one dub. Undoing the first dub will also stop playing.
* Press the Redo button to redo a dub. Redoing is possible as many times as undo was used before. Redoing the first dub will start playing 
  again. Redoing is no longer possible when the next dub record is started! This will invalidate all undone dubs!
* Press the Reset button to reset to initial state. All dubs are cleared, the loop is reset, playing stops. Redoing is not possible anymore.
* Press the Dub button to start recording a dub. When pressed while recording, the last dub is finished and immediately it starts a new 
  one.
