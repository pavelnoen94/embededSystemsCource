#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "headers/modes.h"
#include "headers/elevator.h"
#include "headers/hardware.h"
#include "headers/modeReaders.h"
#include "headers/interface.h"
#include "headers/modeSelector.h"

static void sigHandler(int sig) {

	switch(sig){
		case SIGSEGV:
			printf("[Warning]: Recieved Segmentation fault.\n");
			return;
		case SIGINT:

		default:
			printf("Resieved signal %d, Terminating elevator\n", sig);
			hardware_command_movement(HARDWARE_MOVEMENT_STOP);
			exit(0);
	}

}
void startUp() {
	printf("\n");
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
	printf("To terminalte program press ctrl+c or type 'kill -9 %d in terminal'\n", getpid());
	signal(SIGTERM, sigHandler);
	signal(SIGKILL, sigHandler);
	signal(SIGSEGV, sigHandler);

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
	readFloorSensors();

	while (!hasOrders) {
		getOrders();

		if(readStop() || orderAt(position.lastKnownFloor)){
			return;
		}
	}

}

void running() {

	while (hasOrders) {
		// update sensors and set direction
		
		getOrders();
		readFloorSensors();
		direction = getDirection(targetFloor);
		
		// wait untill a floor with orders is reached
		while(!atTargetFloor()){
			findTargetFloor();
			if(((targetFloor > position.lastKnownFloor) && direction == DOWN ) || ((targetFloor < position.lastKnownFloor) && (direction == UP))){
				printf("[Warning]: Elevator is moving in opposite direction of order.\n");
				break;
			}

			if (direction == UP) {
				elevatorMoveUp();
			}

			if (direction == DOWN) {
				elevatorMoveDown();
			}

			
			getOrders();
			readFloorSensors();
			
			if(readStop()){
				return;
			}

		}

		elevatorStop();
		return;
	}
}

void serveFloor(){
    if(!atSomeFloor()){
      return;
    }
    readFloorSensors();
    clearAllOrdersAtThisFloor();
    elevatorStop();
    openDoor();

    // start a timer and hold the door open for a time without obstructions
    time_t startTime = clock() / CLOCKS_PER_SEC;
    time_t start = startTime;
    time_t endTime = startTime + DOOR_OPEN_TIME;
    
    
    while (startTime < endTime){
        getOrders();

        if(readObstruction() || readStop() || orderAt(position.lastKnownFloor)){
			if(readStop()){
				clearAllOrders();
			}
			// reset timer
			endTime = clock() / CLOCKS_PER_SEC + DOOR_OPEN_TIME;
			clearAllOrdersAtThisFloor();
        }
        startTime = clock()/ CLOCKS_PER_SEC;
    }
    printf("The door was open for %d seconds\n", (int)(endTime - start));
    closeDoor();
    return;

}

void emergency() {
	emergencyState = true;
  clearAllOrders();
  elevatorStop();

  while(readStop()){

  }
  return;
}
