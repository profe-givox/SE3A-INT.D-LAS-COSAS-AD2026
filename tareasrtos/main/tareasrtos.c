#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

TaskHandle_t taskHandle1=NULL;


void task1(void *args){
    while(1){
        printf("Task 1 is running\n");
    }
}

void app_main(void)
{
    
    xTaskCreate(task1, "Task1", 2048,
         NULL, 10, &taskHandle1);
}
