#pragma once

struct PowerState {
  float batteryVolts = 0.0F;
  unsigned batteryPercent = 0;
  bool charging = false;
  bool usbPresent = false;
  bool regulatorOk = false;
};

class PowerMonitor {
 public:
  void begin();
  PowerState read() const;
};

