#include "main.h"

int readCount = 0, writeCount = 0;
double startTime;

int main(int argc, char* argv[]) {
	if (argc != 5) {
		fprintf(stderr, "Invalid number of arguments. \nUsage: ./main <num_items>(Maximum: %d) <num_customers>(Maximum: %d) <num_sale_assistants>(Maximum: %d) <simulation_time>(Maximum: %d).\n", MAX_NUMBER_ITEMS, MAX_NUMBER_CUSTOMERS, MAX_NUMBER_SALE_ASSISTANTS, MAX_TIME_SIMULATION);
		return 1;
	}
	
	int numItems = atoi(argv[1]);
	int numCustomers = atoi(argv[2]);
	int numSaleAssistants = atoi(argv[3]);
	int simulationTime = atoi(argv[4]);
	
	if (numItems <= 0 || numItems > MAX_NUMBER_ITEMS || numCustomers <= 0 || numCustomers > MAX_NUMBER_CUSTOMERS || numSaleAssistants <= 0 || numSaleAssistants > MAX_NUMBER_SALE_ASSISTANTS || simulationTime <= 0 || simulationTime > MAX_TIME_SIMULATION) {
		fprintf(stderr, "Invalid input parameters.\n");
		return 1;
	}

	startTime = getTime();
	
	printf("======Welcome to Beer Sheva Store======\n");
	printf("Simulation time: %d\n", simulationTime);
	printf("Number of products: %d\n", numItems);
	printf("Number of customers: %d\n", numCustomers);
	printf("Number of sale assistants: %d\n", numSaleAssistants);
	printf("=======================================\n\n");
	printf("%.3f Main process ID (%d) started\n\n", getTime() - startTime, getpid());
	
	int shmStoreId, shmBBId;
	PRODUCT *shmStore;
	ORDER *shmBB;
	
	createStore(&shmStoreId, numItems);
	if ((shmStore = shmat(shmStoreId, NULL, 0)) == (PRODUCT *)-1) {
		perror("connecting to shared memory (store) error");
		exit(1);
	}
	createBuyingBoard(&shmBBId, numCustomers);
	if ((shmBB = shmat(shmBBId, NULL, 0)) == (ORDER *)-1) {
		perror("connecting to shared memory (BB) error");
		exit(1);
	}
	
	
	for(int i = 0; i < numItems; i++) {
		shmStore[i].id = i;
		sprintf(shmStore[i].name, "Thing %d", i);
		shmStore[i].price = rand() % 100;
		shmStore[i].totalOrdered = 0;
	}
	
	for(int i = 0; i < numCustomers; i++) {
		shmBB[i].customerId = i;
		shmBB[i].itemId = -1;
		shmBB[i].amount = 0;
		shmBB[i].done = true;
	}
	
	printMenu(shmStore, numItems);
	
	key_t semKey;
	int semid;
	if ((semKey = ftok(".", 'B')) < 0) {
		perror("ftok semaphore error");
		exit(EXIT_FAILURE);
	}
	semid = semget(semKey, 1, 0666 | IPC_CREAT);
	
	semctl(semid, SEMAPHORE_ORDERS_WRITE_ID, SETVAL, 1);
	semctl(semid, SEMAPHORE_ORDERS_READ_ID, SETVAL, 1);
	semctl(semid, SEMAPHORE_CONSOLE_ID, SETVAL, 1);
	semctl(semid, SEMAPHORE_READ_COUNT_ID, SETVAL, 1);
	semctl(semid, SEMAPHORE_WRITE_COUNT_ID, SETVAL, 1);
	semctl(semid, SEMAPHORE_READ_QUEUE_ID, SETVAL, 1);
	semctl(semid, SEMAPHORE_WRITE_MENU_ID, SETVAL, 1);
	
	printf("[%.3f] Main process starts creating sub-process\n", getTime() - startTime);
	
	
	for(int i = 0; i < numCustomers; i++) {
		pid_t pid = fork();
		if (pid == 0) {
			customerProcess(i, semid, numItems, shmBB, shmStore,  simulationTime);
			exit(EXIT_SUCCESS);
		}
	}
	
	for(int i = 0; i < numSaleAssistants; i++) {
		pid_t pid = fork();
		if (pid == 0) {
			saleAssistantProcess(i, semid, numItems, shmBB, shmStore,  simulationTime, numCustomers);
			exit(EXIT_SUCCESS);
		}
	}
	
	for(int i = 0; i < numCustomers + numSaleAssistants; i++) { 
		wait(NULL);
	}
	
	printMenu(shmStore, numItems);
	
	
	int totalOrders = 0;
	int income = 0;
	for(int i = 0; i < numItems; i++) {
		if (shmStore[i].totalOrdered != 0) {
			totalOrders += shmStore[i].totalOrdered;
			income += shmStore[i].totalOrdered * shmStore[i].price;
		}
	}
	
	printf("The store sold %d items worth NIS %d.\n", totalOrders, income);
	
	
	shmdt(shmStore);
	shmctl(shmStoreId, IPC_RMID, NULL);
	shmdt(shmBB);
	shmctl(shmBBId, IPC_RMID, NULL);
	semctl(semid, 0, IPC_RMID, 0);
	
	printf("[%.3f] Main process ID [%d] ended work\n", getTime() - startTime, getpid());
	printf("===============TODA RABA===============\n");

	return 0;
}


//================================================================
void saleAssistantProcess(int saleAssistantId, int semId, int numItems, ORDER *shmBB, PRODUCT *shmStore, int simulationTime, int numCustomers) {
	srand(time(NULL) + saleAssistantId);

	semWait(SEMAPHORE_CONSOLE_ID, semId);
	printf("[%.3f] SaleAssistant [%d]: created PID %d PPID %d\n", getTime() - startTime, saleAssistantId, getpid(), getppid());
	semSignal(SEMAPHORE_CONSOLE_ID, semId);
	
	while(getTime() - startTime < simulationTime) {
		int sleepTime = rand() % (MAX_TIME_CHECKING_ORDER - MIN_TIME_CHECKING_ORDER + 1) + MIN_TIME_CHECKING_ORDER;
		sleep(sleepTime);
		
		ORDER order = {-1, -1, -1, true};
		
		semWait(SEMAPHORE_READ_QUEUE_ID, semId);
		semWait(SEMAPHORE_ORDERS_READ_ID, semId);
		semWait(SEMAPHORE_READ_COUNT_ID, semId);
		readCount++;
		if (readCount == 1) {
			semWait(SEMAPHORE_ORDERS_WRITE_ID, semId);
		}
		semSignal(SEMAPHORE_READ_COUNT_ID, semId);
		semSignal(SEMAPHORE_ORDERS_READ_ID, semId);
		semSignal(SEMAPHORE_READ_QUEUE_ID, semId);
		
		for(int i = 0; i < numCustomers; i++) {
			if (shmBB[i].done == false && getTime() - startTime < simulationTime) {
				order = shmBB[i];
				
				semWait(SEMAPHORE_CONSOLE_ID, semId);
				printf("[%.3f][PID %d] SaleAssistant [%d]: read the order(%d) and started to process it\n", getTime() - startTime, getpid(), saleAssistantId, i);
				semSignal(SEMAPHORE_CONSOLE_ID, semId);
				
				break;
			}
		}
		
		semWait(SEMAPHORE_READ_COUNT_ID, semId);
		readCount--;
		if (readCount == 0) {
			semSignal(SEMAPHORE_ORDERS_WRITE_ID, semId);
		}
		semSignal(SEMAPHORE_READ_COUNT_ID, semId);
		
		
		if (order.customerId != -1) {
			semWait(SEMAPHORE_WRITE_COUNT_ID, semId);
			writeCount++;
			if (writeCount == 1) {
				semWait(SEMAPHORE_ORDERS_READ_ID, semId);
			}
			semSignal(SEMAPHORE_WRITE_COUNT_ID, semId);
			semWait(SEMAPHORE_ORDERS_WRITE_ID, semId);
			
			if (getTime() - startTime < simulationTime) {
				shmBB[order.customerId].done = true;
				
				semWait(SEMAPHORE_WRITE_MENU_ID, semId);
				shmStore[order.itemId].totalOrdered += order.amount;
				semSignal(SEMAPHORE_WRITE_MENU_ID, semId);
			}
			semWait(SEMAPHORE_CONSOLE_ID, semId);
			if (getTime() - startTime < simulationTime) {
				printf("[%.3f][PID %d] SaleAssistant [%d]: completed the order(%d)\n", getTime() - startTime, getpid(), saleAssistantId, order.customerId);
			}
			semSignal(SEMAPHORE_CONSOLE_ID, semId);
			
			semSignal(SEMAPHORE_ORDERS_WRITE_ID, semId);
			semWait(SEMAPHORE_WRITE_COUNT_ID, semId);
			writeCount--;
			if (writeCount == 0) {
				semSignal(SEMAPHORE_ORDERS_READ_ID, semId);
			}
			semSignal(SEMAPHORE_WRITE_COUNT_ID, semId);
			
			
		} else {
			semWait(SEMAPHORE_CONSOLE_ID, semId);
			if (getTime() - startTime < simulationTime) {
				printf("[%.3f][PID %d] SaleAssistant [%d]: checked the orders. No orders available.\n", getTime() - startTime, getpid(), saleAssistantId);
			}
			semSignal(SEMAPHORE_CONSOLE_ID, semId);
		}
	}
	
	semWait(SEMAPHORE_CONSOLE_ID, semId);
	printf("[%.3f] SaleAssistant [%d]: PID %d ended work PPID %d\n", getTime() - startTime, saleAssistantId, getpid(), getppid());
	semSignal(SEMAPHORE_CONSOLE_ID, semId);
}


//================================================================
void customerProcess(int customerId, int semId, int numItems, ORDER *shmBB, PRODUCT *shmStore, int simulationTime) {
	srand(time(NULL) + customerId);
	
	semWait(SEMAPHORE_CONSOLE_ID, semId);
	printf("[%.3f]Customer [%d]: created PID %d PPID %d\n", getTime() - startTime, customerId, getpid(), getppid());
	semSignal(SEMAPHORE_CONSOLE_ID, semId);
	
	while(getTime() - startTime < simulationTime) {
		int sleepTime = rand() % (MAX_TIME_READING_MENU - MIN_TIME_READING_MENU + 1) + MIN_TIME_READING_MENU;
		
		sleep(sleepTime);
		sleep(1);
		
		if (shmBB[customerId].done == false) {
			continue;
		}
		
		int randomItem = rand() % numItems;
		if (rand() % 2 == 0) {
			int amount = rand() % (4 - 1 + 1) + 1;
			
			semWait(SEMAPHORE_WRITE_COUNT_ID, semId);
			writeCount++;
			if (writeCount == 1) {
				semWait(SEMAPHORE_ORDERS_READ_ID, semId);
			}
			semSignal(SEMAPHORE_WRITE_COUNT_ID, semId);
			semWait(SEMAPHORE_ORDERS_WRITE_ID, semId);
			
			if (getTime() - startTime < simulationTime) {
				shmBB[customerId].customerId = customerId;
				shmBB[customerId].itemId = randomItem;
				shmBB[customerId].amount = amount;
				shmBB[customerId].done = false;
			}
			
			semSignal(SEMAPHORE_ORDERS_WRITE_ID, semId);
			semWait(SEMAPHORE_WRITE_COUNT_ID, semId);
			writeCount--;
			if (writeCount == 0) {
				semSignal(SEMAPHORE_ORDERS_READ_ID, semId);
			}
			semSignal(SEMAPHORE_WRITE_COUNT_ID, semId);
			
			semWait(SEMAPHORE_CONSOLE_ID, semId);
			if (getTime() - startTime < simulationTime) {
				printf("[%.3f][PID %d] Customer [%d]: thinks about Item %d. Ordered (Amount = %d)\n",getTime() - startTime, getpid(), customerId, randomItem, amount);
			}
			semSignal(SEMAPHORE_CONSOLE_ID, semId);
		} else {
			semWait(SEMAPHORE_CONSOLE_ID, semId);
			if (getTime() - startTime < simulationTime) {
				printf("[%.3f][PID %d] Customer [%d]: thinks about Item %d. Doesn't want to order\n", getTime() - startTime, getpid(), customerId, randomItem);
			}
			semSignal(SEMAPHORE_CONSOLE_ID, semId);
		}
	}
	
	semWait(SEMAPHORE_CONSOLE_ID, semId);
	printf("[%.3f] Customer [%d]: PID %d ended work PPID %d\n", getTime() - startTime, customerId, getpid(), getppid());
	semSignal(SEMAPHORE_CONSOLE_ID, semId);
}


//================================================================
void printMenu(PRODUCT* shmStore, int numItems) {
	printf("===============Menu List===============\n");
	printf("Id\tName\tPrice\tOrders\n");
	for(int i = 0; i < numItems; i++) {
		printf("%d\t%s\t%d\t%d\n", shmStore[i].id, shmStore[i].name, shmStore[i].price, shmStore[i].totalOrdered);
	}
	printf("=======================================\n\n");
}



//================================================================
double getTime() {
	struct timespec currentTime;
	clock_gettime(CLOCK_REALTIME, &currentTime);
	return currentTime.tv_sec + (double) currentTime.tv_nsec / 1e9;
}


//================================================================
void createStore(int *shmStoreId, int productsCount) {
	int size = sizeof(PRODUCT) * productsCount;
	
	key_t key = ftok("test.test", 'A');
	*shmStoreId = shmget(key, size, IPC_CREAT | 0660);
	
	if (shmStoreId < 0) {
		perror("creating shared memory (store) failed");
		exit(1);
	}
}


//================================================================
void createBuyingBoard(int *shmBBId, int countOfCustomers) {
	int size = sizeof(ORDER) * countOfCustomers;
	
	key_t key = ftok(".", 'D');
	*shmBBId = shmget(key, size, IPC_CREAT | 0660);
	
	if (shmBBId < 0) {
		perror("creating shared memory for BB failed");
		exit(1);
	}
}


//================================================================
void semWait(int semType, int semId) {
	struct sembuf semOp;
	semOp.sem_num = semType;
	semOp.sem_op = -1;
	semOp.sem_flg = 0;
	semop(semId, &semOp, 1);
}
void semSignal(int semType, int semId) {
	struct sembuf semOp;
	semOp.sem_num = semType;
	semOp.sem_op = 1;
	semOp.sem_flg = 0;
	semop(semId, &semOp, 1);
}

