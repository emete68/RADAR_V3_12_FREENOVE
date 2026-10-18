#pragma once

class Connectivity {
 public:
  void begin();
  void update();
  bool connected() const;

 private:
  unsigned long lastAttemptMs_ = 0;
};

