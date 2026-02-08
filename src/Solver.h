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
    std::queue<Node*> propogation_queue;


    void propogate_forward(Node& a, Node& b);
    void propogate_backward(Node &a);
};



#endif //SOLVER_H
