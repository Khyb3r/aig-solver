#ifndef DPLLSOLVER_H
#define DPLLSOLVER_H

#include "../aig.h"
#include <vector>
#include <queue>

class DPLLSolver {
public:
    Node** nodes_list;
    std::vector<Edge> output_nodes;
    std::vector<Node*> assignment_list;
    std::vector<int> decision_level_boundary_indexes;
    std::queue<Node*> propagation_queue;


    int solver_decision_level = 0;
    bool conflict = false;
    bool SAT = true;
    int nodes_list_size = 0;
    Node* conflict_node = nullptr;


    // Global variables to track how different heuristics are improving things
    int solver_conflicts = 0;
    int solver_propagations = 0;
    int solver_decisions = 0;

    void preprocess();
    bool run();


    Node* decide_node();

    // Node Choice Heuristics Methods
    Node* choose_first_unassigned();
    Node* choose_largest_fanout();
    Node* and_gate_importance_scoring();

    void propogate(Node*);

    void propogate_forward_helper(Node*);

    void propogate_backward_helper(Node*);

    bool conflict_handler();

    inline void backtrack();

    bool satisfiable_check();

    void check_clauses(Node* n);
};

#endif //DPLLSOLVER_H
