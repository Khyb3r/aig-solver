#include "Solver.h"

void Solver::propogate_backward(Node &a) {
    if (a.type != NodeType::AND) return;

    if (a.assignment == Assignment::TRUE) {
        a.input_nodes->node[0].assignment = Assignment::TRUE;
        a.input_nodes->node[1].assignment = Assignment::TRUE;
    }
    else if (a.assignment == Assignment::FALSE) {
        // try either first input node as True/False or second input node
    }
}

void Solver::propogate_forward(Node &a, Node &b) {

}

