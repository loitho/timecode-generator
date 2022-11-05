# timecode-generator
afnor nfs 87500 and SMPTE timecode generator


https://github.com/InductiveComputerScience/pbPlots

## Wiringpi lib
cd /tmp
wget https://project-downloads.drogon.net/wiringpi-latest.deb
sudo dpkg -i wiringpi-latest.deb
https://forums.raspberrypi.com/viewtopic.php?t=51619


BAUD RATE a 400 000
https://gist.github.com/ribasco/c22ab6b791e681800df47dd0a46c7c3a
sudo nano /boot/config.txt
Find the line containing dtparam=i2c_arm=on
Add i2c_arm_baudrate=<new speed> (Separate with a Comma)
dtparam=i2c_arm=on,i2c_arm_baudrate=400000
dtparam=i2c_arm=on,i2c_arm_baudrate=3400000

mcp4921 

https://github.com/LdB-ECM/linux_device_access