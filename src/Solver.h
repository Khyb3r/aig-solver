#ifndef SOLVER_H
#define SOLVER_H
#include <queue>
#include <vector>
#include "aig.h"

class Solver {
public:
    Node** nodes_list;
    std::vector<Edge> output_nodes;
    std::vector<Node*> assignment_list;
    std::vector<int> decision_level_boundary_indexes;
    std::queue<Node*> propagation_queue;
    std::vector<Clause> clause_db;
    int solver_decision_level = 0;
    bool conflict = false;
    bool SAT = true;
    int nodes_list_size = 0;

    void preprocess();
    void run(int);
    // Introduce heuristics or some other method of deciding whether Node should be T/F
    // Currently always assigns True
    Node* decide_node(int);

    void propogate(Node*);

    void propogate_forward_helper(Node*);

    void propogate_backward_helper(Node*);

    void conflict_handler(Node*);

    void backjump(unsigned int);

    bool satisfiable_check(int);

    void check_clauses(Node* n);
};

#endif //SOLVER_H
