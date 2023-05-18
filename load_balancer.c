/* Copyright 2023 <> */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "load_balancer.h"
#define MIN_HASH 0
#define MAX_HASH 360
#define SIZE_MAX 1500

int flag = 0;

typedef struct load_balancer {
	/* TODO 0 */
	unsigned int nr_servers;
	unsigned int total_keys;
	int *label;
	// array of pointers to hashtables
	server_memory **server;
} load_balancer;

unsigned int hash_function_servers(void *a) {
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

unsigned int hash_function_key(void *a) {
	unsigned char *puchar_a = (unsigned char *)a;
	unsigned int hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c;

	return hash;
}

load_balancer *init_load_balancer() {
	/* TODO 1 */
	load_balancer *main = malloc(sizeof(load_balancer));
	main->label = calloc(SIZE_MAX, sizeof(int));
	main->nr_servers = 0;
	main->total_keys = 0;
	main->server = calloc(SIZE_MAX, sizeof(server_memory *));
	return main;
}

int min(int a, int b)
{
	return a < b ? a : b;
}

int get_index(int value)
{
	int nr = 0, pow = 1, cop = value;
	int cif[6] = {0};
	for (int i = 5; i >= 1; i--) {
		cif[i] = cop % 10;
		cop /= 10;
	}
	for (int i = 5; i >= 1; i--) {
		nr = nr + cif[i] * pow;
		pow *= 10;
	}
	return nr;
}

void rebalance_objects_add(load_balancer *main, int index_server_next, unsigned int pos,
						   unsigned int hash_server)
{
	if (pos == index_server_next || main->server[index_server_next]->size == 0)
		return;

	server_memory *current = main->server[pos];
	server_memory *next = main->server[index_server_next];

	unsigned int next_hash_server = hash_function_servers
						(&main->label[index_server_next]);

	for (unsigned int i = 0; i < next->hmax; i++) {
		ll_node_t *node = next->buckets[i]->head, *node_next;
		while (node != NULL) {
			info *node_info;
			node_next = node->next;
			node_info = (info *)node->data;
			char *key = node_info->key;
			// if I have to copy an object to the new server
			// and erase it from the previous server
			int debug = 0;
			if (strcmp(key, "387fcce410a123886646a6516bfaf00c") == 0) {
				debug = 1;
			}
			if ((hash_function_key(key) < hash_server && !(pos ==
				 main->nr_servers - 1 && hash_function_key(key) <
				 next_hash_server)) || (pos == 0 && hash_function_key(key)
				 > next_hash_server)) {
				char *value = node_info->value;
				server_store(current, key, value);
				server_remove(next, key);
			}
			node = node_next;
		}
	}
}

void add_server_at_pos(load_balancer *main, int pos, int server_id) {
	main->server[pos] = init_server_memory();
	main->server[pos]->hash_function = hash_function_key;
	main->label[pos] = server_id;
	main->nr_servers++;
}

void loader_add_server_helper(load_balancer *main, int server_id) {
	unsigned int hash_server = hash_function_servers(&server_id);

	int pos = -1;

	for (int i = 0; i < main->nr_servers; i++) {
		if (hash_function_servers(&main->label[i]) > hash_server ||
			(hash_function_servers(&main->label[i]) == hash_server &&
			 main->label[i] > server_id)) {
				pos = i;
				break;
			}
	}
	// add as last element
	if (pos == -1) {
		pos = main->nr_servers;
		add_server_at_pos(main, pos, server_id);

		rebalance_objects_add(main, 0, pos, hash_server);
	} else {
		for (int i = main->nr_servers; i > pos; i--) {
			main->server[i] = main->server[i - 1];
			main->label[i] = main->label[i - 1];
		}
		add_server_at_pos(main, pos, server_id);

		rebalance_objects_add(main, (pos + 1) % main->nr_servers, pos, hash_server);
	}
}

void loader_add_server(load_balancer *main, int server_id) {
	/* TODO 2 */
	int replica1_server = 100000  + server_id;
	int replica2_server = 100000 * 2 + server_id;

	loader_add_server_helper(main, server_id);
	loader_add_server_helper(main, replica1_server);
	loader_add_server_helper(main, replica2_server);
}

// se apeleaza doar daca mai exista cel putin un server unde 
// sa remapez elementele
// index este pozitia serverului din care se elimina obiecte
void rebalance_objects_remove(load_balancer *main, int next_idx, int index)
{
	if (next_idx == index || main->server[index]->size == 0) {
		main->total_keys -= main->server[index]->size;
		return;
	}

	server_memory *current, *next;
	current = main->server[index];
	next = main->server[next_idx];

	// parcurg tot hashtable ul, scot elemente si le pun in urm
	for (unsigned int i = 0; i < current->hmax; i++) {
		ll_node_t *node = current->buckets[i]->head;
		while (node != NULL) {
			info *node_info = (info *)node->data;
			char *key = node_info->key;
			char *value = node_info->value;
			server_store(next, key, value);
			node = node->next;
		}
	}
}


void move_servers_left(load_balancer *main, int position, int server_id) {
	rebalance_objects_remove(main, (position + 1) % main->nr_servers, position);

	free_server_memory(main->server[position]);
	main->server[position] = NULL;
	for (int i = position; i < main->nr_servers - 1; i++) {
		main->server[i] = main->server[i + 1];
		main->label[i] = main->label[i + 1];
	}
	main->server[main->nr_servers - 1] = NULL;
	main->nr_servers--;
}

int find_pos_remove(int *v, int n, unsigned int hash_server, int server_id)
{
	if  (n == 0)
		return 0;

	for (int i = 0; i < n; i++) {
		if (hash_function_servers(&v[i]) == hash_server && v[i] == server_id) {
			return i;
		}
	}
	return -1;
}

void loader_remove_server_helper(load_balancer *main, int server_id) {
	flag = 1;
	int hash_server = hash_function_servers(&server_id);

	int pos = find_pos_remove(main->label, main->nr_servers, hash_server, 
							  server_id);

	if (pos == -1)
		return;

	move_servers_left(main, pos, server_id);
}


void loader_remove_server(load_balancer *main, int server_id) {
	/* TODO 3 */
	int replica1_server = 100000  + server_id;
	int replica2_server = 100000 * 2 + server_id;

	loader_remove_server_helper(main, server_id);
	loader_remove_server_helper(main, replica1_server);
	loader_remove_server_helper(main, replica2_server);
}

void loader_store(load_balancer *main, char *key, char *value, int *server_id) {
	/* TODO 4 */
	unsigned int hash_prod = hash_function_key(key);
	unsigned int index = 0;
	for (unsigned int i = 0; i < main->nr_servers; i++) {
		unsigned int hash_server = hash_function_servers(&main->label[i]);
		if (hash_server > hash_prod) {
			index = i;
			break;
		}
	}

	server_store(main->server[index], key, value);
	main->total_keys++;
	*server_id = get_index(main->label[index]);
}

char *loader_retrieve(load_balancer *main, char *key, int *server_id) {
	/* TODO 5 */

	unsigned int hash_key = hash_function_key(key);
	for (unsigned int i = 0; i < main->nr_servers; i++) {
		unsigned int hash_server = hash_function_servers(&main->label[i]);

		if (hash_server > hash_key) {
			char *value = server_retrieve(main->server[i], key);
			*server_id = get_index(main->label[i]);
			return value;
		}
	}
   char *value = server_retrieve(main->server[0], key);
	*server_id = get_index(main->label[0]);
	return value;
}

void free_load_balancer(load_balancer *main) {
	/* TODO 6 */
	free(main->label);
	for (int i = 0; i < main->nr_servers; i++) {
		if (main->server[i] != NULL) {
			free_server_memory(main->server[i]);
			main->server[i] = NULL;
		}
	}
	free(main->server);
	free(main);
}
