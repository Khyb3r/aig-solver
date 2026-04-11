#include <assert.h>
#include <complex>
#include <iostream>
#include <chrono>
#include <__filesystem/recursive_directory_iterator.h>

#include "Solver/Solver.h"
#include "Arena.h"

// Round up to the nearest multiple of align so the arena doesn't run out of memory due to Node not being a
// multiple of DEFAULT_ALIGNMENT
#define ALIGNED_SIZE(size, align) (((size) + (align) - 1) & ~((align) - 1))
#define NODE_ALIGNED_SIZE ALIGNED_SIZE(sizeof(Node), DEFAULT_ALIGNMENT)

// import aiger library - use extern C to disable name mangling
extern "C" {
    #include "aiger.h"
}


void printInternalAIG(Node* nodes_list[], std::vector<Edge>& outputs, unsigned maxvar) {
    for (int i = 1; i <= maxvar; i++) {
        Node* n = nodes_list[i];
        if (n == nullptr) continue;
        // Print node type
        std::cout << "Node " << i << ": type = " << (n->type == NodeType::INPUT ? "INPUT" : "AND") << "\n";

        // Print node inputs if it's an AND gate and whether they're inverted or not
        if (n->type == NodeType::AND) {
            for (int j = 0; j < 2; j++) {
                Edge& e = n->input_nodes[j];
                std::cout << "  Input " << j << " -> Node " << e.node()->variable_number << (e.inverted() ? " (inverted)" : "") << "\n";
            }
        }

        // Skip if there are no outputs for this node
        if (n->output_nodes.size() <= 0) continue;
        // Print the outputs for this node
        std::cout << "Outputs for this node: " << '\n';
        for (int k = 0; k <  n->output_nodes.size(); k++) {
            std::cout << "Node: " << n->output_nodes[k]->variable_number << '\n';
        }
    }

    // Print all the outputs for the entire AIG and whether they are inverted or not
    for (int i = 0; i < outputs.size(); i++) {
        Edge& e = outputs[i];
        std::cout << "Output " << i << " -> ";
        if (e.node() == nullptr) {
            std::cout << (e.inverted() ? "CONST 1" : "CONST 0");
        } else {
            std::cout << "Node " << e.node()->variable_number;
            if (e.inverted()) std::cout << " (inverted)";
        }
        std::cout << '\n';
    }

}
Edge edge_from_literal(unsigned literal, Node* nodes_list[]) {
    // Check if it is a constant literal (Just True or False) or not
    if (literal == 0) return {nullptr, false};
    if (literal == 1) return {nullptr, true};
    // Assign by variable index by /2 via bitshift and check for inversion with mask on LSB
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
    // Ensure no latches present (Script will manually unroll beforehand for sequential circuits being tested)
    if (aig->num_latches > 0) {
        std::cerr << "No latches allowed, unroll first";
        return 1;
    }

    /* Test to see we are printing all nodes from file correctly
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
    */


    // Node Pointers memory
    Node** nodes_list = new Node*[aig->maxvar + 2];
    std::vector<Node*> input_nodes_list;
    // Set memory for all the node pointers
    memset(nodes_list, 0, sizeof(Node*) * (aig->maxvar + 2));

    // AIG arena
    // Use NODE_ALIGNED_SIZE to round to next 16 byte multiple
    size_t nodes_arena_size = NODE_ALIGNED_SIZE * (aig->maxvar + 2);
    void* arena_buffer = malloc(nodes_arena_size);
    Arena<Node> nodes_arena(arena_buffer, nodes_arena_size);

    // add input nodes to graph/circuit
    for (int i = 0; i < aig->num_inputs; i++) {
        const aiger_symbol& input_node = aig->inputs[i];
        const unsigned index = input_node.lit >> 1;
        // Allocate memory and initialise node using the arena
        nodes_list[index] = nodes_arena.alloc();
        nodes_list[index]->type = NodeType::INPUT;
        nodes_list[index]->variable_number = index;
        nodes_list[index]->input_nodes[0] = {nullptr, false};
        nodes_list[index]->input_nodes[1] = {nullptr, false};
        input_nodes_list.push_back(nodes_list[index]);
    }
    // add AND nodes to graph/circuit
    for (int i = 0; i < aig->num_ands; i++) {
        const aiger_and& and_node = aig->ands[i];
        // Index of AND gate output
        const unsigned index = and_node.lhs >> 1;

        // Allocate memory and initialise node using the arena
        nodes_list[index] = nodes_arena.alloc();
        nodes_list[index]->type = NodeType::AND;
        nodes_list[index]->variable_number = index;

        // Ensure variables exist
        if (and_node.rhs0 > 0) assert(nodes_list[and_node.rhs0 >> 1] != nullptr);
        if (and_node.rhs1 > 0) assert(nodes_list[and_node.rhs1 >> 1] != nullptr);

        // Add the 2 inputs to the AND gate as its inputs
        nodes_list[index]->input_nodes[0] = edge_from_literal(and_node.rhs0, nodes_list);
        nodes_list[index]->input_nodes[1] = edge_from_literal(and_node.rhs1, nodes_list);

        // Add the AND gate as an output to the 2 inputs
        if (nodes_list[index]->input_nodes[0].node() != nullptr)
            nodes_list[index]->input_nodes[0].node()->output_nodes.push_back(nodes_list[index]);
        if (nodes_list[index]->input_nodes[1].node() != nullptr)
            nodes_list[index]->input_nodes[1].node()->output_nodes.push_back(nodes_list[index]);
    }

    // outputs
    std::vector<Edge> outputs;
    for (int i = 0; i < aig->num_outputs; i++) {
        unsigned index = aig->outputs[i].lit >> 1;
        bool inverted = (aig->outputs[i].lit & 1) != 0;

        // Handle if output is a constant (True/False)
        if (aig->outputs[i].lit == 0) outputs.push_back(Edge(nullptr, false));
        else if (aig->outputs[i].lit == 1) outputs.push_back(Edge(nullptr, true));
        // Create an edge by converting the AIGER index to our own via /2 and set it if it's inverted or not
        else {
            outputs.push_back(Edge(nodes_list[aig->outputs[i].lit >> 1], (aig->outputs[i].lit & 1) != 0));
        }
    }

    // print structure of AIG to ensure it is correct
    //printInternalAIG(nodes_list, outputs, aig->maxvar);

    // Setup Solver and its internals
    Solver Solver;
    Solver.nodes_list_size = aig->maxvar+2;
    Solver.nodes_list = nodes_list;
    Solver.output_nodes = outputs;
    Solver.input_nodes = input_nodes_list;

    std::string benchmark_file_name = argv[1];
    // Remove path name from benchmark file so it only includes the actual name
    benchmark_file_name.erase(0, 57);
    std::cout << "BENCHMARK: " << benchmark_file_name << '\n';
    //std::cout << "Size of Node struct: " << sizeof(Node) << '\n';
    //std::cout << "TOTAL NODES: " << aig->maxvar+2 << '\n';
    std::cout << "Solving..." << '\n';
    // Track time
    std::chrono::time_point<std::chrono::system_clock> start_time = std::chrono::system_clock::now();

    Solver.run();

    std::chrono::time_point<std::chrono::system_clock> end_time = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double seconds = duration.count() / 1'000'000.0;
    std::cout << "TOTAL TIME: " << std::setprecision(2) << seconds << '\n';
    std::cout << "TOTAL CONFLICTS: " << Solver.solver_conflicts << '\n';
    std::cout << "TOTAL DECISIONS: " << Solver.solver_decisions << '\n';
    std::cout << "TOTAL PROPAGATIONS: " << Solver.solver_propagations << '\n';


    // This freeing is largely unnecessary as solver immediately exits and the OS reclaims all memory
    // Cleanup node as it has internal vector
    for (int i = 0 ; i < aig->maxvar+2; i++) {
        if (nodes_list[i] != nullptr) {
            nodes_list[i]->~Node();
        }
    }
    // Free arena, other memory and reset aiger struct
    free(arena_buffer);
    delete[] nodes_list;
    aiger_reset(aig);

    return 0;
}
