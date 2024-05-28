Task completed:
	Aleksandr Kisliak

Run:
```bash
gcc main.c -o main && ./main 10 10 3 30
```
./main <num_items> <num_customers> <num_sale_assistants> <simulation_time>

## Introduction
A new fashion store in Beer Sheva would like you to create a simulation program of its work. It will have a new interactive menu. A number of sale assistants work in the store. Customers can “come” to the store and check the list of items for sale, and buy a certain amount of the items they want.

## Simulation
In this assignment, you will implement a program which simulates the problem described above. Each sale assistant and each customer will be simulated by a separate process. The different products (items) will be listed in shared memory so all processes can access them. The entire simulation must use one or more sections of shmem.
1.	Main process (the manager)
	a.	Fill out the products - 5-7 items: Id, Name up to 15 chars, Price up to 100 NIS, TotalOrdered initialized to 0, and print it.
	b.	Create “buying board” for orders (one for each customer): CustomerId, ItemId, Amount, Done as binary value initialize to true (to wait for orders).
	c.	The main process should create any needed semaphores and start all sub processes. Any sub process each type {Customer or Sale Assistant} should have an index according to the creation order.
	d.	At the end of simulation time, main process should wait for termination of all sub processes, and print general information about the simulation: the total counts of each item ordered, the total income of the fashion store during the simulation.
2.	Customer process:
	- If not elapsed simulation time
	b.	Sleep for 3 to 6 seconds, randomly
	c.	Read the items in stock list (1 seconds)
	d.	If the previous order has not yet been done, loop to (a) 
	e.	With the probability 0.5, the customer will order as follows: 
		i.	Randomly choose an item, randomly choose an amount (between 1 and 4);
		ii.	Write the order to the “buying board” under customer index and set value of Done to false.
	f.	With the probability 0.5, the customer does not order!
	g.	Loop to (a)
4.	Sale Associate process:
	a.	If not elapsed simulation time
	b.	Sleep for 1 to 2 seconds, randomly
	c.	Read an order from the “buying board”
	d.	If there are no orders (Done is true), loop to (a)
	e.	If there is row that isn’t Done (value is false):
		i.	Add the amount ordered to the totals for the item in main menu
		ii.	Mark the order as Done (set to true)
	f.	Loop to (a) 

When a process is writing to a shared memory it must have unique access, with no other writers and no readers. This is the classic readers-writers problem.  Synchronization between sale assistants and customers should be done with the help of semaphores.
When starting the program, it should get the following arguments on the command line:
	•	Number of different items/products (up to 10)
	•	Number of sale assistants (up to 3)
	•	Number of customers (up to 10)
“Random” sleep times (a floating-point value):
	•	Time between customers reading the menu is 3 to 6 seconds
	•	Time between sale assistants checking orders is 1 to 2 seconds
	•	Total time running of simulation, less than 30 seconds 
Every process should print to stdout if it reads the menu, or if it orders. The message should contain time since start of simulation, message about the event, the customer or sale assistant number and PID of the process. All error messages should be written to stderr.
Notes:
	•	If there was an error in the input parameters you should correctly terminate the program and write the reason for the error.
	•	Use a shared memory for the menu (items 1(a) and 1(b)).
	•	Using a shared resource, such as display to output: make sure that your process has unique access to the output (use a semaphore to protect all output).
	•	Access the shared memory properly, according to the reader-writer algorithm (multiple readers are allowed, writers are allowed only one at a time with no reads). Use semaphores to implement this algorithm.
YOU MUST IMPLEMENT THIS ALGORITHM AT LEAST ONCE IN YOUR SOLUTION TO RECEIVE FULL CREDIT
