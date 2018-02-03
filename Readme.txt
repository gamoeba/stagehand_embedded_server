Building

Build for emulator:
tizen build-native -a x86

Build for target:
tizen build-native -a arm

Deploy to your device

sdb push Debug/libstagehand_armel.so /opt/usr/home/owner/data/stagehand/
sdb push stagehand.zip /opt/usr/home/owner/data/stagehand/

(use libstagehand_i386.so for the emulator)

Link into app

copy client_include/stagehand.h into your app
create an instance of the stagehand debug server before you start the main loop eg.

int DALI_EXPORT_API main(int argc, char **argv)
{
  Application app = Application::New(&argc, &argv);
  StageHand debug_server; // This is the line to add
  ExampleController test(app);
  app.MainLoop();
  return 0;
}

Setup port forwarding then start your app and then connect your web browser to localhost port 27000

sdb forward tcp:27000 tcp:27000
http://localhost:27000

If you can connect directly to the wifi that the device is registered on you can skip the port forwarding

eg.
http://<device-ip>:27000/
