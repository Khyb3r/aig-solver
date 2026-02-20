#ifndef AIG_H
#define AIG_H

struct Node;

struct Edge {
    Node* node;                                    // 8 bytes
    bool inverted;                                 // 1 byte
};

enum class NodeType {INPUT, AND};
enum class Assignment {TRUE, FALSE, UNASSIGNED};

struct Node {
    NodeType type;                                  // 4 bytes
    unsigned int variable_number;                   // 4 bytes
    Edge input_nodes[2];                            // 32 bytes

    Assignment assignment = Assignment::UNASSIGNED; // 4 bytes
    unsigned int decision_level = 0;                // 4 bytes
    Node* reason = nullptr;                         // 8 bytes
    Node* reason_two = nullptr;                       // 8 bytes

    std::vector<Node*> output_nodes;             // 16 bytes
};

struct Literal {
    Node* node;
    bool is_negated;
};

struct ConflictReason {
    std::vector<Literal*> nodes_involved;
};

#endif //AIG_H
