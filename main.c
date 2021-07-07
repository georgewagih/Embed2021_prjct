
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "diag/trace.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#define INCLUDE_vTaskSuspend      1
SemaphoreHandle_t receiver_sem=0;
SemaphoreHandle_t sender_sem=0;
TimerHandle_t receiver_timer;
TimerHandle_t sender_timer;
QueueHandle_t queue;
#define Treceiver pdMS_TO_TICKS(200)
int Tsender[]={100,140,180,220,260,300};
/*Global variables to count the total number of sent, blocked and received messages
-------------------------------------------------------------------------------*/
int sentM=0;
int blockedM=0;
int receivedM=0;
//-----------------------------------------------------------------------------
int i=0;  
#define CCM_RAM __attribute__((section(".ccmram")))



/*implementing the initialization function "init();"
---------------------------------------------------------*/
void init()
{
	trace_printf("the total number of successfully sent messages is: %d\n ",sentM);
	trace_printf("the total number of blocked sent messages is: %d\n ",blockedM);
	sentM=0;
	blockedM=0;
    receivedM=0;
    xQueueReset(queue);
/*if the Tsender array isn't over yet change the period of the sender timer, reset the receiver timer then increment i otherwise
print "Game over" and end excution*/
    if(i != 6)
    {
    	xTimerChangePeriod(sender_timer,pdMS_TO_TICKS(Tsender[i]),0);
    	xTimerReset(receiver_timer,0);
        i++;
    }
    else
    {
    	trace_puts("Game Over...\n");
    	xTimerDelete(receiver_timer,0);
		xTimerDelete(sender_timer,0);
		vQueueDelete(queue);
		vTaskEndScheduler();
    }
//---------------------------------------------------------
}


/*timers callback functions(basically give the semaphores to activate the dormant tasks)
----------------------------------------------*/
void receiver_callback(TimerHandle_t xTimer)
{
	if(receivedM != 500)
	   xSemaphoreGive(receiver_sem);
	else init();
}

void sender_callback(TimerHandle_t xTimer)
{
xSemaphoreGive(sender_sem);
}
/*----------------------------------------------*/

/*creating the sender and receiver tasks
----------------------------------------------*/
void sender_task(void*p)
{
	TickType_t Timesent;     //variable holds the current time in ticks
	char message_sent[20];  //a buffer of to contain the message to be sent
	while(1)
	{
		if(xSemaphoreTake(sender_sem,portMAX_DELAY)) //the task is indefinitely blocked on the semaphore sender_sem 
		{
			Timesent=xTaskGetTickCount();  //getting the current time in system ticks
			sprintf(message_sent,"Time is %lu",Timesent); //printing the message to the buffer
			if(xQueueSend(queue,message_sent,0)!=pdPASS)  //communicating with the queue to send the message
			   blockedM++;  //if the queue is full,the message is blocked and blockedM is increased 
			else sentM++;  //the message was successfully sent to the queue and sentM is increased
	    }
	}
}

void receiver_task(void*p)
{
	char message_received[20];  //a buffer in which the message is received from the queue
	while(1)
	{
		if(xSemaphoreTake(receiver_sem,portMAX_DELAY)) //the task is indefinitely blocked on the semaphore receiver_sem
		{
			if(xQueueReceive(queue,message_received,0)==pdTRUE) //communicating with the queue to receive the message
				{receivedM++; } //if the message was successfully received increment the counter
		}
	}
}
/*----------------------------------------------*/


// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

int main()
{
	// Add your code here.
	receiver_sem=xSemaphoreCreateBinary();
	sender_sem=xSemaphoreCreateBinary();
	queue=xQueueCreate(2,20*sizeof(char));
	receiver_timer=xTimerCreate("receiver_timer",Treceiver,pdTRUE,0,receiver_callback);
	sender_timer=xTimerCreate("sender_timer",pdMS_TO_TICKS(Tsender[0]),pdTRUE,0,sender_callback);
	xTaskCreate(sender_task,"sender_task",1000,NULL,1,NULL);
	xTaskCreate(receiver_task,"receiver_task",1000,NULL,2,NULL);
	init();
	vTaskStartScheduler();
	return 0;
}

#pragma GCC diagnostic pop

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

