#include <assert.h>
#include <complex>
#include <iostream>
// import aiger library - use extern C to disable name mangling
extern "C" {
    #include "aiger.h"
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

    enum class NodeType {INPUT, AND};
    struct Node;
    struct Edge {
        Node* node;
        bool inverted;
    };
    struct Node {
        NodeType type;
        unsigned int variable_number;
        Edge input_nodes[2];
    };

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

        static_assert(nodes_list[and_node.rhs0 >> 1] != nullptr);
        static_assert(nodes_list[and_node.rhs1 >> 1] != nullptr);
        nodes_list[index]->input_nodes[0] = {nodes_list[and_node.rhs0 >> 1], and_node.rhs0 % 2 == 1};
        nodes_list[index]->input_nodes[1] = {nodes_list[and_node.rhs1 >> 1], and_node.rhs1 % 2 == 1};
    }

    // outputs
    std::vector<Edge> outputs;

    for (int i = 0; i < aig->num_outputs; i++) {
        unsigned index = aig->outputs[i].lit >> 1;
        bool inverted = (aig->outputs[i].lit & 1) != 0;
        // (Arena allocator will be used)
        outputs.push_back({nodes_list[aig->outputs[i].lit >> 1], (aig->outputs[i].lit & 1) != 0});
    }

    aiger_reset(aig);
    return 0;

}
