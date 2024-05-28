#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define MAX_NUMBER_ITEMS 10
#define MAX_NUMBER_CUSTOMERS 10
#define MAX_NUMBER_SALE_ASSISTANTS 3

#define MIN_TIME_READING_MENU 3
#define MAX_TIME_READING_MENU 6
#define MIN_TIME_CHECKING_ORDER 1
#define MAX_TIME_CHECKING_ORDER 2

#define MAX_TIME_SIMULATION 30

#define SEMAPHORE_ORDERS_WRITE_ID 1
#define SEMAPHORE_ORDERS_READ_ID 2
#define SEMAPHORE_CONSOLE_ID 3
#define SEMAPHORE_READ_COUNT_ID 4
#define SEMAPHORE_WRITE_COUNT_ID 5
#define SEMAPHORE_READ_QUEUE_ID 6
#define SEMAPHORE_WRITE_MENU_ID 7

typedef struct {
	int id;
	char name[15];
	int price;
	int totalOrdered;
} PRODUCT;

typedef struct {
	int customerId;
	int itemId;
	int amount;
	bool done;
} ORDER;

void createStore(int*, int);
void createBuyingBoard(int*, int);
void semWait(int, int);
void semSignal(int, int);
double getTime();
void printMenu(PRODUCT* , int);
void customerProcess(int, int, int, ORDER*, PRODUCT*, int);
void saleAssistantProcess(int, int, int, ORDER*, PRODUCT*, int, int);

