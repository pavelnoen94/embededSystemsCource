#include <stdbool.h>
#include <time.h>
#include "hardware.h"
#include "elevator.h"
#include "sensor.h"
#include "actions.h"
#include "routines.h"

void serveFloor(){
    if(!atSomeFloor()){
        return;
    }
    elevatorStop();
    openDoor();

    // start a timer and hold the door open for a time without obstructions
    time_t startTime = clock() * 1000 / CLOCKS_PER_SEC;
    time_t endTime = startTime + DOOR_OPEN_TIME*1000;
    while (startTime < endTime){
        getOrders();

        if(readObstruction() || readStop()){
            // reset timer
            startTime = clock() * 1000 / CLOCKS_PER_SEC;
        }
    }
    // close the door
    closeDoor();
    return;

}