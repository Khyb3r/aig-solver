#include <assert.h>
#include <complex>
#include <iostream>
#include "aig.h"
// import aiger library - use extern C to disable name mangling
extern "C" {
    #include "aiger.h"
}

void printInternalAIG(const Node* nodes_list[], const std::vector<Edge>& outputs) {

}

Edge edge_from_literal(unsigned literal, Node* nodes_list[]) {
    if (literal == 0) return {nullptr, false};
    else if (literal == 1) return {nullptr, true};
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

    // Make sure we are correctly parsing the file
    /*std::cout << "Inputs: " << aig->num_inputs << '\n';
    std::cout << "Outputs: " << aig->num_outputs << '\n';
    std::cout << "ANDs: " << aig->num_ands << '\n';
    */
    for (int i = 0; i < aig->num_inputs; i++) {
        unsigned variable_index = aig->inputs[i].lit / 2;
        std::cout << "Input Variables: " << variable_index << '\n';
    }
    for (int i = 0; i < aig->num_ands; i++) {
        const aiger_and& a = aig->ands[i];
        unsigned lhs_idx = a.lhs / 2;
        unsigned rhs_idx = a.rhs0 / 2;
        unsigned rhs_ix_2 = a.rhs1 / 2;
        std::cout << lhs_idx << " = " << rhs_idx << " AND " << rhs_ix_2 << "\n";
    }

    Node* nodes_list[aig->maxvar + 2] = {nullptr};

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
        if (and_node.rhs0 > 0) assert(nodes_list[and_node.rhs1 >> 1] != nullptr);
        if (and_node.rhs1 > 0) assert(nodes_list[and_node.rhs0 >> 1] != nullptr);

        nodes_list[index]->input_nodes[0] = edge_from_literal(and_node.rhs0, nodes_list);
        nodes_list[index]->input_nodes[1] = edge_from_literal(and_node.rhs1, nodes_list);
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

    aiger_reset(aig);
    return 0;

}
