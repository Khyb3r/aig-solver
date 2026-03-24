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


    // === METRICS FOR ANALYSIS ===
    int solver_conflicts = 0;
    int solver_propagations = 0;
    int solver_decisions = 0;

    // === CORE SOLVER METHODS ===
    void preprocess();
    bool run();
    Node* decide_node();
    void propogate(Node*);
    void propogate_forward_helper(Node*);
    void propogate_backward_helper(Node*);
    bool conflict_handler();
    inline void backtrack();


    // === HEURISTICS RELATED METHODS ===

    // Preprocessing heuristic to cut search space heavily
    void failed_literal_probing();
    void undo_probing(size_t);

    // Node Choice Heuristics
    Node* choose_first_unassigned();
    Node* choose_largest_fanout();
    Node* and_gate_importance_scoring();

    // Branching heuristic
    inline void always_branch_true(Node*);
    inline void phase_saving(Node*);

    // Mimicking VSIDS effect but on pure DPLL
    double solver_activity_increment = 1.0;
    double solver_activity_decay = 0.95;
    Node* vsids_choose_node();
    void inline bump_activity(Node*);
    void inline decay_every_N_conflicts();
    double vsids_and_gate_scoring(Node*);
};

#endif //DPLLSOLVER_H
