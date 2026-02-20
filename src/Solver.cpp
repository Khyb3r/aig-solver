#include "Solver.h"
#include <set>
#include <stack>

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

            // A (FALSE) AND ? = OUTPUT (FALSE)
            if (((a->assignment == Assignment::FALSE && !curr_node_edge->inverted) ||
                (a->assignment == Assignment::TRUE && curr_node_edge->inverted))) {
                if (output.assignment == Assignment::UNASSIGNED) {
                    output.assignment = Assignment::FALSE;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);
                    output.reason = a;
                }
                else if (output.assignment == Assignment::TRUE) {
                    // conflict
                    conflict_handler(&output);
                }
            }

            // A (TRUE) AND B (TRUE) = OUT (TRUE)
            else if (((a->assignment == Assignment::TRUE && !curr_node_edge->inverted) ||
                (a->assignment == Assignment::FALSE && curr_node_edge->inverted)) &&
                ((other_input->node->assignment == Assignment::TRUE && !other_input->inverted)  ||
                (other_input->node->assignment == Assignment::FALSE && other_input->inverted))) {

                if (output.assignment == Assignment::UNASSIGNED) {
                    output.assignment = Assignment::TRUE;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);
                    output.reason = a;
                    output.reason_two = other_input->node;
                }
                else if (output.assignment == Assignment::FALSE) {
                    // conflict
                    conflict_handler(&output);
                }
            }
        }
    }
}

void Solver::propogate_backward_helper(Node *curr) {
    if (curr->type != NodeType::AND) return;

    Edge& first_input = curr->input_nodes[0];
    Edge& second_input = curr->input_nodes[1];

    bool first_input_assigned = first_input.node->assignment != Assignment::UNASSIGNED;
    bool second_input_assigned = second_input.node->assignment != Assignment::UNASSIGNED;
    bool first_input_true = false;
    bool second_input_true = false;
    if (first_input.node->assignment != Assignment::UNASSIGNED)
        first_input_true = (first_input.node->assignment == Assignment::TRUE) ^ first_input.inverted;
    if (second_input.node->assignment != Assignment::UNASSIGNED)
        second_input_true = (second_input.node->assignment == Assignment::TRUE) ^ second_input.inverted;


    // AND being TRUE
    // OUT (TRUE) = A (TRUE) AND B (TRUE)
    if (curr->assignment == Assignment::TRUE) {
        // If First Unassigned set to True otherwise conflict if set to False
        if (first_input.node->assignment == Assignment::UNASSIGNED) {
            first_input.node->assignment = first_input.inverted ? Assignment::FALSE : Assignment::TRUE;
            assignment_list.push_back(first_input.node);
            propagation_queue.push(first_input.node);
            first_input.node->reason = curr;
        }

        else if (!first_input_true && first_input_assigned) {
            // conflict
            conflict_handler(first_input.node);
        }

        // If Second Unassigned set to True otherwise conflict if set to True
        if (second_input.node->assignment == Assignment::UNASSIGNED) {
            second_input.node->assignment = second_input.inverted ? Assignment::FALSE : Assignment::TRUE;
            assignment_list.push_back(second_input.node);
            propagation_queue.push(second_input.node);
            second_input.node->reason = curr;
        }
        else if (!second_input_true && second_input_assigned) {
            // conflict
            conflict_handler(second_input.node);
        }
    }


    // OUT (FALSE) = A (T/F) AND B (T/F)
    else if (curr->assignment == Assignment::FALSE) {
        //  If one gate is True make other False
        if (first_input.node->assignment != Assignment::UNASSIGNED && second_input.node->assignment != Assignment::UNASSIGNED) {
            if (first_input_true && first_input_assigned && second_input_true && second_input_assigned) {
                // conflict
                conflict_handler(curr);
            }
        }

        if (first_input.node->assignment == Assignment::UNASSIGNED) {
            if (second_input_true && second_input_assigned) {
                first_input.node->assignment = first_input.inverted ? Assignment::TRUE : Assignment::FALSE;
                assignment_list.push_back(first_input.node);
                propagation_queue.push(first_input.node);
                first_input.node->reason = curr;
                first_input.node->reason_two = second_input.node;
            }
        }

        if (second_input.node->assignment == Assignment::UNASSIGNED) {
            if (first_input_true && first_input_assigned) {
                second_input.node->assignment = second_input.inverted ? Assignment::TRUE : Assignment::FALSE;
                assignment_list.push_back(second_input.node);
                propagation_queue.push(second_input.node);
                second_input.node->reason = curr;
                second_input.node->reason_two = first_input.node;
            }
        }
    }
}

void Solver::conflict_handler(Node* conflict_node) {
    // Move up graph from current node using reason pointers till last decision (no reason)
    std::set<Node*> visited_nodes;
    std::stack<Node*> nodes_stack;
    unsigned int current_nodes_counted = (conflict_node->decision_level == decision_level_boundary_indexes.back()) ? 1 : 0;
    nodes_stack.push(conflict_node);
    Node* uip = nullptr;
    do {
        Node* curr = nodes_stack.top();
        nodes_stack.pop();
        visited_nodes.insert(curr);
        uip = curr;
        if (curr->decision_level == decision_level_boundary_indexes.back()) {
            current_nodes_counted--;
            if (current_nodes_counted == 1 && uip == nullptr) uip = curr;
        }

        if (curr->reason != nullptr && !visited_nodes.contains(curr->reason)
            && curr->reason->decision_level == conflict_node->decision_level) {
            nodes_stack.push(curr->reason);
            current_nodes_counted++;
        }
        if (curr->reason_two != nullptr && !visited_nodes.contains(curr->reason_two)
            && curr->reason_two->decision_level == conflict_node->decision_level) {
            nodes_stack.push(curr->reason_two);
            current_nodes_counted++;
        }
    }
    while (current_nodes_counted > 1 && !nodes_stack.empty());

    // Detect conflict
    // Create Implication Graph
    // Analyse conflict  (Cut/UIP)
    // Generate learned constraint and Store in some manner not to do this again?
    // Get the backjump level
    // Undo assignments /  Purge assigment list back to last decision level --> Backjump
    // Add learned clause
    // Resume propagation
}
