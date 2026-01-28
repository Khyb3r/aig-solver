#ifndef AIG_H
#define AIG_H

struct Node;

struct Edge {
    Node* node;
    bool inverted;
};

enum class NodeType {INPUT, AND};

struct Node {
    NodeType type;
    unsigned int variable_number;
    Edge input_nodes[2];
};

#endif //AIG_H
