# Copyright 2023 <Elena Dulgheru 314CA elenadulgheru03@gmail.com>

	Program is called Load Balancer and its purpose is to implement efficiently
a load balancer using Consistent Hashing. The program consists of 3 .c files and
3 .h files.
	Program starts in main.c file and then goes to load_balancer.c where the
logic is implemented. This file contains 17 functions and declaration and
initialization of the load balancer. In load_balancer structure the fields are
server_memory ** - this is an array of servers, which is actually an array of
hashtables, due to the fact that a server is implemented as a hashtable;
label * - this is the 'hashring', is the array that stores the actual index of
a server on the correct position, the same position as in the array of servers;
nr_servers -> total number of servers and total_keys -> the total number of
objects stored on all servers.

	The functions implemented have the following functionality:
1) loader_add_server() - here 3 new servers are added to the  'hashring', the
	main id and its other labels. After getting the id of other 2 servers, it
	calls another function loader_add_server_helper(). In this function the
	hash of the current server is calculated and it traverses all servers and
	gets the correct position on which the new server must be put, by calling
	function add_server_at_pos(), where a server is initialized and added to
	the hashring. Here is also stored its position in the array of servers and
	its label.
	Also in loader_add_server_helper() function if the added server is not the
	only one and if the next server has any objects the rebalance_objects_add()
	function is called to rebalance all objects.

2) loader_remove_server() - in this function the given server and its replicas
are deleted and their objects, if exists, are rebalanced to the next server.
This is done in loader_remove_server_helper() function that calls another
functions: find_pos_remove() - to get the position of the server that will be
deleted and move_servers_left() where remaining servers from right are moved to
the left with one position and objects of the next server are rebalanced.

3)loader_store() - here a new object is stored in a server. This function calls
server_store()

4) loader_retrieve() - this function traverses all servers and searches the
value of the given key, if it exists. It also calls function server_retrieve()
from serve.c, and returns via parameter the index of the server which contains
the key.

5) free_load_balancer() - it frees all resources from the load_balancer by
calling function free_server_memory() from server.c

	In server.c I have implemented functions specific to a hashtable, with all
	its operation, as taught in laboratory, but adapted to the current request:
the information in a node is a structure containing a key and a value field.
