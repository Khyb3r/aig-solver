#include "DPLLSolver.h"

#include <iostream>

void DPLLSolver::preprocess() {
    for (int i = 0; i < output_nodes.size(); i++) {
        Edge& output = output_nodes[i];
        if (output.node == nullptr) {
            if (!output.inverted) {
                conflict = true;
                return;
            }
            continue;
        }

        Assignment required = output.inverted ? Assignment::FALSE : Assignment::TRUE;

        if (output.node->assignment == Assignment::UNASSIGNED) {
            output.node->assignment = required;
            output.node->decision_level = 0;
            assignment_list.push_back(output.node);
            propagation_queue.push(output.node);

            // Phase saving
            if (output.node->type == NodeType::INPUT)
                output.node->saved_phase = (required == Assignment::TRUE) ? SavedPhase::TRUE : SavedPhase::FALSE;
        }
        else if (output.node->assignment != required) {
            conflict = true;
            return;
        }
    }
    while (!propagation_queue.empty() && !conflict) {
        propogate(propagation_queue.front());
        propagation_queue.pop();
    }

    // After propagation, print how many inputs are still unassigned
    int unassigned_inputs = 0;
    int assigned_inputs = 0;
    for (int i = 0; i < nodes_list_size; i++) {
        if (nodes_list[i] != nullptr && nodes_list[i]->type == NodeType::INPUT) {
            if (nodes_list[i]->assignment == Assignment::UNASSIGNED) unassigned_inputs++;
            else assigned_inputs++;
        }
    }
}

bool DPLLSolver::run() {
    preprocess();
    if (conflict) {
        std::cout << "UNSAT" << '\n';
        return false;
    }
    solver_decision_level = 1;
    while (true) {
        // First drain any pending propagation (from a flip or fresh start)
        while (!propagation_queue.empty() && !conflict) {
            propogate(propagation_queue.front());
            propagation_queue.pop();
        }

        if (conflict) {
            if (!conflict_handler()) {
                std::cout << "UNSAT\n";
                return false;
            }
            continue; // go back and propagate the flipped decision
        }

        Node* unassigned_node = decide_node();
        if (unassigned_node == nullptr) {
            std::cout << "SAT\n";
            return true;
        }

        decision_level_boundary_indexes.push_back(assignment_list.size());
        assignment_list.push_back(unassigned_node);
        propagation_queue.push(unassigned_node);
        solver_decision_level++;
    }
}

Node* DPLLSolver::decide_node() {
    solver_decisions++;

    // Change decision heuristic here
    Node* chosen_node = and_gate_importance_scoring();

    if (chosen_node != nullptr) {
        //always_branch_true(chosen_node);
        phase_saving(chosen_node);
    }
    return chosen_node;
}

inline void DPLLSolver::always_branch_true(Node* chosen_node) {
    chosen_node->assignment = Assignment::TRUE;
    chosen_node->decision_level = solver_decision_level;
}

void DPLLSolver::phase_saving(Node* chosen_node) {
    if (chosen_node->saved_phase != SavedPhase::NONE) {
        if (chosen_node->saved_phase == SavedPhase::TRUE) {
            chosen_node->assignment = Assignment::TRUE;
            chosen_node->decision_level = solver_decision_level;
        }
        else {
            chosen_node->assignment = Assignment::FALSE;
            chosen_node->decision_level = solver_decision_level;
        }
    }
    // Always branch True currently
    else {
        chosen_node->assignment = Assignment::TRUE;
        chosen_node->decision_level = solver_decision_level;
        chosen_node->saved_phase = SavedPhase::TRUE;
    }
}


Node *DPLLSolver::choose_first_unassigned() {
    for (int i = 0; i < nodes_list_size; i++) {
        Node* node = nodes_list[i];
        if (node != nullptr && node->type == NodeType::INPUT && node->assignment == Assignment::UNASSIGNED) {
            return node;
        }
    }
    return nullptr;
}


// Choose the node with the largest fanout early so that larger amount of the search space is explored earlier
Node* DPLLSolver::choose_largest_fanout() {
    size_t max_fanout_length = 0;
    Node* chosen_node = nullptr;
    for (int i = 0; i < nodes_list_size; i++) {
        Node* node = nodes_list[i];
        if (node != nullptr && node->type == NodeType::INPUT &&
            node->assignment == Assignment::UNASSIGNED && node->output_nodes.size() > max_fanout_length) {
            max_fanout_length = node->output_nodes.size();
            chosen_node = node;
        }
    }
    return chosen_node;
}

Node* DPLLSolver::and_gate_importance_scoring() {
    // Go through all input nodes and give them a scoring
    int best_score = 0;
    Node* best_node = nullptr;
    for (int i = 0; i < nodes_list_size; i++) {
        int score = 0;
        Node* node = nodes_list[i];
        if (node != nullptr && node->type == NodeType::INPUT && node->assignment == Assignment::UNASSIGNED) {
            for (int j = 0; j < node->output_nodes.size(); j++) {
                Edge* curr_node_edge = &node->output_nodes[j]->input_nodes[0];
                Edge* other_node_edge = &node->output_nodes[j]->input_nodes[1];
                if (node == other_node_edge->node) {
                    other_node_edge = &node->output_nodes[j]->input_nodes[0];
                    curr_node_edge = &node->output_nodes[j]->input_nodes[1];
                }
                bool true_value = other_node_edge->node == nullptr ? other_node_edge->inverted :
                                  (other_node_edge->node->assignment == Assignment::TRUE) ^ other_node_edge->inverted;

                if (other_node_edge->node == nullptr) {
                    if (true_value) score += 10;
                }
                else {
                    if (true_value) score += 10;
                    else if (other_node_edge->node->assignment == Assignment::UNASSIGNED) score += 2;
                }
            }
        }
        if (score > best_score) {
            best_score = score;
            best_node = node;
        }
    }
    return best_node;
}




void DPLLSolver::propogate(Node *a) {
    solver_propagations++;
    if (a->type == NodeType::AND) {
        // backprop and forward prop
        propogate_backward_helper(a);
        propogate_forward_helper(a);
        return;
    }
    // forward prop only
    propogate_forward_helper(a);
}

void DPLLSolver::propogate_forward_helper(Node *a) {
    for (int i = 0; i < a->output_nodes.size(); i++) {
        // Current node output
        Node& output = *a->output_nodes[i];
        // Output node is an AND node
        if (output.type == NodeType::AND) {
            Edge* curr_node_edge = &output.input_nodes[0];
            Edge* other_input    = &output.input_nodes[1];

            if (output.input_nodes[1].node == a) {
                curr_node_edge = &output.input_nodes[1];
                other_input    = &output.input_nodes[0];
            }

            bool other_is_const = (other_input->node == nullptr);

            // A (FALSE) AND ? = OUTPUT (FALSE)
            if ((a->assignment == Assignment::FALSE && !curr_node_edge->inverted) ||
                (a->assignment == Assignment::TRUE  &&  curr_node_edge->inverted)) {
                if (output.assignment == Assignment::UNASSIGNED) {
                    output.assignment = Assignment::FALSE;
                    output.decision_level = solver_decision_level;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);

                }
                else if (output.assignment == Assignment::TRUE) {
                    // conflict
                    conflict = true;
                    return;
                }
            }

            // A (TRUE) AND B (TRUE) = OUT (TRUE)
            else if ((a->assignment == Assignment::TRUE  && !curr_node_edge->inverted) ||
                     (a->assignment == Assignment::FALSE &&  curr_node_edge->inverted)) {

                bool other_is_true = other_is_const
                    ? other_input->inverted
                    : (other_input->node->assignment == Assignment::TRUE) ^ other_input->inverted;
                bool other_assigned = other_is_const
                    || other_input->node->assignment != Assignment::UNASSIGNED;

                if (other_assigned && other_is_true) {
                    if (output.assignment == Assignment::UNASSIGNED) {
                        output.assignment = Assignment::TRUE;
                        output.decision_level = solver_decision_level;
                        assignment_list.push_back(&output);
                        propagation_queue.push(&output);

                    }
                    else if (output.assignment == Assignment::FALSE) {
                        // conflict
                        conflict = true;
                        return;
                    }
                }
            }
        }
    }
}

void DPLLSolver::propogate_backward_helper(Node *curr) {
    if (curr->type != NodeType::AND) return;

    Edge& first_input  = curr->input_nodes[0];
    Edge& second_input = curr->input_nodes[1];

    bool first_is_const  = (first_input.node  == nullptr);
    bool second_is_const = (second_input.node == nullptr);

    // constant: nullptr + not inverted = FALSE, nullptr + inverted = TRUE
    bool first_input_assigned  = first_is_const  || first_input.node->assignment  != Assignment::UNASSIGNED;
    bool second_input_assigned = second_is_const || second_input.node->assignment != Assignment::UNASSIGNED;

    bool first_input_true  = first_is_const  ? first_input.inverted
                           : (first_input.node->assignment  == Assignment::TRUE) ^ first_input.inverted;
    bool second_input_true = second_is_const ? second_input.inverted
                           : (second_input.node->assignment == Assignment::TRUE) ^ second_input.inverted;

    // AND being TRUE
    // OUT (TRUE) = A (TRUE) AND B (TRUE)
    if (curr->assignment == Assignment::TRUE) {
        if (!first_is_const && first_input.node->assignment == Assignment::UNASSIGNED) {
            first_input.node->assignment = first_input.inverted ? Assignment::FALSE : Assignment::TRUE;
            first_input.node->decision_level = solver_decision_level;
            assignment_list.push_back(first_input.node);
            propagation_queue.push(first_input.node);

        }
        else if (!first_input_true && first_input_assigned) {

            conflict = true;
            return;
        }

        if (!second_is_const && second_input.node->assignment == Assignment::UNASSIGNED) {
            second_input.node->assignment = second_input.inverted ? Assignment::FALSE : Assignment::TRUE;
            second_input.node->decision_level = solver_decision_level;
            assignment_list.push_back(second_input.node);
            propagation_queue.push(second_input.node);

        }
        else if (!second_input_true && second_input_assigned) {
            conflict = true;
            return;
        }
    }

    // OUT (FALSE) = A (T/F) AND B (T/F)
    else if (curr->assignment == Assignment::FALSE) {
        // Both assigned and both true = conflict (FALSE AND cannot have both inputs true)
        if (first_input_assigned && second_input_assigned) {
            if (first_input_true && second_input_true) {
                conflict = true;
                return;
            }
        }

        // If other input is true, this one must be false
        if (!first_is_const && first_input.node->assignment == Assignment::UNASSIGNED) {
            if (second_input_true && second_input_assigned) {
                first_input.node->assignment = first_input.inverted ? Assignment::TRUE : Assignment::FALSE;
                first_input.node->decision_level = solver_decision_level;
                assignment_list.push_back(first_input.node);
                propagation_queue.push(first_input.node);

            }
        }

        if (!second_is_const && second_input.node->assignment == Assignment::UNASSIGNED) {
            if (first_input_true && first_input_assigned) {
                second_input.node->assignment = second_input.inverted ? Assignment::TRUE : Assignment::FALSE;
                second_input.node->decision_level = solver_decision_level;
                assignment_list.push_back(second_input.node);
                propagation_queue.push(second_input.node);

            }
        }
    }
}

bool DPLLSolver::conflict_handler() {
    // Backtrack to last decision level
    // Clear all assignments after last decision
    // Flip Decision and push back onto propagation queue
    solver_conflicts++;
    while (true) {
        if (solver_decision_level == 0 || decision_level_boundary_indexes.empty()) return false;
        Node* last_decision = assignment_list[decision_level_boundary_indexes.back()];
        if (last_decision->assignment == Assignment::TRUE) {
            backtrack();
            last_decision->assignment = Assignment::FALSE;
            assignment_list.push_back(last_decision);
            propagation_queue.push(last_decision);
            decision_level_boundary_indexes.pop_back();

            // Phase saving update
            last_decision->saved_phase = SavedPhase::FALSE;

            conflict = false;
            return true;
        }
        // Purge one decision further back
        backtrack();
        decision_level_boundary_indexes.pop_back();
        solver_decision_level--;
    }

}

inline void DPLLSolver::backtrack() {
    while (assignment_list.size() > decision_level_boundary_indexes.back()) {
        Node* removed_node = assignment_list.back();
        removed_node->assignment = Assignment::UNASSIGNED;
        removed_node->decision_level = -1;
        assignment_list.pop_back();
    }
    while (!propagation_queue.empty()) propagation_queue.pop();
}
