# Valheim Pause

This is a silly little program I wrote to "pause" and "unpause" Valheim (as it doesn't support that in game).

It uses SIGSTOP and SIGCONT to "fake" pausing. It also uses JACK to watch for MIDI events from my Korg nanoKONTOL2.

The Play and Stop transport buttons are what triggers this.

This could easily be expanded to control more than Valheim but that's all I need it to do for now.

val is the test script for getting this going in the first place.