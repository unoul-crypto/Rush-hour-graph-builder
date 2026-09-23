#pragma once

#include "board.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

// A state stores one varying coordinate per vehicle, in the board's vehicle order.
// Horizontal vehicles store their column; vertical vehicles store their row.
struct GraphNode {
    std::string positions;
    int depth = 0;
    int layerIndex = 0;
};

struct GraphEdge {
    int from = 0;
    int to = 0;
    int vehicle = 0;
    int distance = 0;
};

class StateGraph {
public:
    explicit StateGraph(const Board& initial);

    const Board& board() const { return board_; }
    const std::vector<GraphNode>& nodes() const { return nodes_; }
    const std::vector<GraphEdge>& edges() const { return edges_; }
    bool complete() const { return nextToExpand_ == nodes_.size(); }
    std::size_t expanded() const { return nextToExpand_; }

    // Expand at most budget states. Repeated calls eventually build the full graph.
    void step(std::size_t budget);
    Vehicle positionedVehicle(std::size_t node, std::size_t vehicle) const;

private:
    Board board_;
    std::vector<GraphNode> nodes_;
    std::vector<GraphEdge> edges_;
    std::unordered_map<std::string, int> stateIds_;
    std::vector<int> layerCounts_;
    std::size_t nextToExpand_ = 0;

    void expand(int nodeId);
};
