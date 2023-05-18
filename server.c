/* Copyright 2023 <> */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "server.h"

#define HMAX 10

void free_node(ll_node_t *node)
{
	info *node_info = (info *)node->data;
	free(node_info->key);
	free(node_info->value);
	free(node->data);
	free(node);
	node = NULL;
}

linked_list_t *ll_create(unsigned int data_size)
{
	linked_list_t *ll;

	ll = malloc(sizeof(*ll));

	ll->head = NULL;
	ll->data_size = data_size;
	ll->size = 0;

	return ll;
}

void ll_add_nth_node(linked_list_t *list, unsigned int n, const void *new_data)
{
	ll_node_t *prev, *curr;
	ll_node_t *new_node;

	if (!list)
	{
		return;
	}

	/* n >= list->size inseamna adaugarea unui nou nod la finalul listei. */
	if (n > list->size)
	{
		n = list->size;
	}

	curr = list->head;
	prev = NULL;
	while (n > 0)
	{
		prev = curr;
		curr = curr->next;
		--n;
	}

	new_node = calloc(1, sizeof(*new_node));
	new_node->data = *(info **)new_data;

	new_node->next = curr;
	if (prev == NULL)
	{
		/* Adica n == 0. */
		list->head = new_node;
	}
	else
	{
		prev->next = new_node;
	}
	list->size++;
}

ll_node_t *ll_remove_nth_node(linked_list_t *list, unsigned int n)
{
	ll_node_t *prev, *curr;

	if (!list || !list->head)
	{
		printf("Invalid list!\n");
		return NULL;
	}

	if (n > list->size - 1)
	{
		n = list->size - 1;
	}

	curr = list->head;
	prev = NULL;
	while (n > 0)
	{
		prev = curr;
		curr = curr->next;
		--n;
	}

	if (prev == NULL)
	{
		if (list->size == 1)
			list->head = NULL;
		else
			list->head = curr->next;
	}
	else
	{
		prev->next = curr->next;
	}

	list->size--;

	return curr;
}

unsigned int ll_get_size(linked_list_t *list)
{
	if (list == NULL)
	{
		return -1;
	}

	return list->size;
}

void ll_free(linked_list_t **pp_list)
{
	if (!pp_list || !*pp_list)
		return;

	while (ll_get_size(*pp_list) > 0)
	{
		ll_node_t *currNode = ll_remove_nth_node(*pp_list, 0);
		free_node(currNode);
	}

	free(*pp_list);
	*pp_list = NULL;
}

server_memory *init_server_memory()
{
	/* TODO 1 */
	server_memory *server = malloc(sizeof(*server));
	server->hmax = HMAX;
	server->buckets = malloc(HMAX * sizeof(linked_list_t *));
	for (unsigned int i = 0; i < HMAX; i++)
	{
		server->buckets[i] = ll_create(sizeof(info *));
		server->buckets[i]->head = NULL;
	}
	server->size = 0;
	return server;
}

char *server_retrieve(server_memory *server, char *key)
{
	/*TODO 3*/
	if (server == NULL || server->size == 0)
		return NULL;

	unsigned int index = server->hash_function(key) % server->hmax;

	ll_node_t *curr_node = server->buckets[index]->head;
	info *current_info;

	if (curr_node == NULL)
		return NULL;

	while (curr_node != NULL)
	{
		current_info = (info *)curr_node->data;
		char *curr_key = current_info->key;

		if (strcmp(curr_key, key) == 0)
		{
			return current_info->value;
		}
		curr_node = curr_node->next;
	}
	return NULL;
}

int server_has_key(server_memory *server, char *key)
{
	return server_retrieve(server, key) != NULL;
}

void server_store(server_memory *server, char *key, char *value)
{
	/* TODO 2 */
	if (server == NULL)
	{
		printf("dc serverul e null\n");
		return;
	}

	unsigned int index = server->hash_function(key) % server->hmax;
	info *new = calloc(1, sizeof(info));
	new->key = calloc((strlen(key) + 1), sizeof(char));
	new->value = calloc((strlen(value) + 1), sizeof(char));

	memcpy(new->key, key, strlen(key));
	memcpy(new->value, value, strlen(value));


	ll_add_nth_node(server->buckets[index], server->buckets[index]->size, &new);
	server->size++;
}

void server_remove(server_memory *server, char *key)
{
	/* TODO 4 */

	unsigned int index, position = 0;

	index = server->hash_function(key) % server->hmax;
	ll_node_t *it = server->buckets[index]->head;

	while (it != NULL)
	{
		char *curr_key = ((struct info *)(it->data))->key;
		if (strcmp(curr_key, key) == 0)
		{
			ll_node_t *node = ll_remove_nth_node(server->buckets[index],
												 position);
			free_node(node);
			server->size--;
			return;
		}
		position++;
		it = it->next;
	}
}

void free_server_memory(server_memory *server)
{
	/* TODO 5 */
	for (unsigned int i = 0; i < server->hmax; i++)
	{
		if (server->buckets[i] != NULL)
			ll_free(&server->buckets[i]);
	}
	free(server->buckets);
	free(server);
	server = NULL;
}