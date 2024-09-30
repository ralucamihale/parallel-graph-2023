// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t mutexGraph;

/* TODO: Define graph task argument. */

int stop() {
	for (unsigned int i = 0; i < graph->num_nodes; i++) {
		if (graph->visited[i] == PROCESSING) {
			return 0;
		}
	}
	return 1;
}

static void aux(void *p) {
	os_node_t *node = graph->nodes[*(unsigned int *)p];

	pthread_mutex_lock(&mutexGraph);
	sum += node->info;
	pthread_mutex_unlock(&mutexGraph);

	graph->visited[*(unsigned int *)p] = DONE;

	for (unsigned int i = 0; i < node->num_neighbours; i++) {
		pthread_mutex_lock(&mutexGraph);
		if (graph->visited[node->neighbours[i]] == NOT_VISITED) {
			printf("plm\n");
			graph->visited[node->neighbours[i]] = PROCESSING;
			void *p = malloc(sizeof(unsigned int));
			*(unsigned int *)p = graph->nodes[node->neighbours[i]]->id;
			os_task_t *t = create_task(aux, p, free);
			enqueue_task(tp, t);
		}
		pthread_mutex_unlock(&mutexGraph);
	}
}

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */
	
	void *p = malloc(sizeof(unsigned int));
	*(unsigned int *)p = idx;
	os_task_t *t = create_task(aux, (void *)p, free);
	enqueue_task(tp, t);
	pthread_mutex_lock(&mutexGraph);
	tp->flag = 1;
	pthread_mutex_unlock(&mutexGraph);
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	/* TODO: Initialize graph synchronization mechanisms. */

	pthread_mutex_init(&mutexGraph, NULL);
	tp = create_threadpool(NUM_THREADS);
	tp->stop = stop;
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);
	pthread_mutex_destroy(&mutexGraph);

	printf("%d", sum);

	return 0;
}