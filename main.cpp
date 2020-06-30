// fast
#if 0
#define POPULATION  512
#define GENERATIONS 8
#define PARENTS     (POPULATION/2)
//#define BREED
#define SPARSE_GRAPH
#endif

// normal
#if 1
#define POPULATION  (1024*2)
#define GENERATIONS (1024*2)
#define PARENTS     (POPULATION/2)
#define BREED
#define SPARSE_GRAPH
#endif

#define MUTATIONS   2
#define PARALLEL
//#define SINGLE_TEST

// for the moment optimizing the graph actually makes the results worse
// it may be caused by storing the merged nodes in candidates
#define OPTIMIZE_GRAPH

// hypothetical alternate breeding schemes:
// find a place in the parent_a where it would be appropriate
// to try something else and only then switch to parent_b
// - probably optimize and score at the same time, cuz it's easy
// - and mutate at the same time too?
// - could try some heuristic like split before a costly edge
// - could find the split at random - better diversity
// - could litterally dfs the crap out of the parents combined graph


#include <stdio.h>
#include <stdint.h>
#include <assert.h>

// platform independent filesystem interface
#ifdef _MSC_VER
#include "dirent.h"
#else
#include <dirent.h>
#endif

#define STB_DEFINE
#define STB_NO_REGISTRY
#include "stb.h"

// platform independent high precision timer
#define SOKOL_IMPL
#include "sokol_time.h"

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
            if (i == graph[current].edge_count) { // legal edge not found
                break;
            }
        }
        total_length += edge.cost;
        current = edge.next;
        assert(current >= 0 && current < node_count);
    }
    return oncts_visited;
}

s32 mutate_optimize_and_score(Edge *candidate, Node *graph, s32 onct_length,
                       s32 max_solution_length, s32 node_count, s32 mutate_after) {
    s32 oncts_visited = 0;
    s32 total_length = onct_length;
    s32 current = 0;
    u8 visited[1024] = {};
    bool mutation_done = false;
    while (candidate[current].cost) {
        visited[current] = true;
        oncts_visited++;
        if (!mutation_done && oncts_visited >= mutate_after && graph[current].edge_count > 1) {
            double rand_v = stb_frand();
            s32 new_edge_i = (s32)(rand_v * rand_v * graph[current].edge_count);
            Edge new_edge = graph[current].edges[new_edge_i];
            candidate[current] = new_edge;
            mutation_done = true;
        }
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
            if (i == graph[current].edge_count) { // legal edge not found
                break;
            }
        }
        total_length += edge.cost;
        current = edge.next;
        assert(current >= 0 && current < node_count);
    }
    return oncts_visited;
}

void optimize_graph(Node *graph, s32 node_count) {
    s32 total_optimized = 0;
    // find optimal edges connecting [i] to [j]
    s32 optimal_edges[1024][1024] = {};
    for (s32 node_i = 0; node_i < node_count; node_i++) {
        Node node = graph[node_i];
        for (s32 edge_i = 0; edge_i < node.edge_count; edge_i++) {
            Edge e = node.edges[edge_i];
            if (e.cost != 1) break;
            optimal_edges[node_i][e.next] += 1;
        }
    }
#if 0
    for (s32 i = 0; i < node_count; i++) {
        for (s32 j = 0; j < node_count; j++) {
            printf("%d;", optimal_edges[i][j]);
        }
        puts("");
    }
#endif
    // search for nodes to merge
    for (s32 node_i = 0; node_i < node_count; node_i++) {
        s32 row_sum = 0;
        s32 dest_j = -1;
        for (s32 j = 0; j < node_count; j++) {
            row_sum += optimal_edges[node_i][j];
            if (optimal_edges[node_i][j] > 0) {
                dest_j = j;
            }
            if (row_sum > 1) break;
        }
        if (row_sum != 1) continue;
        s32 col_sum = 0;
        for (s32 i = 0; i < node_count; i++) {
            col_sum += optimal_edges[i][dest_j];
            if (col_sum > 1) break;
        }
        if (col_sum != 1) continue;
        // at this point we know the nodes can be merged
        total_optimized++;
        // delete all suboptimal edges from source node
        {
            Node *node = &graph[node_i];
            node->edges[0].next = dest_j;
            node->edges[0].cost = 1;
            node->edge_count = 1;
        }

        // delete all suboptimal edges into dest node
        for (s32 i = 0; i < node_count; i++) {
            Node *node = &graph[i];
            if (i == node_i || i == dest_j) continue;
            for (s32 edge_i = 0; edge_i < node->edge_count; edge_i++) {
                if (node->edges[edge_i].next != dest_j) continue;

                s32 remaining_edges = node->edge_count - edge_i - 1;
                memmove(&node->edges[edge_i],
                        &node->edges[edge_i+1],
                        remaining_edges * sizeof(Edge));
                node->edge_count--;
                break;
            }
        }
    }

    printf("merged nodes %d times\n", total_optimized);
}

Edge * solve(char **dict, s32 dict_size, s32 original_oncts, double *percent_score) {
    s32 onct_length = strlen(dict[0]);
    s32 max_solution_length = original_oncts + onct_length - 1;

    //
    // build the graph
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
#ifdef SPARSE_GRAPH
            if (overlap > 0)
#endif
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

#ifdef OPTIMIZE_GRAPH
    optimize_graph(graph, node_count);
#endif

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

    s32 population = POPULATION;
    s32 parent_count = PARENTS;
    s32 candidate_size = node_count * sizeof(Edge);
    u8 *candidates = (u8 *)calloc(population + parent_count, candidate_size);
    u8 *parents = candidates + population * candidate_size;

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

    s32 generations = GENERATIONS;
    for (s32 gen_index = 0; gen_index < generations; gen_index++) {
#if 0
        printf("gen %d\n", gen_index);
        // print all candidates
        for (s32 i = 0; i < population; i++) {
            Edge *candidate = (Edge *)(candidates + i*candidate_size);
            for (s32 j = 0; j < node_count; j++) {
                Edge e = candidate[j];
                Node n = graph[j];
                for (s32 k = 0; k < n.edge_count; k++) {
                    if (n.edges[k].next == e.next) {
                        printf("%d;", k);
                        break;
                    }
                }
            }
            puts("");
        }
#endif

        qsort(scores, population, sizeof(Score), stb_intcmprev(0));

#ifdef PARALLEL
#pragma omp parallel
#endif
        {
            // save the best solutions for breeding
#ifdef PARALLEL
#pragma omp for
#endif
            for (s32 parent_i = 0; parent_i < parent_count; parent_i++) {
                Edge *parent_slot = (Edge *)(parents + parent_i*candidate_size);
                s32 old_index = scores[parent_i].index;
                scores[parent_i].index = parent_i;
                Edge *parent = (Edge *)(candidates + old_index*candidate_size);
                memcpy(parent_slot, parent, candidate_size);
            }
            // move them to the beggining of the population
#ifdef PARALLEL
#pragma omp single
#endif
            memcpy(candidates, parents, parent_count * candidate_size);

            // set the rest of the population to modified versions of parents
#ifdef PARALLEL
#pragma omp for
#endif
            for (s32 candidate_index = parent_count;
                    candidate_index < population;
                    candidate_index++)
            {
#ifdef BREED
                s32 parent_a_i = candidate_index % parent_count;
                s32 parent_b_i = stb_rand() % parent_count;
                s32 split = stb_rand() % node_count;
                //s32 split = node_count/2;
                s32 size_a = split * sizeof(Edge);
                //s32 size_a = (node_count/2) * sizeof(Edge);
                s32 size_b = candidate_size - size_a;
                Edge *parent_a = (Edge *)(candidates + parent_a_i*candidate_size);
                Edge *parent_b = (Edge *)(candidates + parent_b_i*candidate_size + size_a);
                Edge *candidate   = (Edge *)(candidates + candidate_index*candidate_size);
                Edge *candidate_b = (Edge *)(candidates + candidate_index*candidate_size + size_a);
                // move the first half of the genes from the first parent
                memcpy(candidate, parent_a, size_a);
                // move the second half of the genes from the second parent
                memcpy(candidate_b, parent_b, size_b);
#else
                Edge *candidate = (Edge *)(candidates + candidate_index*candidate_size);
                s32 parent_i = candidate_index % parent_count;
                Edge *parent = (Edge *)(candidates + parent_i*candidate_size);
                memcpy(candidate, parent, candidate_size);
#endif
                //
                // mutate
                //
#if 0
                for (s32 i = 0; i < MUTATIONS; i++) {
                    s32 node_to_mutate = stb_rand() % node_count;
                    Node node = graph[node_to_mutate];
                    double rand_v = stb_frand();
                    s32 new_edge = (s32)(rand_v * rand_v * node.edge_count);
                    candidate[node_to_mutate] = node.edges[new_edge];
                }
                s32 score = optimize_and_score(candidate, graph, onct_length,
                        max_solution_length, node_count);
#else
                s32 mutate_after = stb_rand() % scores[0].oncts;
                s32 score = mutate_optimize_and_score(candidate, graph, onct_length,
                        max_solution_length, node_count, mutate_after);
#endif
                scores[candidate_index].oncts = score;
                scores[candidate_index].index = candidate_index;
            }
        }
    }

    qsort(scores, population, sizeof(Score), stb_intcmprev(0));
    s32 best_score = scores[0].oncts;
    s32 best_index = scores[0].index;
    s32 optimal_score = stb_min(dict_size, original_oncts);
    *percent_score = 100*(double)best_score / (double)optimal_score;
    //printf("best = %d (%f%%)\n", best_score, *percent_score);

    Edge *best_candidate = (Edge *)malloc(candidate_size);
    memcpy(best_candidate,
           candidates + best_index*candidate_size,
           candidate_size);
    free(graph);
    free(candidates);
    free(scores);
    return best_candidate;
}

int main(int argc, char **argv) {
    stb_srand(time(0));
    stm_setup();

#ifdef SINGLE_TEST
    char *path = "test.txt";
    //char *path = "Instances/RandomNegativeErrors/9.200-40.txt";
    s32 original_oncts = 10;
    char **dict;
    s32 dict_size;
    dict = stb_stringfile(path, &dict_size);

    double percent_score = 0;
    u64 start_time = stm_now();
    Edge *best = solve(dict, dict_size, original_oncts, &percent_score);
    (void)best;
    double elapsed = stm_ms(stm_since(start_time));
    printf("9.200-40.txt\t%f%%\t%fms\n", percent_score, elapsed);
    //print_path(dict, best, 209);
    //print_solution(dict, best, 209);

    return 0;
#endif

    assert(argc > 1);
    char *problem_dirs[] = {"Instances/PositiveErrorsWithDistortions",
                            "Instances/RandomNegativeErrors",
                            "Instances/RandomPositiveErrors",
                            "Instances/RepetitionNegativeErrors"};
    char *problem_dir_path = problem_dirs[atoi(argv[1])];
    DIR *problem_dir = opendir(problem_dir_path);

    double *scores = 0;
    double *times = 0;

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
        //s32 onct_length = strlen(dict[0]);
        //s32 max_solution_length = original_oncts + onct_length - 1;
        double percent_score = 0;

        u64 start_time = stm_now();
        Edge *best = solve(dict, dict_size, original_oncts, &percent_score);
        (void)best;
        double elapsed = stm_ms(stm_since(start_time));
        stb_arr_push(scores, percent_score);
        stb_arr_push(times, elapsed);
        printf("%s\t%f%%\t%fms\n", dir_entry->d_name, percent_score, elapsed);
        //s32 result = score_candidate(best, onct_length, max_solution_length, dict_size);
    }

    // calculate average score and time
    double sum_score = 0;
    for (s32 i = 0; i < stb_arr_len(scores); i++) {
        sum_score += scores[i];
    }
    double sum_time = 0;
    for (s32 i = 0; i < stb_arr_len(times); i++) {
        sum_time += times[i];
    }
    double average_score = sum_score / stb_arr_len(scores);
    double average_time = sum_time / stb_arr_len(times);
    puts("______________________________________________");
    printf("average\t\t%f%%\t%fms\n", average_score, average_time);
        
    return 0;
}
