#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <stdbool.h>
#include <time.h>

#define MAX_FUNCTION_BODY 2048
#define BUFFER_SIZE 8192
#define MAX_FUNCTIONS 100
#define MAX_MUTEX_OPS 20
#define MAX_NODES 1000

struct MutexOperation {
    char mutex_name[128];
    char operation[10];
    int line_number;
    int pthread_join_count;
};

struct Function {
    char function_name[MAX_FUNCTION_BODY];
    char function_body[MAX_FUNCTION_BODY];
    int pthread_join_count;
    int encountered_line;
    struct MutexOperation mutex_operations[MAX_MUTEX_OPS];
    int mutex_op_count;
};

#define HASH_MAP_SIZE 400

typedef struct Node {
    char *key;
    int value;
    struct Node *next;
} Node;

typedef struct HashMap {
    Node *buckets[HASH_MAP_SIZE];
} HashMap;

HashMap* createHashMap() {
    HashMap *hashMap = malloc(sizeof(HashMap));
    for (int i = 0; i < HASH_MAP_SIZE; ++i) {
        hashMap->buckets[i] = NULL;
    }
    return hashMap;
}

unsigned int hash(char *key) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c;
    return hash % HASH_MAP_SIZE;
}

void hashMapInsert(HashMap *hashMap, char *key, int value) {
    unsigned int bucketIndex = hash(key);
    Node *newNode = malloc(sizeof(Node));
    newNode->key = strdup(key);
    newNode->value = value;
    newNode->next = hashMap->buckets[bucketIndex];
    hashMap->buckets[bucketIndex] = newNode;
}

int hashMapGet(HashMap *hashMap, char *key) {
    unsigned int bucketIndex = hash(key);
    Node *node = hashMap->buckets[bucketIndex];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }
    return -1;
}

void printHashMap(HashMap *hashMap) {
    for (int i = 0; i < HASH_MAP_SIZE; ++i) {
        Node *node = hashMap->buckets[i];
        if (node) {
            printf("Bucket %d:\n", i);
            while (node) {
                printf("  Key: %s, Value: %d\n", node->key, node->value);
                node = node->next;
            }
        }
    }
}

void replaceSemicolons(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == ';') {
            str[i] = '_';
        }
    }
}

void removeSpaces(char *str) {
    replaceSemicolons(str);
    int len = strlen(str);
    int i, j;
    for (i = 0, j = 0; i < len; i++) {
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

int graph[MAX_NODES][MAX_NODES];
int curent_nod_number = 0 ;
char* resource_name [MAX_NODES];
int cycle_value_found = -1;
int value_found = -1;
bool found_mutex = true;

void AddMutexToFunction(struct Function *function, int function_number, HashMap *myHashMap) {
    char *line = strtok(function->function_body, "\n");
    int line_number = 1;
    char critical_section[128];
    strcpy(critical_section, function->function_name);
    int number_mutexes_passed = 0;
    char number_to_string[10] = "";
    int node_value;
    if (found_mutex == true) {
        sprintf(number_to_string, "%d", number_mutexes_passed);
        strcat(critical_section, number_to_string); 
        node_value = hashMapGet(myHashMap, critical_section);
        if (node_value== -1) {
            hashMapInsert(myHashMap, critical_section, curent_nod_number++);
            node_value = hashMapGet(myHashMap, critical_section);
        }
        resource_name[node_value]=strdup(critical_section);
        removeSpaces(resource_name[node_value]);
        found_mutex = false;
    }
    while (line != NULL) {
        if (strstr(line, "pthread_mutex_lock") || strstr(line, "pthread_mutex_unlock")) {
            if (function->mutex_op_count < MAX_MUTEX_OPS) {
                strcpy(function->mutex_operations[function->mutex_op_count].operation, line);
                function->mutex_op_count++;
                number_mutexes_passed++;
                char critical_section2[128];
                strcpy(critical_section2, function->mutex_operations[function->mutex_op_count - 1].operation);
                sprintf(number_to_string, "%d", function->pthread_join_count);
                strcat(critical_section2, number_to_string);
                int node_value2 = hashMapGet(myHashMap, critical_section2);
                if(node_value2 == -1) {   
                    hashMapInsert(myHashMap, critical_section2, curent_nod_number++);
                    node_value2 = hashMapGet(myHashMap, critical_section2);
                }
                graph[node_value][node_value2] = 1;
                resource_name[node_value2]=strdup(critical_section2);
                removeSpaces(resource_name[node_value2]);
                sprintf(number_to_string, "%d", number_mutexes_passed);
                strcpy(critical_section, function->function_name);
                char critical_section3[128];
                strcpy(critical_section3, critical_section);
                strcat(critical_section3, number_to_string);
                int node_value3 = hashMapGet(myHashMap, critical_section3);
                if(node_value3 == -1) {   
                    hashMapInsert(myHashMap, critical_section3, curent_nod_number++);
                    node_value3 = hashMapGet(myHashMap, critical_section3);
                }
                graph[node_value2][node_value3] = 1;
                node_value = node_value3;
                resource_name[node_value3]=strdup(critical_section3);
                removeSpaces(resource_name[node_value3]);
                found_mutex = true;
            } else {
                printf("Warning: Maximum mutex operations count reached.\n");
            }
        }
        line = strtok(NULL, "\n");
        line_number++;
    }
}

void addMutexOperationsToFunctions(struct Function *functions, int function_count, HashMap* myHashMap) {
    for(int i = 0; i<function_count-1; ++i) {
        AddMutexToFunction(&functions[i], i, myHashMap);
    }
}

void printMainFunctionBody(char *function_body, struct Function *functions, int function_count) {
    char *line = strtok(function_body, "\n");
    int line_number = 1;
    int pthread_joins_before = 0;
    while (line != NULL) {
        if (strstr(line, "pthread_join") != NULL) {
            pthread_joins_before++;
        }
        for (int i = 0; i < function_count; i++) {
            if (strstr(line, functions[i].function_name) != NULL && strcmp(line, functions[i].function_name) != 0) {
                if (functions[i].encountered_line == 0) {
                    functions[i].encountered_line = line_number;
                    functions[i].pthread_join_count = pthread_joins_before;
                }
            }
        }
        line = strtok(NULL, "\n");
        line_number++;
    }
}

void DFS(int node, int visited[], clock_t begin) {
    if(cycle_value_found != -1) return;
    visited[node] = 1;
    for (int i = 0; i < curent_nod_number && value_found==-1; ++i) {
        if (graph[node][i]) {
            if (!visited[i]) {
                DFS(i, visited, begin);
            } else {
                printf("DEADLOCK FOUND IN:\n");
                clock_t end = clock();
                double time_spent = (double)(end - begin) * 1000 / CLOCKS_PER_SEC;
                printf("%f seconds\n",time_spent);
                printf("%s\n", resource_name[node]);
                cycle_value_found = i;
                return;
            }
            if(cycle_value_found != -1 && value_found==-1) {
                printf("%s\n", resource_name[node]);
                if(cycle_value_found == node)
                value_found = cycle_value_found;
                return;
            }
        }
    }
}

void detectDeadlock(clock_t begin) {
    int visited[curent_nod_number];
    int deadlockFound = 0;
    memset(visited, 0, sizeof(visited));
    for (int i = 0; i < curent_nod_number && cycle_value_found==-1; ++i) {
        if (!visited[i]) {
            DFS(i, visited, begin);
        }
    }
    if(cycle_value_found==-1)
    printf("No deadlock detected\n");
}

int main(int argc, char *argv[]) {
    clock_t begin = clock();

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    char input_text[BUFFER_SIZE] = {0};
    char *input_ptr = input_text;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), file)) {
        int len = strlen(buffer);
        if (input_ptr - input_text + len < BUFFER_SIZE) {
            strcpy(input_ptr, buffer);
            input_ptr += len;
        } else {
            fprintf(stderr, "File too large for buffer\n");
            fclose(file);
            return 1;
        }
    }
    fclose(file);

    const char *function_pattern = "\\bmain\\b|([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\([^)]*\\)\\s*\\{([^}]*)\\}";
    const char *pthread_create_pattern = "\\s*pthread_create\\s*\\([^,]*,\\s*[^,]*,\\s*([a-zA-Z_][a-zA-Z0-9_]*)";
    regex_t function_regex;
    regex_t pthread_create_regex;
    regmatch_t matches[3];

    HashMap *myHashMap = createHashMap();

    if (regcomp(&function_regex, function_pattern, REG_EXTENDED)) {
        fprintf(stderr, "Could not compile function regex\n");
        return 1;
    }

    if (regcomp(&pthread_create_regex, pthread_create_pattern, REG_EXTENDED)) {
        fprintf(stderr, "Could not compile pthread_create regex\n");
        return 1;
    }

    char *p = input_text;
    struct Function functions[MAX_FUNCTIONS];
    int function_count = 0;

    while (regexec(&function_regex, p, 3, matches, 0) == 0) {
        if (matches[1].rm_so != -1) {
            int start = matches[1].rm_so + (p - input_text);
            int end = matches[1].rm_eo + (p - input_text);
            strncpy(functions[function_count].function_name, input_text + start, end - start);
            functions[function_count].function_name[end - start] = '\0';
            start = matches[2].rm_so + (p - input_text);
            end = matches[2].rm_eo + (p - input_text);
            strncpy(functions[function_count].function_body, input_text + start, end - start);
            functions[function_count].function_body[end - start] = '\0';
            functions[function_count].pthread_join_count = 0;
            functions[function_count].encountered_line = 0;
            function_count++;
        }
        p += matches[0].rm_eo;
    }

    p = input_text;
    while (regexec(&pthread_create_regex, p, 2, matches, 0) == 0) {
        int start = matches[1].rm_so + (p - input_text);
        int end = matches[1].rm_eo + (p - input_text);
        p += matches[0].rm_eo;
    }

    regfree(&function_regex);
    regfree(&pthread_create_regex);

    struct Function *main_function = NULL;
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].function_name, "main") == 0) {
            main_function = &functions[i];
            break;
        }
    }

    if (main_function == NULL) {
        fprintf(stderr, "main function not found\n");
        return 1;
    }

    printMainFunctionBody(main_function->function_body, functions, function_count);

    addMutexOperationsToFunctions(functions, function_count, myHashMap);
    detectDeadlock(begin);
    char program_path[100] = "./";
    strcat(program_path, argv[1]);
    program_path[strlen(program_path)-2]='\0';
    printf("RUNNING THE PROGRAM EXECUTABLE IF EXISTS...\n");
    int result = system(program_path);
    clock_t end_time = clock();
    double execution_time = (double)(end_time - begin) * 1000 / CLOCKS_PER_SEC;

    if (result == 0) {
        printf("Program being tested execution time: %.2f seconds\n", execution_time);
    }
    return 0;
}
