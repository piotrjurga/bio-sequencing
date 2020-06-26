//
// notes:
// should we sometimes allow revisiting nodes? for the instances with repetitions
//

#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <assert.h>
#include <algorithm>

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

s32 optimize_and_score(Edge *candidate, Node *graph, s32 onct_length,
                       s32 max_solution_length, s32 node_count) {
    s32 oncts_visited = 0;
    s32 total_length = onct_length;
    s32 current = 0;
    u8 visited[1024] = {};
    while (candidate[current].cost) {
        visited[current] = true;
        oncts_visited++;
        Edge edge = candidate[current];
        bool too_long = edge.cost + total_length > max_solution_length;
        bool next_visited = visited[edge.next];
        if (too_long || next_visited) {
            // try to find a legal edge
            s32 i;
            for (i = 0; i < graph[current].edge_count; i++) {
                edge = graph[current].edges[i];
                too_long = edge.cost + total_length > max_solution_length;
                next_visited = visited[edge.next];
                if (!too_long && !next_visited) {
                    candidate[current] = edge;
                    break;
                }
            }
            if (i == graph[current].edge_count) {
                break;
            }
        }
        total_length += edge.cost;
        current = edge.next;
        assert(current >= 0 && current < node_count);
    }
    return oncts_visited;
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
        Node *node = &graph[node_i];
        node->edges = edges + total_edges;
        node->edge_count = 0;
        for (s32 dest_i = 0; dest_i < node_count; dest_i++) {
            if (node_i == dest_i) continue;
            if (node_i == 48 && dest_i == 18) {
                s32 test = 0;
                test++;
            }
            s32 overlap = get_overlap(dict[node_i], dict[dest_i], onct_length);
            //if (overlap > 0)
            {
                Edge e;
                e.next = dest_i;
                e.cost = onct_length - overlap;
                node->edges[node->edge_count++] = e;
            }
        }

        // sort edges in the node by cost
        qsort(node->edges, node->edge_count,
              sizeof(Edge), stb_intcmp(offsetof(Edge, cost)));
        total_edges += graph[node_i].edge_count;
    }

#if 0
    puts("GRAPH:");
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

    s32 population = 1000;
    s32 parent_count = population / 10;
    s32 candidate_size = node_count * sizeof(Edge);
    void *candidates = calloc(population + parent_count, candidate_size);
    void *parents = candidates + population * candidate_size;

    struct Score {s32 oncts; s32 index;};
    Score *scores = (Score *)malloc(sizeof(Score) * population);

    for (s32 candidate_index = 0;
         candidate_index < population;
         candidate_index++)
    {
        Edge *candidate = (Edge *)(candidates + candidate_index*candidate_size);
        for (s32 i = 0; i < node_count; i++) {
            s32 edge_count = graph[i].edge_count;
            if (edge_count) {
                //s32 chosen_edge = stb_rand() % edge_count;
                //s32 chosen_edge = stb_rand() % stb_min(2, edge_count);
                s32 chosen_edge = 0;
                candidate[i] = graph[i].edges[chosen_edge];
            }
        }
        Score s;
        s.oncts = optimize_and_score(candidate, graph, onct_length,
                                     max_solution_length, node_count);
        s.index = candidate_index;
        scores[candidate_index] = s;
    }

    //
    // evolve
    //

//#define CPPSORT
#ifdef CPPSORT
    struct {
        bool operator()(Score a, Score b) const {
            return a.oncts > b.oncts;
        }
    } scoreCompare;
#endif

    s32 generations = 300;
    for (s32 gen_index = 0; gen_index < generations; gen_index++) {
#ifdef CPPSORT
        std::sort(scores, scores+population, scoreCompare);
#else
        qsort(scores, population, sizeof(Score), stb_intcmprev(0));
#endif

        // save the best solutions for breeding
        for (s32 parent_i = 0; parent_i < parent_count; parent_i++) {
            Edge *parent_slot = (Edge *)(parents + parent_i*candidate_size);
            s32 old_index = scores[parent_i].index;
            scores[parent_i].index = parent_i;
            Edge *parent = (Edge *)(candidates + old_index*candidate_size);
            memcpy(parent_slot, parent, candidate_size);
        }
        // move them to the beggining of the population
        memcpy(candidates, parents, parent_count * candidate_size);

        // set the rest of the population to modified versions of parents
        for (s32 candidate_index = parent_count;
             candidate_index < population;
             candidate_index++)
        {
            Edge *candidate = (Edge *)(candidates + candidate_index*candidate_size);
            s32 parent_i = candidate_index % parent_count;
            Edge *parent = (Edge *)(candidates + parent_i*candidate_size);
            memcpy(candidate, parent, candidate_size);
            // mutate
            s32 node_to_mutate = stb_rand() % node_count;
            Node node = graph[node_to_mutate];
            s32 new_edge = (s32)(stb_frand() * stb_frand() * node.edge_count);
            //s32 new_edge = stb_rand() % node.edge_count;
            candidate[node_to_mutate] = node.edges[new_edge];
/*
            s32 score = score_candidate(candidate, onct_length,
                                        max_solution_length, node_count);
*/
            s32 score = optimize_and_score(candidate, graph, onct_length,
                                           max_solution_length, node_count);
            scores[candidate_index].oncts = score;
            scores[candidate_index].index = candidate_index;
        }
    }

    qsort(scores, population, sizeof(Score), stb_intcmprev(0));
    s32 best_score = scores[0].oncts;
    s32 best_index = scores[0].index;
    s32 optimal_score = stb_min(dict_size, original_oncts);

    printf("best = %d (%f%%)\n", best_score, 100*(double)best_score / (double)optimal_score);

    Edge *best_candidate = (Edge *)malloc(candidate_size);
    memcpy(best_candidate,
           candidates + best_index*candidate_size,
           candidate_size);
    free(graph);
    free(candidates);
    free(scores);
    return best_candidate;
}

int main() {
    stb_srand(time(0));

#if 0
    //char *path = "test.txt";
    char *path = "Instances/RandomNegativeErrors/9.200-80.txt";
    s32 original_oncts = 200;
    char **dict;
    s32 dict_size;
    dict = stb_stringfile(path, &dict_size);

    Edge *best = solve(dict, dict_size, original_oncts);
    (void)best;
    //print_path(dict, best, 209);
    //print_solution(dict, best, 209);

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
