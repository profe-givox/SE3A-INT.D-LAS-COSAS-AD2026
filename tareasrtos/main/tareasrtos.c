#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

TaskHandle_t taskHandle1=NULL;
TaskHandle_t taskHandle2=NULL;


void task1(void *args){
    int counter=0;

    printf("Task 1 is running\n");
    while(1){
        printf("Task 1 counter: %d\n", counter++);
        if(counter == 3){
            printf("Task 1 is suspending Task 2\n");
            //vTaskSuspend(taskHandle2);
        }
        if(counter == 6){
            printf("Task 1 is resuming Task 2\n");
            //vTaskResume(taskHandle2);
        }
        if(counter == 10){
            printf("Task 1 is deleting task2\n");
            //vTaskDelete(taskHandle2);
        }
        if(counter >= 15){
            printf("Task 1 finished\n");
            //vTaskDelete(NULL);  
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task2(void *args){
    int counter=0;

    printf("Task 2 is running\n");
    while(1){
        printf("Task 2 counter: %d\n", counter++);
        if(counter == 5){
            printf("Task 2 is suspending Task 1\n");
            //vTaskSuspend(taskHandle1);
            //vTaskResume(taskHandle1);
        }
        if(counter >= 15){
            printf("Task 2 finished\n");
            //vTaskDelete(NULL);  
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void app_main(void) 
{
    
    xTaskCreate(task1, "Task1", 2048,
         NULL, 10, &taskHandle1);
    xTaskCreate(task2, "Task2", 2048,
         NULL, 10, &taskHandle2);
}
