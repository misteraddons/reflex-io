#include <cassert>
#include <cstring>

#include "../firmware/core/dreamcast_twin_stick.h"

static controller_state_t frame(const char* name, bool connected) {
  controller_state_t result{};
  std::strncpy(result.controller_type_name, name,
               sizeof(result.controller_type_name) - 1);
  result.connected = connected;
  return result;
}

int main() {
  controller_state_t ports[2] = {
    frame("Twin-Stick", true),
    frame("Dreamcast", true),
  };
  assert(controllerFramesContainDreamcastTwinStick(ports, 2));
  assert(controllerFrameIsDreamcastTwinStick(ports[0]));
  assert(!controllerFrameIsDreamcastTwinStick(ports[1]));

  ports[0] = frame("Dreamcast", true);
  ports[1] = frame("Twin-Stick", true);
  assert(controllerFramesContainDreamcastTwinStick(ports, 2));
  assert(!controllerFrameIsDreamcastTwinStick(ports[0]));
  assert(controllerFrameIsDreamcastTwinStick(ports[1]));

  ports[0].connected = false;
  assert(controllerFramesContainDreamcastTwinStick(ports, 2));
  ports[0].connected = true;
  assert(controllerFramesContainDreamcastTwinStick(ports, 2));

  ports[1].connected = false;
  assert(!controllerFramesContainDreamcastTwinStick(ports, 2));
  ports[1].connected = true;
  assert(controllerFramesContainDreamcastTwinStick(ports, 2));

  ports[0] = frame("Twin-Stick", true);
  ports[1] = frame("Dreamcast", true);
  ports[0].connected = false;
  assert(!controllerFramesContainDreamcastTwinStick(ports, 2));
  ports[0].connected = true;
  assert(controllerFramesContainDreamcastTwinStick(ports, 2));

  assert(!controllerFramesContainDreamcastTwinStick(nullptr, 2));
  return 0;
}
