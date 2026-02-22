#include <assert.h>
#include <complex>
#include <iostream>
#include "aig.h"
#include "Solver.h"

// import aiger library - use extern C to disable name mangling
extern "C" {
    #include "aiger.h"
}


void printInternalAIG(Node* nodes_list[], std::vector<Edge>& outputs, unsigned maxvar) {
    for (int i = 1; i <= maxvar; i++) {
        Node* n = nodes_list[i];
        if (!n) continue;
        std::cout << "Node " << i << ": type="
             << (n->type == NodeType::INPUT ? "INPUT" : "AND") << "\n";

        if (n->type == NodeType::AND) {
            for (int j = 0; j < 2; j++) {
                Edge& e = n->input_nodes[j];
                std::cout << "  Input " << j << " -> Node " << e.node->variable_number
                          << (e.inverted ? " (inverted)" : "") << "\n";
            }
        }

        if (n->output_nodes.size() <= 0) continue;
        std::cout << "Outputs for this node: " << '\n';
        for (int k = 0; k <  n->output_nodes.size(); k++) {
            std::cout << "Node: " << n->output_nodes[k]->variable_number << '\n';
        }
    }


    for (int i = 0; i < outputs.size(); i++) {
        Edge& e = outputs[i];
        std::cout << "Output " << i << " -> ";
        if (e.node == nullptr) {
            std::cout << (e.inverted ? "CONST 1" : "CONST 0");
        } else {
            std::cout << "Node " << e.node->variable_number;
            if (e.inverted) std::cout << " (inverted)";
        }
        std::cout << '\n';
    }

}
Edge edge_from_literal(unsigned literal, Node* nodes_list[]) {
    if (literal == 0) return {nullptr, false};
    if (literal == 1) return {nullptr, true};
    return {nodes_list[literal >> 1], (literal & 1) != 0};
}

int main(int argc, char **argv) {
    // Parse .aig format using aiger library
    aiger* aig = aiger_init();

    const char* aiger_file =  aiger_open_and_read_from_file(aig, argv[1]);
    if (aiger_file != nullptr) {
        std::cerr << "Error opening file: " << aiger_file << '\n';
        aiger_reset(aig);
        return 1;
    }
    // Ensure no latches present (Script will manually unroll beforehand)
    if (aig->num_latches > 0) {
        std::cerr << "No latches allowed, unroll first";
        return 1;
    }

    // Test to see we are printing all nodes from file correctly
    for (int i = 0; i < aig->num_inputs; i++) {
        unsigned variable_index = aig->inputs[i].lit / 2;
        std::cout << "Input Variables: " << variable_index << '\n';
    }
    for (int i = 0; i < aig->num_ands; i++) {
        const aiger_and& a = aig->ands[i];
        unsigned lhs_idx = a.lhs / 2;
        unsigned rhs_idx = a.rhs0 / 2;
        unsigned rhs_ix_2 = a.rhs1 / 2;
        std::cout << lhs_idx << " = " << rhs_idx << " AND " << rhs_ix_2 << "\n\n";
    }


    Node* nodes_list[aig->maxvar + 2];
    // add input nodes to graph/circuit
    for (int i = 0; i < aig->num_inputs; i++) {
        const aiger_symbol& input_node = aig->inputs[i];
        const unsigned index = input_node.lit >> 1;
        // allocate space for the new node on heap for now (Arena allocator will be used)
        nodes_list[index] = new Node();
        nodes_list[index]->type = NodeType::INPUT;
        nodes_list[index]->variable_number = index;
        nodes_list[index]->input_nodes[0] = {nullptr, false};
        nodes_list[index]->input_nodes[1] = {nullptr, false};
    }
    // add AND nodes to graph/circuit
    for (int i = 0; i < aig->num_ands; i++) {
        const aiger_and& and_node = aig->ands[i];
        const unsigned index = and_node.lhs >> 1;

        // allocate space for the new node on heap for now (Arena allocator will be used)
        nodes_list[index] = new Node();
        nodes_list[index]->type = NodeType::AND;
        nodes_list[index]->variable_number = index;

        // Ensure variables exist
        if (and_node.rhs0 > 0) assert(nodes_list[and_node.rhs0 >> 1] != nullptr);
        if (and_node.rhs1 > 0) assert(nodes_list[and_node.rhs1 >> 1] != nullptr);

        nodes_list[index]->input_nodes[0] = edge_from_literal(and_node.rhs0, nodes_list);
        nodes_list[index]->input_nodes[1] = edge_from_literal(and_node.rhs1, nodes_list);

        if (nodes_list[index]->input_nodes[0].node != nullptr)
            nodes_list[index]->input_nodes[0].node->output_nodes.push_back(nodes_list[index]);
        if (nodes_list[index]->input_nodes[1].node != nullptr)
            nodes_list[index]->input_nodes[1].node->output_nodes.push_back(nodes_list[index]);
    }

    // outputs
    std::vector<Edge> outputs;
    for (int i = 0; i < aig->num_outputs; i++) {
        unsigned index = aig->outputs[i].lit >> 1;
        bool inverted = (aig->outputs[i].lit & 1) != 0;
        // (Arena allocator will be used)
        Edge output{};
        if (aig->outputs[i].lit == 0) output = {nullptr, false};
        else if (aig->outputs[i].lit == 1) output = {nullptr, true};
        else {
            output = {
                .node = nodes_list[aig->outputs[i].lit >> 1],
                .inverted = (aig->outputs[i].lit & 1) != 0
            };
        }
        outputs.push_back(output);
    }

    // print structure of AIG to ensure it is correct
    printInternalAIG(nodes_list, outputs, aig->maxvar);




    aiger_reset(aig);
    return 0;

}


/* flow of what is to occur in algorithm
// assign a value for a node
Solver solver{};
solver.nodes_list = nodes_list;
solver.output_nodes = outputs;

Node& test_node = *nodes_list[0];
test_node.assignment = Assignment::TRUE;
test_node.decision_level = 0;

solver.assignment_list.push_back(&test_node);
solver.decision_level_boundary_indexes.at(test_node.decision_level)++;
solver.propogation_queue.push(&test_node);


// Node A (l : 1)
// Node B (l : 1)

// [2, 3, 4, 5]


PROPAGATION RULES

            // Cover entire truth table
            // F AND F = F
            // F AND T = F
            // T AND F = F
            // T AND T = T

            if (other_input->node->assignment == Assignment::UNASSIGNED && output.assignment == Assignment::UNASSIGNED) {
                // Only thing we can propagate if F AND F = F
                if (a->assignment == Assignment::TRUE && !curr_node_edge->inverted ||
                    a->assignment == Assignment::FALSE && curr_node_edge->inverted) {
                    output.assignment = Assignment::FALSE;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);
                }
            }

            else if (other_input->node->assignment != Assignment::UNASSIGNED) {
                // F AND (T || F) = F
                // If we are FALSE, Propagate out that our AND gate is false
                if ((curr_node_edge->inverted && a->assignment == Assignment::TRUE) ||
                    !curr_node_edge->inverted && a->assignment == Assignment::FALSE) {
                    output.assignment = Assignment::FALSE;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);
                    }

                // (T || F) AND F = F
                // Propagate if other input is FALSE, so AND is a FALSE
                else if ((other_input->node->assignment == Assignment::TRUE && other_input->inverted) ||
                    other_input->node->assignment == Assignment::FALSE && !other_input->inverted) {
                    if (output.assignment == Assignment::UNASSIGNED) {
                        output.assignment = Assignment::FALSE;
                        assignment_list.push_back(&output);
                        propagation_queue.push(&output);
                    }
                    else if (output.assignment == Assignment::TRUE) {}// conflict
                    else {} // skip

                    }

                // T AND T = T
                // If we are True, and other input Gate is True, AND must be true
                else if (((curr_node_edge->inverted && a->assignment == Assignment::FALSE) ||
                    (!curr_node_edge->inverted && a->assignment == Assignment::TRUE))
                    && ((other_input->inverted && other_input->node->assignment == Assignment::FALSE) ||
                    (!other_input->inverted && other_input->node->assignment == Assignment::TRUE))) {

                    if (output.assignment == Assignment::UNASSIGNED) {}

                    output.assignment = Assignment::TRUE;
                    assignment_list.push_back(&output);
                    propagation_queue.push(&output);
                    }
            }

            else if (output.assignment != Assignment::UNASSIGNED) {
                // T AND ? = F
                // T AND ? = T
                if ((a->assignment == Assignment::TRUE && !curr_node_edge->inverted ||
                    a->assignment == Assignment::FALSE && curr_node_edge->inverted) &&
                    (output.assignment == Assignment::FALSE)) {
                    if (other_input->inverted) other_input->node->assignment = Assignment::TRUE;
                    else other_input->node->assignment = Assignment::FALSE;
                }

                if ((a->assignment == Assignment::TRUE && !curr_node_edge->inverted ||
                    a->assignment == Assignment::FALSE && curr_node_edge->inverted) &&
                    (output.assignment == Assignment::TRUE)) {
                    if (other_input->inverted) other_input->node->assignment = Assignment::FALSE;
                    else other_input->node->assignment = Assignment::TRUE;
                }
            }

            else {continue;}



// Detect conflict
    // Create Implication Graph
    // Analyse conflict  (Cut/UIP)
    // Generate learned constraint and Store in some manner not to do this again?
    // Get the backjump level
    // Undo assignments /  Purge assigment list back to last decision level --> Backjump
    // Add learned clause
    // Resume propagation


    /* clause = reasons of conflict
    while (number of literals in clause at current level > 1):
        pick one
        replace it with its reasons
    UIP = the remaining current-level literal
    learn clause


unsigned int current_nodes_counted = (conflict_node->decision_level == decision_level_boundary_indexes.back()) ? 1 : 0;
Node* uip = nullptr;
do {
    Node* curr = nodes_stack.top();
    nodes_stack.pop();
    visited_nodes.insert(curr);
    if (curr->decision_level == decision_level_boundary_indexes.back()) {
        current_nodes_counted--;
        if (current_nodes_counted == 1 && uip == nullptr)
            uip = curr;
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
*/