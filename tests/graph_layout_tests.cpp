#include "../graph_layout.hpp"

#include <cassert>
#include <cmath>
#include <string>

int main() {
    assert(nextLayoutMode(LayoutMode::FewerCrossings) == LayoutMode::ForceDirected);
    assert(nextLayoutMode(LayoutMode::ForceDirected) == LayoutMode::Original);
    assert(nextLayoutMode(LayoutMode::Original) == LayoutMode::FewerCrossings);
    assert(std::string(layoutModeName(LayoutMode::ForceDirected)) == "force-directed");

    Board board(3, 3);
    assert(board.add({0, 0, 1, Direction::Horizontal, false}));
    assert(board.add({2, 0, 1, Direction::Horizontal, false}));
    StateGraph graph(board);
    while (!graph.complete()) graph.step(10);

    const auto original = buildLayout(graph, LayoutMode::Original);
    const auto optimized = buildLayout(graph, LayoutMode::FewerCrossings);
    assert(original.size() == graph.nodes().size());
    assert(optimized.size() == graph.nodes().size());
    assert(original[0].x == 0 && original[0].y == 0);
    bool usesPlane = false;
    for (std::size_t i = 0; i < graph.nodes().size(); ++i)
        usesPlane |= optimized[i].x != graph.nodes()[i].depth * 185.0f;
    assert(usesPlane);
    assert(countEdgeIntersections(graph, optimized) < countEdgeIntersections(graph, original));

    Board threeCars(3, 3);
    for (int row = 0; row < 3; ++row)
        assert(threeCars.add({row, 0, 1, Direction::Horizontal, false}));
    StateGraph larger(threeCars);
    while (!larger.complete()) larger.step(10);
    const auto oldPositions = buildLayout(larger, LayoutMode::Original);
    const int oldCrossings = countEdgeIntersections(larger, oldPositions);
    const int newCrossings = countEdgeIntersections(larger, buildLayout(larger, LayoutMode::FewerCrossings));
    assert(newCrossings * 2 < oldCrossings);
    const auto forced = buildLayout(larger, LayoutMode::ForceDirected);
    assert(forced.size() == larger.nodes().size());
    assert(forced[0].x == 0 && forced[0].y == 0);
    bool distinctLayout = false;
    for (std::size_t i = 0; i < forced.size(); ++i) {
        assert(std::isfinite(forced[i].x) && std::isfinite(forced[i].y));
        distinctLayout |= forced[i].x != oldPositions[i].x || forced[i].y != oldPositions[i].y;
        for (std::size_t j = i + 1; j < forced.size(); ++j)
            assert(forced[i].x != forced[j].x || forced[i].y != forced[j].y);
    }
    assert(distinctLayout);
    assert(countEdgeIntersections(larger, forced) < oldCrossings);

    Board fourCars(3, 4);
    for (int row = 0; row < 4; ++row)
        assert(fourCars.add({row, 0, 1, Direction::Horizontal, false}));
    StateGraph biggest(fourCars);
    while (!biggest.complete()) biggest.step(100);
    const int fourOld = countEdgeIntersections(biggest, buildLayout(biggest, LayoutMode::Original));
    const int fourNew = countEdgeIntersections(biggest, buildLayout(biggest, LayoutMode::FewerCrossings));
    assert(fourNew * 3 < fourOld * 2);
    return 0;
}
