#include "modes.h"
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "actions.h"
#include "elevator.h"
#include "hardware.h"
#include "readers.h"
#include "routines.h"
#include "sensor.h"

static void sigint_handler(int sig) {
  (void)(sig);
  printf("\nResieved terminating signal %d, Terminating elevator\n", sig);
  hardware_command_movement(HARDWARE_MOVEMENT_STOP);
  exit(0);
}

void startUp() {
  // connect to hardware
  int error = hardware_init();
  if (error != 0) {
    fprintf(stderr, "Unable to initialize hardware\n");
    exit(1);
  }

  elevatorStop();
  clearAllOrders();
  closeDoor();

  // crash handling
  printf("\nTo terminalte program press ctrl+c or type 'kill -9 %d in terminal'\n", getpid());
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);
  signal(SIGSEGV, sigint_handler);

  // find floor
  readFloorSensors();
  if (!atSomeFloor()) {
    elevatorMoveUp();
    while (!atSomeFloor()) {
      readFloorSensors();
    }
  }

  printf("\ninit complete!\n");
  elevatorStop();
}

void idle() {
  status = IDLE;
  while (!hasOrders) {
    getOrders();
    
    if(readStop() || orderAt(lastKnownFloor)){
      serveFloor();
    }
  }
}

void serving() {
  status = SERVING;
  while (hasOrders) {
    getOrders();
    findTargetFloor();
    gotoFloor(targetFloor);
    clearAllOrdersAtThisFloor();
    if(readStop()){
      emergency();
    }
  }
}

void gotoFloor(int floor) {
  if (!isValidFloor(floor)) {
    printf("\nError: invalid argument in gotoFloor(%d)\n", floor);
  }
  status = MOVING;

  setTargetFloor(floor);
  direction = getDirection(getTargetFloor());

  if (direction == UP) {
    elevatorMoveUp();
  }

  if (direction == DOWN) {
    elevatorMoveDown();
  }

  if (direction == NONE) {
    serveFloor();
  }

  bool targetReached = false;
  while (!targetReached) {
    readFloorSensors();
    getOrders();

    if (lastKnownFloor == getTargetFloor()) {
      targetReached = true;
      clearAllOrdersAtThisFloor();
      serveFloor();
    }

    if (atSomeFloor()) {
      if (direction == UP && (upOrders[lastKnownFloor] || insideOrders[lastKnownFloor])) {
      }
    }
  }
  elevatorStop();
}

void emergency() {
  if (atSomeFloor()) {
    serveFloor();
    return;
  }

  // between floors
  elevatorStop();
  status = STOP;
  clearAllOrders();

  while (!hasOrders) {
    emergencyModeReader();
  }
  gotoFloor(getTargetFloor());
  status = IDLE;
  return;
}
