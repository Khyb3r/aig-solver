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
        Node& n = a->output_nodes[i];
        // If already assigned skip
        if (n.assignment != Assignment::UNASSIGNED) continue;
        // Output node is an AND node
        if (n.type == NodeType::AND) {
            // Inputs to our output node, one of them will be us
            Edge& curr_node_edge = n.input_nodes[0];
            Edge& other_input = n.input_nodes[1];

            if (n.input_nodes[1].node == a) {
                curr_node_edge = n.input_nodes[1];
                other_input = n.input_nodes[0];
            }

            // If we are FALSE, Propagate out that our AND gate is false
            if ((curr_node_edge.inverted && a->assignment == Assignment::TRUE) ||
                !curr_node_edge.inverted && a->assignment == Assignment::FALSE) {
                n.assignment = Assignment::FALSE;
            }

            // Propagate if other input is FALSE, so AND is a FALSE
            if (other_input.node->assignment == Assignment::TRUE &&
                other_input.inverted) {
                n.assignment = Assignment::FALSE;
                assignment_list.push_back(other_input.node);
                propagation_queue.push(other_input.node);
            }
            else if (other_input.node->assignment == Assignment::FALSE &&
                !other_input.inverted) {
                n.assignment = Assignment::FALSE;
                assignment_list.push_back(other_input.node);
                propagation_queue.push(other_input.node);
            }

            // add to assignments list
            assignment_list.push_back(&n);
            propagation_queue.push(&n);

        }

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
