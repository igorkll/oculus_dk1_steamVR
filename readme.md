# oculus_dk1_steamVR
* SteamVR didn't work with it through native runtime and I had to do it. I JUST WANT TO PLAY!!!!!!!!!
* with this driver, your Oculus devkit 1 will work in SteamVR without Oculus runtime, without oculus home and oculus link.

## installation
* close SteamVR
* remove oculus runtime if you installed it.
* similar with openXR, oculus home and oculus link
* make sure that VR is displayed in the system simply as a monitor and is not the main monitor
* download the project release and unzip it to a folder that is convenient for you (you will not be able to delete files later, this will remove the driver)
* run the python script: registry_driver.py

## warnings
* if tracking doesn't work, launch steamVR first and only then the game
* if you see a red screen with glasses, try moving the mouse to the VR monitor and making a click
* if tracking doesn't work, try closing SteamVR and your VR game. then make sure that there are no processes running on the computer oculus_tunnel_32.exe and vrserver.exe if this does not help, restart your computer
* oculus_tunnel must be built as a 32-bit program(x86)
* the main "driver" project should be built as a 64-bit DLL