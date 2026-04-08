#include "Solver.h"
#include <iostream>
#include <unordered_set>

// --- Only 1 must be active ---
//#define CHOOSE_FIRST_UNASSIGNED
#define CHOOSE_AND_GATE_IMPORTANCE
//#define CHOOSE_LARGEST_FANOUT
//#define CHOOSE_VSIDS

//#define ALWAYS_BRANCH_TRUE
//#define ALWAYS_BRANCH_FALSE
#define PHASE_SAVING

// -- Only 1 must be active ---
//#define BRANCH_STRONGER_PROPAGATION
#define BRANCH_TRUE_DEFAULT
//#define BRANCH_FALSE_DEFAULT

#define PRIORITISE_CLOSER_OUTPUTS

void Solver::preprocess() {
    for (int i = 0; i < output_nodes.size(); i++) {
        Edge& output = output_nodes[i];
        if (output.node() == nullptr) {
            if (!output.inverted()) {
                conflict = true;
                return;
            }
            continue;
        }

        Assignment required = output.inverted() ? Assignment::FALSE : Assignment::TRUE;

        if (output.node()->assignment == Assignment::UNASSIGNED) {
            output.node()->assignment = required;
            output.node()->decision_level = 0;
            assignment_list.push_back(output.node());
            propagation_queue.push(output.node());

            // Phase saving if output connected node is somehow an input
            if (output.node()->type == NodeType::INPUT)
                output.node()->saved_phase = (required == Assignment::TRUE) ? SavedPhase::TRUE : SavedPhase::FALSE;
        }
        else if (output.node()->assignment != required) {
            conflict = true;
            return;
        }
    }
    while (!propagation_queue.empty() && !conflict) {
        propogate(propagation_queue.front());
        propagation_queue.pop();
    }
}

void Solver::compute_active_inputs() {
    // Go backwards up the graph from the output/s and remove/mark nodes that aren't directly relevant to the output

    // Records which node to process next (DFS traversal)
    std::vector<Node*> node_stack;
    // Ensures same node isn't processed again
    std::unordered_set<Node*> reachable_nodes;

    // Initialise by starting from outputs
    for (const auto& output_edge : output_nodes) {
        Node* node = output_edge.node();
        if (node != nullptr) {
            node_stack.push_back(node);
            reachable_nodes.insert(node);
            // Mark node as active (directly part of aig and influences output in some way)
            node->flipped_and_active_field = SET_NODE_ACTIVE(node->flipped_and_active_field);
        }
    }

    // Traverse back and add nodes that are directly related to output
    while (!node_stack.empty()) {
        Node* curr = node_stack.back();
        node_stack.pop_back();
        for (const Edge& edge : curr->input_nodes) {
            Node* input_node = edge.node();
            // Ensure not already in HashSet tracking already seen nodes
            if (input_node != nullptr && reachable_nodes.insert(input_node).second) {
                node_stack.push_back(input_node);
                // Mark node as active (directly part of aig and influences output in some way)
                input_node->flipped_and_active_field = SET_NODE_ACTIVE(input_node->flipped_and_active_field);
            }
        }
    }

    /* Avoids need for branching to see if active when choosing a node by reconstructing the input_nodes vector
    std::vector<Node*> new_marked_input_nodes;
    for (const auto& node : reachable_nodes) {
        if (node->type != NodeType::INPUT) continue;
        if (!node->active) continue;
        new_marked_input_nodes.push_back(node);
    }
    input_nodes = new_marked_input_nodes; */
}


void Solver::failed_literal_probing() {
    for (int i = 0; i < input_nodes.size(); i++) {
        Node* node = input_nodes[i];
        if (node == nullptr || node->assignment != Assignment::UNASSIGNED) continue;

        // Probe TRUE
        size_t saved_size = assignment_list.size();
        node->assignment = Assignment::TRUE;
        node->decision_level = 0;
        assignment_list.push_back(node);
        propagation_queue.push(node);
        while (!propagation_queue.empty() && !conflict) {
            propogate(propagation_queue.front());
            propagation_queue.pop();
        }
        bool true_fail = conflict;
        // Undo probing
        undo_probing(saved_size);

        // Prove FALSE
        node->assignment = Assignment::FALSE;
        node->decision_level = 0;
        assignment_list.push_back(node);
        propagation_queue.push(node);
        while (!propagation_queue.empty() && !conflict) {
            propogate(propagation_queue.front());
            propagation_queue.pop();
        }
        bool false_fail = conflict;
        // Undo probing
        undo_probing(saved_size);

        // UNSAT
        if (true_fail && false_fail) {
            conflict = true;
            return;
        }
        else if (true_fail) {
            // Has to be false
            node->assignment = Assignment::FALSE;
            node->decision_level = 0;
            assignment_list.push_back(node);
            propagation_queue.push(node);
            while (!propagation_queue.empty() && !conflict) {
                propogate(propagation_queue.front());
                propagation_queue.pop();
            }
            if (conflict) return;
        }
        else if (false_fail) {
            node->assignment = Assignment::TRUE;
            node->decision_level = 0;
            assignment_list.push_back(node);
            propagation_queue.push(node);
            while (!propagation_queue.empty() && !conflict) {
                propogate(propagation_queue.front());
                propagation_queue.pop();
            }
            if (conflict) return;
        }
    }
}

void Solver::undo_probing(size_t before_size) {
    while (assignment_list.size() > before_size) {
        Node* n = assignment_list.back();
        n->assignment = Assignment::UNASSIGNED;
        n->decision_level = -1;
        assignment_list.pop_back();
    }
    while (!propagation_queue.empty()) propagation_queue.pop();
    conflict = false;
}

bool Solver::run() {
    compute_active_inputs();
#ifdef PRIORITISE_CLOSER_OUTPUTS
    calculate_nodes_depth_from_output();
#endif
    preprocess();
    if (conflict) {
        std::cout << "UNSAT" << '\n';
        return false;
    }
    failed_literal_probing();
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

Node* Solver::decide_node() {
    solver_decisions++;

    // Change decision heuristic here
#ifdef CHOOSE_FIRST_UNASSIGNED
    Node* chosen_node = choose_first_unassigned();
#endif

#ifdef CHOOSE_AND_GATE_IMPORTANCE
    Node* chosen_node = and_gate_importance_scoring();
#endif

#ifdef CHOOSE_LARGEST_FANOUT
    Node* chosen_node = choose_largest_fanout();
#endif

#ifdef CHOOSE_VSIDS
    Node* chosen_node = vsids_choose_node();
#endif

    if (chosen_node != nullptr) {
    #ifdef ALWAYS_BRANCH_TRUE
        always_branch_true(chosen_node);
    #endif

    #ifdef ALWAYS_BRANCH_FALSE
        always_branch_false(chosen_node);
    #endif

    #ifdef PHASE_SAVING
        phase_saving(chosen_node);
    #endif

    }
    return chosen_node;
}

inline void Solver::always_branch_true(Node* chosen_node) {
    chosen_node->assignment = Assignment::TRUE;
    chosen_node->decision_level = solver_decision_level;
}
inline void Solver::always_branch_false(Node* chosen_node) {
    chosen_node->assignment = Assignment::FALSE;
    chosen_node->decision_level = solver_decision_level;
}
void Solver::phase_saving(Node* chosen_node) {
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

    else {
    #ifdef BRANCH_STRONGER_PROPAGATION
        // Choose based on which branch has stronger propagation implications
        bool true_prop_score = output_propogator_scorer(chosen_node, Assignment::TRUE);
        bool false_prop_score = output_propogator_scorer(chosen_node, Assignment::FALSE);

        chosen_node->assignment = (true_prop_score >= false_prop_score) ? Assignment::TRUE : Assignment::FALSE;
        chosen_node->decision_level = solver_decision_level;
        chosen_node->saved_phase = (chosen_node->assignment == Assignment::TRUE) ? SavedPhase::TRUE : SavedPhase::FALSE;
    #endif

    #ifdef BRANCH_TRUE_DEFAULT
        // Choose true every time by default
        chosen_node->assignment = Assignment::TRUE;
        chosen_node->decision_level = solver_decision_level;
        chosen_node->saved_phase = SavedPhase::TRUE;
    #endif

    #ifdef BRANCH_FALSE_DEFAULT
        // Choose false every time by default
        chosen_node->assignment = Assignment::FALSE;
        chosen_node->decision_level = solver_decision_level;
        chosen_node->saved_phase = SavedPhase::FALSE;
    #endif
    }
}


Node* Solver::choose_first_unassigned() {
    // input_nodes used rather than nodes_list to avoid unnecessarily long if condition (Applied for all node
    // choosing methods below as well)
    for (int i = 0; i < input_nodes.size(); i++) {
        Node* node = input_nodes[i];
        // Only choose UNASSIGNED nodes
        if (node->assignment == Assignment::UNASSIGNED) {
            return node;
        }
    }
    return nullptr;
}


// Choose the node with the largest fanout early so that larger amount of the search space is explored earlier
Node* Solver::choose_largest_fanout() {
    size_t max_fanout_length = 0;
    Node* chosen_node = nullptr;
    for (int i = 0; i < input_nodes.size(); i++) {
        Node* node = input_nodes[i];
        if (node->assignment == Assignment::UNASSIGNED && node->output_nodes.size() > max_fanout_length) {
            max_fanout_length = node->output_nodes.size();
            chosen_node = node;
        }
    }
    return chosen_node;
}

Node* Solver::and_gate_importance_scoring() {
    // Go through all input nodes and give them a scoring
    int best_score = 0;
    Node* best_node = nullptr;
    for (int i = 0; i < input_nodes.size(); i++) {
        int score = 0;
        Node* node = input_nodes[i];
        if (node->assignment == Assignment::UNASSIGNED) {
            for (int j = 0; j < node->output_nodes.size(); j++) {
                Edge* other_node_edge = (node == node->output_nodes[j]->input_nodes[0].node())
                                        ? &node->output_nodes[j]->input_nodes[1] : &node->output_nodes[j]->input_nodes[0];

                bool true_value = other_node_edge->node() == nullptr ? other_node_edge->inverted() :
                                  (other_node_edge->node()->assignment == Assignment::TRUE) ^ other_node_edge->inverted();

                if (other_node_edge->node() == nullptr) {
                    if (true_value) score += 10;
                }
                else {
                    if (true_value) score += 10;
                    else if (other_node_edge->node()->assignment == Assignment::UNASSIGNED) score += 2;
                }
            }
        }
    #ifdef PRIORITISE_CLOSER_OUTPUTS
        // Scale according to depth
        double depth_score_scaled = (node->depth_from_out >= 0) ? (10.0 / (node->depth_from_out + 1)) : 0;
        score += static_cast<int>(depth_score_scaled);
    #endif

        if (score > best_score) {
            best_score = score;
            best_node = node;
        }
    }
    return best_node;
}


double Solver::vsids_and_gate_scoring(Node* node) {
    double score = 0;
    for (int j = 0; j < node->output_nodes.size(); j++) {
        Edge* other_node_edge = (node == node->output_nodes[j]->input_nodes[0].node())
                        ? &node->output_nodes[j]->input_nodes[1] : &node->output_nodes[j]->input_nodes[0];

        bool true_value = other_node_edge->node() == nullptr ? other_node_edge->inverted() :
                        (other_node_edge->node()->assignment == Assignment::TRUE) ^ other_node_edge->inverted();

        if (other_node_edge->node() == nullptr) {
            if (true_value) score += 10;
        }
        else {
            if (true_value) score += 10;
            else if (other_node_edge->node()->assignment == Assignment::UNASSIGNED) score += 2;
        }
    }
    return score;
}

Node* Solver::vsids_choose_node() {
    Node* best_node = nullptr;
    double best_score = -1.0;
    for (int i = 0; i < input_nodes.size(); i++) {
        Node* node = input_nodes[i];
        if (node->assignment != Assignment::UNASSIGNED) continue;
        double score = node->activity + vsids_and_gate_scoring(node) * 0.3;
        if (score > best_score) {
            best_score = score;
            best_node = node;
        }
    }
    return best_node;
}

void Solver::propogate(Node *a) {
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

void Solver::propogate_forward_helper(Node *a) {
    for (int i = 0; i < a->output_nodes.size(); i++) {
        // Current node output
        Node& output = *a->output_nodes[i];
        // Output node is an AND node
        if (output.type == NodeType::AND) {
            Edge* curr_node_edge = &output.input_nodes[0];
            Edge* other_input    = &output.input_nodes[1];

            if (output.input_nodes[1].node() == a) {
                curr_node_edge = &output.input_nodes[1];
                other_input    = &output.input_nodes[0];
            }

            bool other_is_const = (other_input->node() == nullptr);

            // A (FALSE) AND ? = OUTPUT (FALSE)
            if ((a->assignment == Assignment::FALSE && !curr_node_edge->inverted()) ||
                (a->assignment == Assignment::TRUE  &&  curr_node_edge->inverted())) {
                if (output.assignment == Assignment::UNASSIGNED) {
                    output.assignment = Assignment::FALSE;
                    output.decision_level = solver_decision_level;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);

                }
                else if (output.assignment == Assignment::TRUE) {
                    // conflict
                    conflict = true;
                    bump_activity(&output);
                    bump_activity(a);
                    return;
                }
            }

            // A (TRUE) AND B (TRUE) = OUT (TRUE)
            else if ((a->assignment == Assignment::TRUE  && !curr_node_edge->inverted()) ||
                     (a->assignment == Assignment::FALSE &&  curr_node_edge->inverted())) {

                bool other_is_true = other_is_const
                    ? other_input->inverted()
                    : (other_input->node()->assignment == Assignment::TRUE) ^ other_input->inverted();
                bool other_assigned = other_is_const
                    || other_input->node()->assignment != Assignment::UNASSIGNED;

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
                        bump_activity(&output);
                        bump_activity(a);
                        bump_activity(other_input->node());
                        return;
                    }
                }
            }
        }
    }
}

void Solver::propogate_backward_helper(Node *curr) {
    if (curr->type != NodeType::AND) return;

    Edge& first_input  = curr->input_nodes[0];
    Edge& second_input = curr->input_nodes[1];

    bool first_is_const  = (first_input.node()  == nullptr);
    bool second_is_const = (second_input.node() == nullptr);

    // constant: nullptr + not inverted = FALSE, nullptr + inverted = TRUE
    bool first_input_assigned  = first_is_const  || first_input.node()->assignment  != Assignment::UNASSIGNED;
    bool second_input_assigned = second_is_const || second_input.node()->assignment != Assignment::UNASSIGNED;

    bool first_input_true  = first_is_const  ? first_input.inverted()
                           : (first_input.node()->assignment  == Assignment::TRUE) ^ first_input.inverted();
    bool second_input_true = second_is_const ? second_input.inverted()
                           : (second_input.node()->assignment == Assignment::TRUE) ^ second_input.inverted();

    // AND being TRUE
    // OUT (TRUE) = A (TRUE) AND B (TRUE)
    if (curr->assignment == Assignment::TRUE) {
        if (!first_is_const && first_input.node()->assignment == Assignment::UNASSIGNED) {
            first_input.node()->assignment = first_input.inverted() ? Assignment::FALSE : Assignment::TRUE;
            first_input.node()->decision_level = solver_decision_level;
            assignment_list.push_back(first_input.node());
            propagation_queue.push(first_input.node());

        }
        else if (!first_input_true && first_input_assigned) {
            conflict = true;
            bump_activity(first_input.node());
            bump_activity(curr);
            return;
        }

        if (!second_is_const && second_input.node()->assignment == Assignment::UNASSIGNED) {
            second_input.node()->assignment = second_input.inverted() ? Assignment::FALSE : Assignment::TRUE;
            second_input.node()->decision_level = solver_decision_level;
            assignment_list.push_back(second_input.node());
            propagation_queue.push(second_input.node());

        }
        else if (!second_input_true && second_input_assigned) {
            conflict = true;
            bump_activity(second_input.node());
            bump_activity(curr);
            return;
        }
    }

    // OUT (FALSE) = A (T/F) AND B (T/F)
    else if (curr->assignment == Assignment::FALSE) {
        // Both assigned and both true = conflict (FALSE AND cannot have both inputs true)
        if (first_input_assigned && second_input_assigned) {
            if (first_input_true && second_input_true) {
                conflict = true;
                bump_activity(first_input.node());
                bump_activity(second_input.node());
                bump_activity(curr);
                return;
            }
        }

        // If other input is true, this one must be false
        if (!first_is_const && first_input.node()->assignment == Assignment::UNASSIGNED) {
            if (second_input_true && second_input_assigned) {
                first_input.node()->assignment = first_input.inverted() ? Assignment::TRUE : Assignment::FALSE;
                first_input.node()->decision_level = solver_decision_level;
                assignment_list.push_back(first_input.node());
                propagation_queue.push(first_input.node());
            }
        }

        if (!second_is_const && second_input.node()->assignment == Assignment::UNASSIGNED) {
            if (first_input_true && first_input_assigned) {
                second_input.node()->assignment = second_input.inverted() ? Assignment::TRUE : Assignment::FALSE;
                second_input.node()->decision_level = solver_decision_level;
                assignment_list.push_back(second_input.node());
                propagation_queue.push(second_input.node());

            }
        }
    }
}

bool Solver::conflict_handler() {
    // Backtrack to last decision level
    // Clear all assignments after last decision
    // Flip Decision and push back onto propagation queue
    solver_conflicts++;
    // Tweak this if necessary
    if (solver_conflicts % 100 == 0) {
        decay_every_N_conflicts();
    }
    while (true) {
        if (solver_decision_level == 0 || decision_level_boundary_indexes.empty()) return false;
        backtrack();
        // Use index in assignment list from boundaries index to get last decision
        Node* last_decision = assignment_list[decision_level_boundary_indexes.back()];
        assignment_list.pop_back();
        if (!NODE_FLIPPED_TRUE(last_decision->flipped_and_active_field)) {
            last_decision->assignment = (last_decision->assignment == Assignment::TRUE) ? Assignment::FALSE : Assignment::TRUE;
            last_decision->decision_level = solver_decision_level;
            last_decision->flipped_and_active_field = SET_NODE_FLIPPED(last_decision->flipped_and_active_field);

            // Add Phase Saving
            last_decision->saved_phase = (last_decision->assignment == Assignment::TRUE) ? SavedPhase::TRUE : SavedPhase::FALSE;
            assignment_list.push_back(last_decision);
            propagation_queue.push(last_decision);
            conflict = false;
            return true;
        }
        // Purge one decision further back
        last_decision->assignment = Assignment::UNASSIGNED;
        last_decision->decision_level = -1;
        last_decision->flipped_and_active_field = CLEAR_NODE_FLIPPED(last_decision->flipped_and_active_field);
        decision_level_boundary_indexes.pop_back();
        solver_decision_level--;
    }

}

inline void Solver::backtrack() {
    size_t stop = decision_level_boundary_indexes.back() + 1;
    while (assignment_list.size() > stop) {
        Node* removed_node = assignment_list.back();
        removed_node->assignment = Assignment::UNASSIGNED;
        removed_node->decision_level = -1;
        assignment_list.pop_back();
    }
    while (!propagation_queue.empty()) propagation_queue.pop();
}

void Solver::bump_activity(Node *node) {
    if (node == nullptr) return;
    node->activity += solver_activity_increment;
}

void Solver::decay_every_N_conflicts() {
    for (int i = 0; i < nodes_list_size; i++) {
        if (nodes_list[i] != nullptr)
            nodes_list[i]->activity *= solver_activity_decay;
    }
}

void Solver::print_stats() {
    std::cout << "TOTAL DECISIONS: " << solver_decisions << '\n';
    std::cout << "TOTAL CONFLICTS: " << solver_conflicts << '\n';
    std::cout << "TOTAL PROPAGATIONS: " << solver_propagations << '\n';
}

void Solver::print_preprocess_stats() {
    int total_and_gates = 0;
    // Calculate statistics for how our preprocessing was (used for comparison)
    for (int i = 0; i < nodes_list_size; i++) {
        if (nodes_list[i] == nullptr) continue;
        if (nodes_list[i]->type == NodeType::INPUT) {
            solver_total_inputs++;
            if (nodes_list[i]->assignment != Assignment::UNASSIGNED) {
                solver_assigned_inputs++;
                if (nodes_list[i]->assignment == Assignment::TRUE) {
                    solver_true_forced++;
                }
                else {
                    solver_false_forced++;
                }
            }
        }
        else if (nodes_list[i]->type == NodeType::AND) {
            total_and_gates++;
            if (nodes_list[i]->assignment != Assignment::UNASSIGNED) solver_ands_forced++;
        }
    }

    std::cout << "TOTAL INPUTS: " << solver_total_inputs << '\n';
    std::cout << "INPUTS ASSIGNED: " << solver_assigned_inputs << '\n';
    std::cout << "TRUE FORCED: " << solver_true_forced << '\n';
    std::cout << "FALSE FORCED: " << solver_false_forced << '\n';
    std::cout << "ANDS FORCED: " << (100.0 * solver_ands_forced/total_and_gates) << '\n';
}

void Solver::print_compute_active_inputs_stats() {
    total_valid_nodes_count();
    int total_active_nodes = 0;
    int total_input_nodes_active = 0;
    for (int i = 0; i < nodes_list_size; i++) {
        Node* node = nodes_list[i];
        if (node == nullptr) continue;
        if (NODE_ACTIVE_TRUE(node->flipped_and_active_field)) total_active_nodes++;
        if (node->type == NodeType::INPUT && NODE_ACTIVE_TRUE(node->flipped_and_active_field)) total_input_nodes_active++;
    }
    double percent_nodes_active =  (100.0 * total_active_nodes / actual_total_nodes);
    std::cout << "PERCENT OF NODES ACTIVE: " << percent_nodes_active << '\n';
    std::cout << "PERCENT OF NODES INACTIVE: " << 100 - percent_nodes_active << '\n';
    if (!input_nodes.empty()) {
        if (actual_total_nodes > 0) {
            double percent_input_nodes_active = 100.0 * total_input_nodes_active/ static_cast<double>(input_nodes.size());
            std::cout << "PERCENT OF INPUT NODES ACTIVE: " << percent_input_nodes_active << '\n';
            std::cout << "PERCENT OF INPUT NODES INACTIVE: " << 100 - percent_input_nodes_active << '\n';
        }
    }
}

// Because of AIGER indexing scheme there are indexes that don't point to valid nodes so get a clearer indication
// when calculating metrics
inline void Solver::total_valid_nodes_count() {
    // reset counter when recounting
    actual_total_nodes = 0;
    for (int i = 0; i < nodes_list_size; i++) {
        if (nodes_list[i] != nullptr) actual_total_nodes++;
    }
}

int Solver::output_propogator_scorer(Node* node, Assignment assignment) {
    int score = 0;
    for (int i = 0; i < node->output_nodes.size(); i++) {
        Node* output_node = node->output_nodes[i];
        if (output_node->type != NodeType::AND) continue;
        Edge* curr_edge = (node == output_node->input_nodes[0].node()) ? &output_node->input_nodes[1] : &output_node->input_nodes[0];
        Edge* other_edge = (node == output_node->input_nodes[0].node()) ? &output_node->input_nodes[1] : &output_node->input_nodes[0];

        bool other_node_assigned = other_edge->node() == nullptr || other_edge->node()->assignment != Assignment::UNASSIGNED;
        bool other_node_true = other_edge->node() == nullptr ? other_edge->inverted() : (other_edge->node()->assignment == Assignment::TRUE) ^ other_edge->inverted();

        bool curr_node_actual_true = (assignment == Assignment::TRUE) ^ curr_edge->inverted();

        if (curr_node_actual_true) {
            if (other_node_assigned && other_node_true) score += 10;
            else if (!other_node_assigned) score += 5;
            else score += 1;
        }
        else {
            if (output_node->assignment == Assignment::FALSE) score += 7;
            else if (output_node->assignment == Assignment::UNASSIGNED) score += 4;
            else score -= 5;
        }
    }
    return score;
}

void Solver::calculate_nodes_depth_from_output() {
    std::queue<Node*> queue;
    for (const auto& output_edge : output_nodes) {
        Node* node = output_edge.node();
        if (node != nullptr && node->depth_from_out == -1) {
            node->depth_from_out = 0;
            queue.push(node);
        }
    }

    while (!queue.empty()) {
        Node* node = queue.front();
        queue.pop();
        for (const auto& edge : node->input_nodes) {
            Node* input = edge.node();
            if (input != nullptr && input->depth_from_out == -1) {
                input->depth_from_out = node->depth_from_out + 1;
                queue.push(input);
            }
        }
    }
}