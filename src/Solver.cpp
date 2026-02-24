#include "Solver.h"
#include <iostream>
#include <set>

void Solver::preprocess() {
    for (int i = 0; i < output_nodes.size(); i++) {
        Edge& output = output_nodes[i];
        if (output.node != nullptr && output.node->assignment == Assignment::UNASSIGNED) {
            output.node->assignment = output.inverted ? Assignment::FALSE : Assignment::TRUE;
            output.node->decision_level = 0;
            assignment_list.push_back(output.node);
            propagation_queue.push(output.node);
        }
    }
    while (!propagation_queue.empty() && !conflict) {
        Node& n = *propagation_queue.front();
        propagation_queue.pop();
        propogate(&n);
    }
}

void Solver::run(int nodes_list_size) {
    // choose a node to decide
    // make a decision
    // add to assignment list, add to propogation queue
    // keep propogating until queue is empty or conflict found -> repeat step above and this again and again

    preprocess();
    decision_level_boundary_indexes.push_back(assignment_list.size());
    solver_decision_level = 1;
    while (SAT) {
        if (satisfiable_check(nodes_list_size)) {
            std::cout << "SATISFIABLE" << '\n';
            break;
        }
        decision_level_boundary_indexes.push_back(assignment_list.size());
        Node* node = decide_node(nodes_list_size);
        if (node == nullptr) break;
        assignment_list.push_back(node);
        propagation_queue.push(node);
        while (!propagation_queue.empty() && !conflict) {
            Node& n = *propagation_queue.front();
            propagation_queue.pop();
            propogate(&n);
        }
        conflict = false;
        solver_decision_level++;
    }
    if (!SAT) {
        std::cout << "UNSATISFIABLE" << '\n';
    }
}

Node* Solver::decide_node(int nodes_list_size) {
    Node* node = nullptr;
    for (int i = 0; i < nodes_list_size; i++) {
        if (nodes_list[i] != nullptr && nodes_list[i]->assignment == Assignment::UNASSIGNED) {
            node = nodes_list[i];
            break;
        }
    }
    if (node == nullptr) return nullptr;

    if (node->assignment == Assignment::UNASSIGNED) {
        // Change way in which this is done (CURRENTLY ALWAYS DECIDING TRUE)
        node->assignment = Assignment::TRUE;
        node->decision_level = solver_decision_level;
    }
    return node;
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
        Node& output = *a->output_nodes[i];
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
                    output.decision_level = solver_decision_level;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);
                    output.reason = a;
                }
                else if (output.assignment == Assignment::TRUE) {
                    // conflict
                    conflict_handler(&output);
                    conflict = true;
                    return;
                }
            }

            // A (TRUE) AND B (TRUE) = OUT (TRUE)
            else if (((a->assignment == Assignment::TRUE && !curr_node_edge->inverted) ||
                (a->assignment == Assignment::FALSE && curr_node_edge->inverted)) &&
                ((other_input->node->assignment == Assignment::TRUE && !other_input->inverted)  ||
                (other_input->node->assignment == Assignment::FALSE && other_input->inverted))) {

                if (output.assignment == Assignment::UNASSIGNED) {
                    output.assignment = Assignment::TRUE;
                    output.decision_level = solver_decision_level;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);
                    output.reason = a;
                    output.reason_two = other_input->node;
                }
                else if (output.assignment == Assignment::FALSE) {
                    // conflict
                    conflict_handler(&output);
                    conflict = true;
                    return;
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
            first_input.node->decision_level = solver_decision_level;
            assignment_list.push_back(first_input.node);
            propagation_queue.push(first_input.node);
            first_input.node->reason = curr;
        }

        else if (!first_input_true && first_input_assigned) {
            // conflict
            conflict_handler(first_input.node);
            conflict = true;
            return;
        }

        // If Second Unassigned set to True otherwise conflict if set to True
        if (second_input.node->assignment == Assignment::UNASSIGNED) {
            second_input.node->assignment = second_input.inverted ? Assignment::FALSE : Assignment::TRUE;
            second_input.node->decision_level = solver_decision_level;
            assignment_list.push_back(second_input.node);
            propagation_queue.push(second_input.node);
            second_input.node->reason = curr;
        }
        else if (!second_input_true && second_input_assigned) {
            // conflict
            conflict_handler(second_input.node);
            conflict = true;
            return;
        }
    }


    // OUT (FALSE) = A (T/F) AND B (T/F)
    else if (curr->assignment == Assignment::FALSE) {
        //  If one gate is True make other False
        if (first_input.node->assignment != Assignment::UNASSIGNED && second_input.node->assignment != Assignment::UNASSIGNED) {
            if (first_input_true && first_input_assigned && second_input_true && second_input_assigned) {
                // conflict
                conflict_handler(curr);
                conflict = true;
                return;
            }
        }

        if (first_input.node->assignment == Assignment::UNASSIGNED) {
            if (second_input_true && second_input_assigned) {
                first_input.node->assignment = first_input.inverted ? Assignment::TRUE : Assignment::FALSE;
                first_input.node->decision_level = solver_decision_level;
                assignment_list.push_back(first_input.node);
                propagation_queue.push(first_input.node);
                first_input.node->reason = curr;
                first_input.node->reason_two = second_input.node;
            }
        }

        if (second_input.node->assignment == Assignment::UNASSIGNED) {
            if (first_input_true && first_input_assigned) {
                second_input.node->assignment = second_input.inverted ? Assignment::TRUE : Assignment::FALSE;
                second_input.node->decision_level = solver_decision_level;
                assignment_list.push_back(second_input.node);
                propagation_queue.push(second_input.node);
                second_input.node->reason = curr;
                second_input.node->reason_two = first_input.node;
            }
        }
    }
}

void Solver::conflict_handler(Node* conflict_node) {
    std::cout << "CONFLICT HANDLER METHOD" << '\n';
    if (conflict_node->decision_level == 0) {
        SAT = false;
        return;
    }
    Clause clause;

    unsigned int nodes_in_curr_decision_level = 0;
    unsigned int conflict_node_decision_level = conflict_node->decision_level;

    // Add reasons to clause
    if (conflict_node->reason != nullptr) {
        clause.literals.push_back({conflict_node->reason, conflict_node->reason->assignment == Assignment::TRUE});
        if (conflict_node->reason->decision_level == conflict_node_decision_level)
            nodes_in_curr_decision_level++;
    }
    if (conflict_node->reason_two != nullptr) {
        clause.literals.push_back({conflict_node->reason_two, conflict_node->reason_two->assignment == Assignment::TRUE});
        if (conflict_node->reason_two->decision_level == conflict_node_decision_level)
            nodes_in_curr_decision_level++;

    }

    while (nodes_in_curr_decision_level > 1) {
        // Pick any literal to be resolved in clause at current decision level
        Literal literal = {nullptr, false};
        for (int i = 0; i < clause.literals.size(); i++) {
            const Literal& lit = clause.literals[i];
            if (lit.node != nullptr && lit.node->decision_level == conflict_node_decision_level && lit.node->reason != nullptr) {
                literal = lit;
                clause.literals.erase(clause.literals.begin() + i);
                break;
            }
        }
        if (literal.node == nullptr) break;

        // Add its reasons to clause
        bool lit_in_clause_flag_1 = false;
        bool lit_in_clause_flag_2 = false;
        for (int i = 0; i < clause.literals.size(); i++) {
            const Literal& curr_literal = clause.literals[i];
            if (literal.node->reason != nullptr && curr_literal.node == literal.node->reason && curr_literal.is_negated == (literal.node->reason->assignment == Assignment::TRUE))
                lit_in_clause_flag_1 = true;
            if (literal.node->reason_two != nullptr && curr_literal.node == literal.node->reason_two && curr_literal.is_negated == (literal.node->reason_two->assignment == Assignment::TRUE))
                lit_in_clause_flag_2 = true;
        }
        if (literal.node->reason != nullptr && !lit_in_clause_flag_1) {
            clause.literals.push_back({literal.node->reason, literal.node->reason->assignment == Assignment::TRUE});
        }
        if (literal.node->reason_two != nullptr && !lit_in_clause_flag_2){
            clause.literals.push_back({literal.node->reason_two, literal.node->reason_two->assignment == Assignment::TRUE});
        }

        // Update how many literals in clause at current decision level are left (Change loop body to increment/decrement instead of this)
        nodes_in_curr_decision_level = 0;
        for (int i = 0 ; i < clause.literals.size(); i++) {
            const Literal& l = clause.literals[i];
            if (l.node != nullptr && l.node->decision_level == conflict_node_decision_level) {
                nodes_in_curr_decision_level++;
            }
        }
    }

    Literal uip{};
    for (int i = 0; i < clause.literals.size(); i++) {
        const Literal& literal = clause.literals[i];
        if (literal.node->decision_level == conflict_node_decision_level) {
            uip = literal;
        }
    }
    // Calculate backjump level
    unsigned int backjump_level = 0;
    for (int i = 0; i < clause.literals.size(); i++) {
        const Literal& literal = clause.literals[i];
        if (literal.node->decision_level > backjump_level && literal.node != uip.node) {
            backjump_level = literal.node->decision_level;
        }
    }
    // Add clause to database
    clause_db.push_back(clause);
    backjump(backjump_level);

    uip.node->assignment = uip.is_negated ? Assignment::FALSE : Assignment::TRUE;
    uip.node->decision_level = solver_decision_level;
    uip.node->reason = nullptr;
    uip.node->reason_two = nullptr;
    assignment_list.push_back(uip.node);
    propagation_queue.push(uip.node);
}


void Solver::backjump(unsigned int backjump_level) {
    int decision_level_for_backjump = decision_level_boundary_indexes[backjump_level];

    for (int i = decision_level_for_backjump; i < assignment_list.size(); i++) {
        Node* curr_node = assignment_list[i];
        curr_node->assignment = Assignment::UNASSIGNED;
        curr_node->decision_level = 0;
        curr_node->reason = nullptr;
        curr_node->reason_two = nullptr;
    }
    assignment_list.erase(assignment_list.begin() + decision_level_for_backjump, assignment_list.end());
    decision_level_boundary_indexes.erase(decision_level_boundary_indexes.begin() + backjump_level, decision_level_boundary_indexes.end());
    while (!propagation_queue.empty()) {
        propagation_queue.pop();
    }
    solver_decision_level = backjump_level;
}

bool Solver::satisfiable_check(int nodes_list_size) {
    for (int i = 0; i < nodes_list_size; i++) {
        if (nodes_list[i] != nullptr && nodes_list[i]->assignment == Assignment::UNASSIGNED) {
            return false;
        }
    }
    return true;
}
