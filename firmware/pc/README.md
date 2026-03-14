# .wav file merger

The mergeWav.py is a 100% AI generated unedited python script you can use to merge the .wav files produced by the SDRecorder.

When recording, the SDRecorder switches to a new file every X minutes (1 minute for now, this will be an accessible parameter through wifi soon-ish), so that not much recording is lost when the battery charge is too low to power the device.

You may need to install the PyQt6 package:

`> pip install PyQt6`

Then run the script:

`> python mergeWav.py`

This python script with a simple GUI allows to stitch them easily.


![alt text](img/image.png)
