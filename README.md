# A Timer implementation in a producer-consumer manner
A producer-consumer type of problem that "produces" a new item at a specified period  while also using pthreads to parallelize the generation and consumption procedures.

This repository consists of two branches:
* `main`: containing the program version where TasksToExecute are limited and the program terminates after finishing all of them
* `tests`(1): containing a python script to run the program version where TasksToExecute are unlimited, with different arguments and for a user-specified time duration and extract the statistics of each test
* `real-time-impl`(2): similar program version to the `tests` one but with adjusted p, q and queuesize arguments to decrease the time interval when run along with a program argument of t=1

(1) Python should be installed in the system to acquire the data of each test run. The tests were conducted with the python script first saving the data to the SD card of the Raspberry Pi and then running the rest on a laptop to create the plots. Only the basic python libraries need to included in the Raspberry Pi, so not any additional package installation is necessary for that part (since the plotting happens on a laptop). 

(2) The timer's function was measured to last around 20us and the time it took for the producer to add an item was assumed to be equal to the mean value extracted from test 1 plus the timer's period. There are two python scripts: 
* one to extract the stats of program runs
* one to create a 4D graph of the time interval based onn the p, q and n variables (**Note: The only line to be printed by the program should have the form `Time interval: 1234 us`**)
