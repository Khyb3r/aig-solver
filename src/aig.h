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



struct Node;
struct Edge {
    // Tagged Pointer (LSB is inverted field)
    uintptr_t edge;                                // 8 bytes

    Edge (Node* node, bool inverted) : edge(reinterpret_cast<uintptr_t>(node) | inverted) {}
    // Default constructor for initial Edge setup
    Edge() : edge(0) {}

    // Mask LSB to get original Node pointer
    Node* node() const {
        return reinterpret_cast<Node*>(edge & ~uintptr_t(1));
    }

    // Is inverted or not
    bool inverted() const {
        return edge & 1;
    }

};

// Force Enums to 1 byte rather than 4 byte default
enum class NodeType : uint8_t {INPUT, AND};
enum class Assignment : uint8_t {TRUE, FALSE, UNASSIGNED};
enum class SavedPhase : uint8_t {TRUE, FALSE, NONE};

struct Node {
    NodeType type;                                  // 1 byte
    Assignment assignment = Assignment::UNASSIGNED; // 1 byte
    SavedPhase saved_phase = SavedPhase::NONE;      // 1 byte
    uint8_t flipped_and_active_field = 0;           // 1 byte           // 4
    unsigned int variable_number;                   // 4 bytes
    int decision_level = -1;                        // 4 bytes
    float activity = 0.0;                           // 4 bytes          // 12
    Edge input_nodes[2];                            // 16 bytes         // 16
    std::vector<Node*> output_nodes;                // 24 bytes         // 24
};
#endif //AIG_H
