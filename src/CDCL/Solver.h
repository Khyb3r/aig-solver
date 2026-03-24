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

    int solver_decision_level = 0;
    bool conflict = false;
    bool SAT = true;
    int nodes_list_size = 0;

    Node* conflict_node = nullptr;

    std::vector<Clause> learned_cubes;
    Clause build_conflict_cube();
    bool backjump_from_cube(Clause&);
    bool check_relevance(const Edge&);

    void preprocess();
    bool run();
    // Introduce heuristics or some other method of deciding whether Node should be T/F
    // Currently always assigns True
    Node* decide_node();

    void propogate(Node*);

    void propogate_forward_helper(Node*);

    void propogate_backward_helper(Node*);

    inline void backtrack();

    bool conflict_handler();

};

#endif //SOLVER_H
