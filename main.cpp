#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <assert.h>

#define STB_DEFINE
#include "stb.h"

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct Edge {
    s32 next; // connected node index
    s32 cost; // how many nonoverlapping oncts are added to the solution
};

struct Node {
    Edge *edges;
    s32 edge_count;
};

s32 get_overlap(char *a, char *b, s32 onct_length) {
    for (s32 overlap = onct_length-1;
         overlap > 0;
         overlap--)
    {
        char *x = a + onct_length - overlap;
        s32 i = 0;
        while (i < overlap && x[i] == b[i]) {
            i++;
        }
        if (i == overlap) return overlap;
    }

    return 0;
}

s32 score_candidate(Edge *candidate, s32 onct_length, s32 max_solution_length, s32 node_count) {
    s32 oncts_visited = 0;
    s32 total_length = onct_length;
    s32 current = 0;
    u8 visited[1024] = {};
    while (candidate[current].cost) {
        if (visited[current]) break;
        visited[current] = true;
        oncts_visited++;
#if 0
        printf("visiting %d:\n", current);
        printf("\tnext %d\n", candidate[current].next);
        printf("\tcost %d\n", candidate[current].cost);
#endif
        if (candidate[current].cost + total_length > max_solution_length) {
            return oncts_visited;
        }
        total_length += candidate[current].cost;
        current = candidate[current].next;
        assert(current >= 0 && current < node_count);
    }
    return oncts_visited;
}

void print_path(char **dict, Edge *candidate, s32 max_solution_length) {
    s32 total_length = strlen(dict[0]);
    s32 current = 0;
    u8 visited[1024] = {};
    while (candidate[current].cost) {
        if (visited[current]) break;
        visited[current] = true;

        printf("%s\n", dict[current]);
        if (candidate[current].cost + total_length > max_solution_length) {
            break;
        }
        total_length += candidate[current].cost;
        current = candidate[current].next;
    }
    puts("");
    return;
}

void print_solution(char **dict, Edge *candidate, s32 max_solution_length) {
    s32 onct_length = strlen(dict[0]);
    s32 total_length = onct_length;
    s32 current = 0;
    u8 visited[1024] = {};
    s32 last_cost = onct_length;
    while (candidate[current].cost) {
        if (visited[current]) break;
        visited[current] = true;

        printf("%s", dict[current] + (onct_length-last_cost));
        if (candidate[current].cost + total_length > max_solution_length) {
            break;
        }
        total_length += candidate[current].cost;
        last_cost = candidate[current].cost;
        current = candidate[current].next;
    }
    puts("");
    return;
}

Edge * solve(char **dict, s32 dict_size, s32 original_oncts) {
    s32 onct_length = strlen(dict[0]);
    s32 max_solution_length = original_oncts + onct_length - 1;

    //
    // build the main graph
    //

    s32 node_count = dict_size;

    // edges are allocated after the nodes
    s32 nodes_mem_size = sizeof(Node) * node_count;
    s32 edges_mem_size = sizeof(Edge) * node_count * (node_count - 1);

    Node *graph = (Node *)malloc(nodes_mem_size + edges_mem_size);
    Edge *edges = (Edge *)(graph + node_count);
    s32 total_edges = 0;

    for (s32 node_i = 0; node_i < node_count; node_i++) {
        graph[node_i].edges = edges + total_edges;
        graph[node_i].edge_count = 0;
        for (s32 dest_i = 0; dest_i < node_count; dest_i++) {
            if (node_i == dest_i) continue;
            if (node_i == 48 && dest_i == 18) {
                s32 test = 0;
                test++;
            }
            s32 overlap = get_overlap(dict[node_i], dict[dest_i], onct_length);
            if (overlap > 0) {
                Edge e;
                e.next = dest_i;
                e.cost = onct_length - overlap;
                graph[node_i].edges[graph[node_i].edge_count++] = e;
            }
        }
        total_edges += graph[node_i].edge_count;
    }

#if 0
    for (s32 i = 0; i < node_count; i++) {
        Node n = graph[i];
        printf("%d:", i);
        for (s32 j = 0; j < n.edge_count; j++) {
            printf("\t(%d,%d)", n.edges[j].next, n.edges[j].cost);
        }
        puts("");
    }
#endif

    //
    // create population
    //

    s32 population = 100;
    s32 candidate_size = node_count * sizeof(Edge);
    void *candidates = calloc(population, candidate_size);

    for (s32 candidate_index = 0;
         candidate_index < population;
         candidate_index++)
    {
        Edge *candidate = (Edge *)(candidates + candidate_index*candidate_size);
        for (s32 i = 0; i < node_count; i++) {
            s32 edge_count = graph[i].edge_count;
            if (edge_count) {
                // TODO(piotr): choose edges with lower cost more often
                s32 chosen_edge = rand() % edge_count;
                candidate[i] = graph[i].edges[chosen_edge];
            }
        }
    }

    //
    // evolve
    //

    // TODO(piotr): just sort the thing and always return the first candidate as best
    s32 best_index = -1;
    s32 best_score = 0;

    // TODO(piotr): many generations
    for (s32 candidate_index = 0;
         candidate_index < population;
         candidate_index++)
    {
        Edge *candidate = (Edge *)(candidates + candidate_index*candidate_size);
        s32 score = score_candidate(candidate, onct_length,
                                    max_solution_length, node_count);
        if (score > best_score) {
            best_score = score;
            best_index = candidate_index;
        }
    }
    printf("best = %d\n", best_score);

    Edge *best_candidate = (Edge *)malloc(candidate_size);
    memcpy(best_candidate,
           candidates + best_index*candidate_size,
           candidate_size);
    free(graph);
    free(candidates);
    return best_candidate;
}

int main() {

#if 1
    //char *path = "test.txt";
    char *path = "Instances/RandomNegativeErrors/9.200-40.txt";
    s32 original_oncts = 200;
    char **dict;
    s32 dict_size;
    dict = stb_stringfile(path, &dict_size);

    Edge *best = solve(dict, dict_size, original_oncts);
    print_path(dict, best, 209);
    print_solution(dict, best, 209);

    return 0;
#endif


    char *problem_dirs[] = {"Instances/PositiveErrorsWithDistortions",
                            "Instances/RandomNegativeErrors",
                            "Instances/RandomPositiveErrors",
                            "Instances/RepetitionNegativeErrors"};
    char *problem_dir_path = problem_dirs[1];
    DIR *problem_dir = opendir(problem_dir_path);

    struct dirent *dir_entry;
    while (dir_entry = readdir(problem_dir)) {
        if (dir_entry->d_type != DT_REG) continue;

        s32 original_oncts = 0;
        sscanf(dir_entry->d_name, "%*d.%d", &original_oncts);

        char path[1024] = {};
        stb_snprintf(path, 1024, "%s/%s", problem_dir_path, dir_entry->d_name);

        char **dict;
        s32 dict_size;
        dict = stb_stringfile(path, &dict_size);
        s32 onct_length = strlen(dict[0]);
        s32 max_solution_length = original_oncts + onct_length - 1;

        Edge *best = solve(dict, dict_size, original_oncts);
        s32 result = score_candidate(best, onct_length, max_solution_length, dict_size);
        printf("%s\t%d\n", dir_entry->d_name, result);
    }
        
    return 0;
}
