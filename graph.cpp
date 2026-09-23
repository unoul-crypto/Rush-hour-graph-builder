#include "graph.hpp"

#include <array>
#include <cstdint>

StateGraph::StateGraph(const Board& initial) : board_(initial) {
    std::string positions;
    positions.reserve(board_.vehicles().size());
    for (const Vehicle& vehicle : board_.vehicles())
        positions.push_back(static_cast<char>(vehicle.direction == Direction::Horizontal ? vehicle.column : vehicle.row));
    nodes_.push_back({positions, 0, 0});
    stateIds_.emplace(positions, 0);
    layerCounts_.push_back(1);
}

Vehicle StateGraph::positionedVehicle(std::size_t node, std::size_t vehicle) const {
    Vehicle result = board_.vehicles()[vehicle];
    const int position = static_cast<unsigned char>(nodes_[node].positions[vehicle]);
    if (result.direction == Direction::Horizontal) result.column = position;
    else result.row = position;
    return result;
}

void StateGraph::step(std::size_t budget) {
    for (std::size_t i = 0; i < budget && !complete(); ++i) {
        const int nodeId = static_cast<int>(nextToExpand_++);
        expand(nodeId);
    }
}

void StateGraph::expand(int nodeId) {
    // Copy: discovering a new state can reallocate nodes_.
    const std::string positions = nodes_[nodeId].positions;
    const int depth = nodes_[nodeId].depth;
    std::array<int16_t, Board::maxSize * Board::maxSize> occupied;
    occupied.fill(-1);
    for (int i = 0; i < static_cast<int>(board_.vehicles().size()); ++i) {
        const Vehicle& base = board_.vehicles()[i];
        const int position = static_cast<unsigned char>(positions[i]);
        for (int offset = 0; offset < base.length; ++offset) {
            const int row = base.direction == Direction::Vertical ? position + offset : base.row;
            const int column = base.direction == Direction::Horizontal ? position + offset : base.column;
            occupied[row * board_.width() + column] = static_cast<int16_t>(i);
        }
    }

    for (int i = 0; i < static_cast<int>(board_.vehicles().size()); ++i) {
        const Vehicle& vehicle = board_.vehicles()[i];
        const int start = static_cast<unsigned char>(positions[i]);
        const int limit = vehicle.direction == Direction::Horizontal ? board_.width() : board_.height();
        for (int direction : {-1, 1}) {
            for (int distance = 1;; ++distance) {
                const int next = start + direction * distance;
                const int checked = direction < 0 ? next : next + vehicle.length - 1;
                if (checked < 0 || checked >= limit) break;
                const int row = vehicle.direction == Direction::Vertical ? checked : vehicle.row;
                const int column = vehicle.direction == Direction::Horizontal ? checked : vehicle.column;
                if (occupied[row * board_.width() + column] != -1) break;

                std::string moved = positions;
                moved[i] = static_cast<char>(next);
                const auto found = stateIds_.find(moved);
                int otherId;
                if (found == stateIds_.end()) {
                    otherId = static_cast<int>(nodes_.size());
                    stateIds_.emplace(moved, otherId);
                    if (static_cast<int>(layerCounts_.size()) <= depth + 1) layerCounts_.push_back(0);
                    nodes_.push_back({std::move(moved), depth + 1, layerCounts_[depth + 1]++});
                } else otherId = found->second;

                // Each legal move is reversible; record each undirected edge once.
                if (nodeId < otherId) edges_.push_back({nodeId, otherId, i, direction * distance});
            }
        }
    }
}
