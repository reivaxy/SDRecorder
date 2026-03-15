 # Audio recorder

 This is a tiny audio recorder with a small battery, saving .wav files on a SD card.
 
 About 90% of the code was written by model Claude Haiku 4.5 through github Copilot, with very, very little editing.

 Click briefly on the push button to start recording, and once again to stop recording.

 Click for at least two seconds on the push button to start a WiFi access point. Connect to it and open page http://192.168.1.4 to access the settings and the list of files. Files are downloadable and can be deleted.

 While recording, new files are created every X seconds, you can modify X on the settings page.

 To save power, the ESP goes to light sleep when not recording, and wakes up when the button is pushed. The delay before sleeping can be set in the parameters.

 The led flashes quickly if the SD card is absent or can't be written, and it flashes slowly (every 2 seconds while recording)

 Here is the prototype, waiting for version v1 of PCB. A v2 might feature an RGB led instead.

![20260308_143006](https://github.com/user-attachments/assets/27d30efa-58a1-4632-96ae-dd62605cc028)
