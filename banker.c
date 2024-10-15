#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_RESOURCES 3
#define MAX_PROCESSES 5

int available[MAX_RESOURCES];
int maximum[MAX_PROCESSES][MAX_RESOURCES];
int allocation[MAX_PROCESSES][MAX_RESOURCES];
int need[MAX_PROCESSES][MAX_RESOURCES];

pthread_mutex_t lock;

void print_state() {
    printf("\nCurrent State:\n");
    printf("Available: ");
    for (int i = 0; i < MAX_RESOURCES; i++) {
        printf("%d ", available[i]);
    }
    printf("\n");

    printf("Allocation:\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        for (int j = 0; j < MAX_RESOURCES; j++) {
            printf("%d ", allocation[i][j]);
        }
        printf("\n");
    }

    printf("Need:\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        for (int j = 0; j < MAX_RESOURCES; j++) {
            printf("%d ", need[i][j]);
        }
        printf("\n");
    }
}

bool is_safe_state() {
    int work[MAX_RESOURCES];
    bool finish[MAX_PROCESSES] = {false};

    for (int i = 0; i < MAX_RESOURCES; i++)
        work[i] = available[i];

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (finish[i] == false) {
            bool can_allocate = true;
            for (int j = 0; j < MAX_RESOURCES; j++) {
                if (need[i][j] > work[j]) {
                    can_allocate = false;
                    break;
                }
            }

            if (can_allocate) {
                for (int j = 0; j < MAX_RESOURCES; j++)
                    work[j] += allocation[i][j];
                finish[i] = true;
                i = -1;
            }
        }
    }

    for (int i = 0; i < MAX_PROCESSES; i++)
        if (finish[i] == false)
            return false;

    return true;
}

bool request_resources(int process_id, int request[]) {
    pthread_mutex_lock(&lock);

    printf("Process %d requests: ", process_id);
    for (int i = 0; i < MAX_RESOURCES; i++) {
        printf("%d ", request[i]);
    }
    printf("\n");

    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (request[i] > need[process_id][i]) {
            printf("Process %d: Request exceeds declared maximum need\n", process_id);
            pthread_mutex_unlock(&lock);
            return false;
        }
    }

    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (request[i] > available[i]) {
            printf("Process %d: Resources not available\n", process_id);
            pthread_mutex_unlock(&lock);
            return false;
        }
    }

    for (int i = 0; i < MAX_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[process_id][i] += request[i];
        need[process_id][i] -= request[i];
    }

    if (is_safe_state()) {
        printf("Process %d: Resource allocation successful\n", process_id);
        print_state();
        pthread_mutex_unlock(&lock);
        return true;
    } else {
        printf("Process %d: Resource allocation would lead to unsafe state. Rolling back.\n", process_id);
        for (int i = 0; i < MAX_RESOURCES; i++) {
            available[i] += request[i];
            allocation[process_id][i] -= request[i];
            need[process_id][i] += request[i];
        }
        print_state();
        pthread_mutex_unlock(&lock);
        return false;
    }
}

void* process_thread(void* arg) {
    int process_id = *(int*)arg;
    int request[MAX_RESOURCES];

    sleep(process_id);  // Simulate different arrival times

    for (int i = 0; i < MAX_RESOURCES; i++) {
        request[i] = rand() % (need[process_id][i] + 1);
    }

    request_resources(process_id, request);

    return NULL;
}

int main() {
    pthread_t threads[MAX_PROCESSES];
    int process_ids[MAX_PROCESSES];

    srand(time(NULL));

    pthread_mutex_init(&lock, NULL);

    // Initialize available resources
    available[0] = 10;
    available[1] = 5;
    available[2] = 7;

    // Initialize maximum need and calculate initial need
    for (int i = 0; i < MAX_PROCESSES; i++) {
        for (int j = 0; j < MAX_RESOURCES; j++) {
            maximum[i][j] = rand() % 5;
            allocation[i][j] = rand() % (maximum[i][j] + 1);
            need[i][j] = maximum[i][j] - allocation[i][j];
            available[j] -= allocation[i][j];
        }
    }

    printf("Initial State:\n");
    print_state();

    // Create threads
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_ids[i] = i;
        pthread_create(&threads[i], NULL, process_thread, &process_ids[i]);
    }

    // Wait for threads to finish
    for (int i = 0; i < MAX_PROCESSES; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    printf("Final State:\n");
    print_state();

    return 0;
}