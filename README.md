# A Timer implementation in a producer-consumer manner
A producer-consumer type of problem that "produces" a new item at a specified period  while also using pthreads to parallelize the generation and consumption procedures.

This repository consists of two branches:
* `main`: containing the program version where TasksToExecute are limited and the program terminates after finishing all of them
* `tests`: containing a python script to run the program version where TasksToExecute are unlimited, with different arguments and for a user-specified time duration and extract the statistics of each test
