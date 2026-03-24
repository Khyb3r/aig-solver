#ifndef AIG_H
#define AIG_H
#include <vector>

struct Node;
struct Edge {
    Node* node;                                    // 8 bytes
    bool inverted;                                 // 1 byte
};

enum class NodeType {INPUT, AND};
enum class Assignment {TRUE, FALSE, UNASSIGNED};
enum class SavedPhase {TRUE, FALSE, NONE};
struct Node {
    NodeType type;                                  // 4 bytes
    unsigned int variable_number;                   // 4 bytes
    Edge input_nodes[2];                            // 32 bytes

    Assignment assignment = Assignment::UNASSIGNED; // 4 bytes
    int decision_level = -1;                        // 4 bytes

    std::vector<Node*> output_nodes;                // 16 bytes

    SavedPhase saved_phase = SavedPhase::NONE;
};

#endif //AIG_H
