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
        while (!propagation_queue.empty() && !conflict) {
            Node& n = *propagation_queue.front();
            propagation_queue.pop();
            propogate(&n);
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
}

void Solver::update_propogation_queue(Node* a) {
    propagation_queue.push(a);
}

void Solver::propogate(Node* a) {
    if (a->type == NodeType::AND) {
        // backprop and forward prop
        propogate_backward_helper(a);
        propogate_forward_helper(a);
        return;
    }
    // forward prop only
    propogate_forward_helper(a);

}

void Solver::propogate_forward_helper(Node *a) {
    for (int i = 0; i < a->output_nodes.size(); i++) {
        // Current node output
        Node& output = a->output_nodes[i];
        // Output node is an AND node
        if (output.type == NodeType::AND) {
            // Inputs to our output node, one of them will be us
            Edge* curr_node_edge = &output.input_nodes[0];
            Edge* other_input = &output.input_nodes[1];

            if (output.input_nodes[1].node == a) {
                curr_node_edge = &output.input_nodes[1];
                other_input = &output.input_nodes[0];
            }


            if (((a->assignment == Assignment::FALSE && !curr_node_edge->inverted) ||
                (a->assignment == Assignment::TRUE && curr_node_edge->inverted))) {
                if (output.assignment == Assignment::UNASSIGNED) {
                    output.assignment = Assignment::FALSE;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);
                }
                else if (output.assignment == Assignment::TRUE) {} // conflict
                else {} // do nothing
            }

            else if (((a->assignment == Assignment::TRUE && !curr_node_edge->inverted) ||
                (a->assignment == Assignment::FALSE && curr_node_edge->inverted)) &&
                ((other_input->node->assignment == Assignment::TRUE && !other_input->inverted)  ||
                (other_input->node->assignment == Assignment::FALSE && other_input->inverted))) {

                if (output.assignment == Assignment::UNASSIGNED) {
                    output.assignment = Assignment::TRUE;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);
                }
                else if (output.assignment == Assignment::FALSE) {} // conflict
                else {} // do nothing
            }
        }
    }
}

void Solver::propogate_backward_helper(Node *curr) {
    if (curr->type != NodeType::AND) return;

    Edge& first_input = curr->input_nodes[0];
    Edge& second_input = curr->input_nodes[1];

    // AND being TRUE
    if (curr->assignment == Assignment::TRUE) {
        // Either is Unassigned
        if (first_input.node->assignment == Assignment::UNASSIGNED || second_input.node->assignment == Assignment::UNASSIGNED) {
            if (first_input.node->assignment == Assignment::UNASSIGNED) {
                if (first_input.inverted) first_input.node->assignment = Assignment::FALSE;
                else first_input.node->assignment = Assignment::TRUE;
                propagation_queue.push(first_input.node);
            }

            if (second_input.node->assignment == Assignment::UNASSIGNED) {
                if (second_input.inverted) second_input.node->assignment = Assignment::FALSE;
                else second_input.node->assignment = Assignment::TRUE;
            }
        }

        // First Input is FALSE
        else if (first_input.node->assignment == Assignment::TRUE && first_input.inverted ||
            first_input.node->assignment == Assignment::FALSE && !first_input.inverted) {
            // conflict
        }

        // Second input is FALSE
        else if (second_input.node->assignment == Assignment::FALSE && !second_input.inverted ||
            second_input.node->assignment == Assignment::TRUE && second_input.inverted) {
            // conflict
        }

        // First Input is TRUE
        else if ((first_input.node->assignment == Assignment::TRUE && !first_input.inverted) ||
            (first_input.node->assignment == Assignment::FALSE && first_input.inverted)) {

            if (second_input.node->assignment == Assignment::UNASSIGNED) {
                if (second_input.inverted) second_input.node->assignment = Assignment::FALSE;
                else second_input.node->assignment = Assignment::TRUE;
                assignment_list.push_back(second_input.node);
                propagation_queue.push(second_input.node);
            }

            else if ((second_input.node->assignment == Assignment::FALSE && second_input.inverted) ||
                (second_input.node->assignment == Assignment::FALSE && !second_input.inverted)) {
                // conflict
            }
        }
        // Second Input is TRUE
        else if ((second_input.node->assignment == Assignment::TRUE && !second_input.inverted) ||
            (second_input.node->assignment == Assignment::FALSE && second_input.inverted)) {

            if (first_input.node->assignment == Assignment::UNASSIGNED) {
                if (first_input.inverted) first_input.node->assignment = Assignment::FALSE;
                else first_input.node->assignment = Assignment::TRUE;
                assignment_list.push_back(first_input.node);
                propagation_queue.push(first_input.node);
            }

            else if ((first_input.node->assignment == Assignment::FALSE && first_input.inverted) ||
                (first_input.node->assignment == Assignment::FALSE && !first_input.inverted)) {
                // conflict
            }
        }
    }

    else if (curr->assignment == Assignment::FALSE) {
        if (((first_input.node->assignment == Assignment::TRUE && !first_input.inverted) ||
            (first_input.node->assignment == Assignment::FALSE && first_input.inverted)) &&
            (second_input.node->assignment == Assignment::TRUE && !second_input.inverted) ||
            (second_input.node->assignment == Assignment::FALSE && second_input.inverted)) {
            // conflict both are TRUE when FALSE
        }

        // First True, Second should be False
        else if ((first_input.node->assignment == Assignment::TRUE && !first_input.inverted) ||
            (first_input.node->assignment == Assignment::FALSE && first_input.inverted)) {
                if (second_input.node->assignment == Assignment::UNASSIGNED) {
                    if (second_input.inverted) second_input.node->assignment = Assignment::TRUE;
                    else second_input.node->assignment = Assignment::FALSE;
                    assignment_list.push_back(second_input.node);
                    propagation_queue.push(second_input.node);
                }
        }
        // Second True, First should be False
        else if ((second_input.node->assignment == Assignment::TRUE && !second_input.inverted) ||
            (second_input.node->assignment == Assignment::FALSE && second_input.inverted)) {
                if (first_input.node->assignment == Assignment::UNASSIGNED) {
                    if (first_input.inverted) first_input.node->assignment = Assignment::TRUE;
                    else first_input.node->assignment = Assignment::FALSE;
                    assignment_list.push_back(first_input.node);
                    propagation_queue.push(first_input.node);
                }
        }
    }
}

void Solver::propogated_node_reason_assignment(Node* a) {
    // pre popping store prev node as reason
    assignment_list.push_back(a);
    // Add reason
    a->reason = assignment_list[assignment_list.size() - 2];
    propagation_queue.push(a);
}
