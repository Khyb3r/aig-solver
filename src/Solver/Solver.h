#ifndef DPLLSOLVER_H
#define DPLLSOLVER_H

#include "../aig.h"
#include <vector>
#include <queue>

class Solver {
public:
    Node** nodes_list;
    std::vector<Node*> input_nodes;
    std::vector<Edge> output_nodes;
    std::vector<Node*> assignment_list;
    std::vector<int> decision_level_boundary_indexes;
    std::queue<Node*> propagation_queue;
    int actual_total_nodes = 0;

    int solver_decision_level = 0;
    bool conflict = false;
    bool SAT = true;
    int nodes_list_size = 0;
    Node* conflict_node = nullptr;


    // === METRICS FOR ANALYSIS ===
    // Unsigned to prevent overflow for very large benchmark instances
    unsigned int solver_conflicts = 0;
    unsigned int solver_propagations = 0;
    unsigned int solver_decisions = 0;

    // Metrics for preprocess specific analysis
    int solver_total_inputs = 0;
    int solver_assigned_inputs = 0;
    int solver_true_forced = 0;
    int solver_false_forced = 0;
    double solver_ands_forced = 0;


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
    void compute_active_inputs();

    // Node Choice Heuristics
    Node* choose_first_unassigned();
    Node* choose_largest_fanout();
    Node* and_gate_importance_scoring();
    void calculate_nodes_depth_from_output();

    // Branching heuristic
    inline void always_branch_true(Node*);
    inline void always_branch_false(Node*);
    inline void phase_saving(Node*);
    int output_propogator_scorer(Node*, Assignment);

    void print_stats();
    void print_preprocess_stats();
    void print_compute_active_inputs_stats();
    inline void total_valid_nodes_count();

    // Mimicking VSIDS effect but on pure DPLL
    double solver_activity_increment = 1.0;
    double solver_activity_decay = 0.95;
    Node* vsids_choose_node();
    void inline bump_activity(Node*);
    void inline decay_every_N_conflicts();
    double vsids_and_gate_scoring(Node*);
};

#endif //DPLLSOLVER_H