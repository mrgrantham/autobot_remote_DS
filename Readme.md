# Nintendo DS Remote Control for Autobot Autonomous Vehicle


This code allows the DS to connect to and send packets over an unprotected local wifi network.

Note: The Nintendo DS does not support modern WPA protection. Supposedly it supports older WEP password protection but I have not seen that work

    Default IP: 192.168.0.101
    Default port: 11156
    
    Both can be reassigned at run time

### Controls
* D-pad can control steering and velocity
* Right Shoulder button reset velocity to 0
* Left Shoulder button reset steering angle to 0

* B Resets velocity to 0
* A resets steering to straight ahead

* Touch screen can be used to control steering
* Closing of the lid resets velocity to 0 and steering to straight ahead