This  Arduino project solves the problem of common diesel heaters that do not have a regulation to turn off, once your desired temperature has reached. Instead, the device continues to run at a minimum level until it is shut down manually.

Thanks to the Blynk library which makes it simple to bring your data from an Arduino single-board computer to a web app.
You can design your own GUI and display the data like the temperature of different sensors. Also, it's possible to send a signal. In my case start and stop the heater manually from the remote.

The program has multiple features:
- start/stop heater manually via app (remote)
- change heating strength
- keep 17°C temperature (auto turn off and on)
- shutdown the system when outside is too warm
- auto-reconnect with the internet once the system is running
- the system doesn't need internet necessarily
- Consumer relay which triggers once the motor engine started
