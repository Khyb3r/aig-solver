#ifndef AIG_H
#define AIG_H
#include <vector>

struct Node;
struct Edge {
    Node* node;                                    // 8 bytes
    bool inverted;                                 // 1 byte
};

// Force Enums to 1 byte rather than 4 byte default
enum class NodeType : uint8_t {INPUT, AND};
enum class Assignment : uint8_t {TRUE, FALSE, UNASSIGNED};
enum class SavedPhase : uint8_t {TRUE, FALSE, NONE};

struct Node {
    NodeType type;                                  // 1 byte
    unsigned int variable_number;                   // 4 bytes
    Edge input_nodes[2];                            // 32 bytes

    Assignment assignment = Assignment::UNASSIGNED; // 1 byte
    int decision_level = -1;                        // 4 bytes

    std::vector<Node*> output_nodes;                // 24 bytes

    SavedPhase saved_phase = SavedPhase::NONE;      // 1 byte

    bool flipped = false;                           // 1 byte

    float activity = 0.0;                          // 8 bytes

    bool active = false;                            // 1 byte
};

#endif //AIG_H
