#ifndef AIG_H
#define AIG_H
#include <vector>

// [0,0,0,0,0,0,1,1]
//              | | flipped bit
//            active bit
#define NODE_FLIPPED_TRUE(x) ((x) & 1)
#define NODE_ACTIVE_TRUE(x) (((x) >> 1) & 1)
#define SET_NODE_ACTIVE(x) ((x) | (1 << 1))
#define SET_NODE_FLIPPED(x) ((x) | (1 << 0))
#define CLEAR_NODE_ACTIVE(x) ((x) & ~(1 << 1))
#define CLEAR_NODE_FLIPPED(x) ((x) & ~(1 << 0))

#define CLEAR_LOWER_THREE_BITS(x) (x)


struct Node;
struct Edge {
    Node* node;                                    // 8 bytes
    bool inverted;                                 // 1 byte
};

struct NewEdge {
    uintptr_t data;
    NewEdge (Node* node, bool inverted) : data(reinterpret_cast<uintptr_t>(node) | inverted) {}

    Node* node() {
        return reinterpret_cast<Node*>(data & ~uintptr_t(1));
    }

    bool inverted() {
        return data & 1;
    }
};

// Force Enums to 1 byte rather than 4 byte default
enum class NodeType {INPUT, AND};
enum class Assignment {TRUE, FALSE, UNASSIGNED};
enum class SavedPhase {TRUE, FALSE, NONE};

struct Node {
    NodeType type;                                  // 1 byte
    unsigned int variable_number;                   // 4 bytes
    Edge input_nodes[2];                            // 32 bytes
    Assignment assignment = Assignment::UNASSIGNED; // 1 byte
    int decision_level = -1;                        // 4 bytes
    std::vector<Node*> output_nodes;                // 24 bytes
    SavedPhase saved_phase = SavedPhase::NONE;      // 1 byte
    bool flipped = false;                           // 1 byte
    float activity = 0.0;                           // 8 bytes
    bool active = false;                            // 1 byte
};
#endif //AIG_H
