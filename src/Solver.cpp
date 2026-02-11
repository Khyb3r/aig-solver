#include "Solver.h"

void Solver::run() {
    // choose a node to decide
    // make a decision
    // add to assignment list, add to propogation queue
    // keep propogating until queue is empty or conflict found -> repeat step above and this again and again

    bool satisfiable = false;
    bool conflict = false;
    while (!satisfiable) {
        Node* curr_node = choose_node_to_decide();
        decide_node_assignment(curr_node);
        add_to_assignment_list(curr_node);
        update_propogation_queue(curr_node);
        while (!propagation_queue.empty() || !conflict) {
            propogate(curr_node);
        }
    }

}

Node* Solver::choose_node_to_decide() {
    Node* ptr = nodes_list[0];
    while (ptr != nullptr && ptr->assignment != Assignment::UNASSIGNED) {
        ptr++;
    }
    return ptr;
}

// CHANGE: Currently always decides True
void Solver::decide_node_assignment(Node* a) {
    if (a->assignment == Assignment::UNASSIGNED) a->assignment = Assignment::TRUE;
}

void Solver::add_to_assignment_list(Node *a) {
    assignment_list.push_back(a);
}

void Solver::move_to_next_decision_level() {
    decision_level_boundary_indexes.push_back(assignment_list.size() - 1);
    //decision_level_boundary_indexes.push_back(decision_level_boundary_indexes[decision_level_boundary_indexes.size() - 1] + 1);
}

void Solver::update_propogation_queue(Node* a) {
    propagation_queue.push(a);
}

void Solver::propogate(Node* a) {
    propogated_node_reason_assignment(a);
    if (a->type == NodeType::AND) {
        // backprop first
        propogate_backward_helper(a);
        propogate_forward_helper(a);
        return;
    }
    propogate_forward_helper(a);

}

void Solver::propogate_forward_helper(Node *a) {
    for (int i = 0; i < a->output_nodes.size(); i++) {
        assignment_list.push_back(a->output_nodes[i]);
    }
}

void Solver::propogate_backward_helper(Node *a) {
    Edge& first_input = a->input_nodes[0];
    Edge& second_input = a->input_nodes[1];

    if (a->assignment == Assignment::TRUE) {
        if (first_input.inverted)
            first_input.node->assignment = Assignment::FALSE;
        else
            first_input.node->assignment = Assignment::TRUE;

        if (second_input.inverted)
            second_input.node->assignment = Assignment::FALSE;
        else
            second_input.node->assignment = Assignment::TRUE;

    }
    else if (a->assignment == Assignment::FALSE) {
        // Decide which should be false or true can be one or the other

    }
}

void Solver::propogated_node_reason_assignment(Node* a) {
    // pre popping store prev node as reason
    assignment_list.push_back(a);
    // Add reason
    a->reason = assignment_list[assignment_list.size() - 2];
    propagation_queue.push(a);
}