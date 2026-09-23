#include "../graph.hpp"

#include <cassert>
#include <cstdlib>

int main() {
    Board single(3, 3);
    assert(single.add({1, 0, 2, Direction::Horizontal, true}));
    StateGraph first(single);
    assert(first.nodes().size() == 1 && !first.complete());
    first.step(1);
    assert(first.nodes().size() == 2 && first.edges().size() == 1);
    assert(!first.complete());
    first.step(1);
    assert(first.complete());
    assert(first.positionedVehicle(1, 0).column == 1);
    assert(first.positionedVehicle(1, 0).target);

    Board openLane(6, 3);
    assert(openLane.add({1, 0, 2, Direction::Horizontal, false}));
    StateGraph fullLane(openLane);
    while (!fullLane.complete()) fullLane.step(1);
    assert(fullLane.nodes().size() == 5);
    assert(fullLane.edges().size() == 10); // Every free destination is one move away.

    Board independent(3, 3);
    assert(independent.add({0, 0, 1, Direction::Horizontal, false}));
    assert(independent.add({2, 0, 1, Direction::Horizontal, false}));
    StateGraph grid(independent);
    while (!grid.complete()) grid.step(2);
    assert(grid.nodes().size() == 9);
    assert(grid.edges().size() == 18);
    for (const GraphEdge& edge : grid.edges()) {
        assert(edge.from < edge.to);
        const auto& before = grid.nodes()[edge.from].positions;
        const auto& after = grid.nodes()[edge.to].positions;
        int changed = 0;
        for (std::size_t i = 0; i < before.size(); ++i)
            if (before[i] != after[i]) ++changed;
        assert(changed == 1);
        assert(std::abs(edge.distance) >= 1);
    }

    Board blocked(3, 3);
    assert(blocked.add({1, 0, 2, Direction::Horizontal, false}));
    assert(blocked.add({0, 2, 3, Direction::Vertical, false}));
    StateGraph immobile(blocked);
    immobile.step(10);
    assert(immobile.complete());
    assert(immobile.nodes().size() == 1 && immobile.edges().empty());
    return 0;
}
