#ifndef SOLVER_H
#define SOLVER_H
#include <queue>
#include <vector>
#include "aig.h"

class Solver {
public:
    Solver();

    Node** nodes_list;
    std::vector<Edge> output_nodes;
    std::vector<Node*> assignment_list;
    std::vector<int> decision_level_boundary_indexes;
    std::queue<Node*> propagation_queue;

    void run();

    Node* choose_node_to_decide();

    // Introduce heuristics or some other method of deciding whether Node should be T/F
    // Currently always assigns True
    void decide_node_assignment(Node*);

    void add_to_assignment_list(Node* a);

    void move_to_next_decision_level();

    void update_propogation_queue(Node*);

    void propogate_forward_helper(Node*);
    void propogate_backward_helper(Node*);
    void propogate(Node*);

    void conflict_handler();

};



#endif //SOLVER_H
