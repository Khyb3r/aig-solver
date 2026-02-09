#include "Solver.h"

void Solver::run() {
}

Node* Solver::choose_node_to_decide() {
    Node* ptr = nodes_list[0];
    while (ptr != nullptr && ptr->assignment != Assignment::UNASSIGNED) {
        ptr++;
    }
    return ptr;
}

// CHANGE: Currently always decides True
int Solver::decide_node_assignment(Node* a) {
    if (a->assignment == Assignment::UNASSIGNED) return -1;
    a->assignment = Assignment::TRUE;
    return 0;
}

void Solver::add_to_assignment_list(Node *a) {
    assignment_list.push_back(a);
}

void Solver::update_current_decision_level_index() {
    decision_level_boundary_indexes.at(decision_level_boundary_indexes.size() - 1)++;
}

void Solver::move_to_next_decision_level() {
    decision_level_boundary_indexes.push_back(decision_level_boundary_indexes.at(decision_level_boundary_indexes.size() - 1) + 1);
}

void Solver::update_propogation_queue(Node* a) {
    propogation_queue.push(a);
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

void Solver::propogated_node_reason_assignment(Node* a) {
    assignment_list.push_back(a);
    update_current_decision_level_index();
    // Add reason
    a->reason = assignment_list[assignment_list.size() - 2];
    propogation_queue.push(a);
}



void Solver::propogate_forward_helper(Node *a) {

}

void Solver::propogate_backward_helper(Node *a) {
    Edge& first_input = a->input_nodes[0];
    Edge& second_input = a->input_nodes[1];

    if (a->assignment == Assignment::TRUE) {
        first_input.node->assignment == Assignment::TRUE;
        second_input.node->assignment == Assignment::TRUE;
    }
    else if (a->assignment == Assignment::FALSE) {
        // Decide which should be false or true can be one or the other
    }
}

