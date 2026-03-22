#ifndef SOLVER_H
#define SOLVER_H
#include <queue>
#include <vector>
#include "../cdcl-aig.h"

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

    Node* conflict_node = nullptr;
    Node* conflict_reason = nullptr;
    Node* conflict_reason_two = nullptr;

    void preprocess();
    bool run();
    // Introduce heuristics or some other method of deciding whether Node should be T/F
    // Currently always assigns True
    Node* decide_node();

    void propogate(Node*);

    void propogate_forward_helper(Node*);

    void propogate_backward_helper(Node*);

    inline void backtrack(int);

    void clause_propogation();

    inline void backtrack();

    bool conflict_handler();

    // -- THINGS TO ADD/CHANGE --
    void backjump();
    bool cdcl_conflict_handler();


};

#endif //SOLVER_H
