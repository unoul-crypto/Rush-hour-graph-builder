#include "../graph_layout.hpp"

#include <cassert>
#include <cmath>

namespace {
float side(LayoutPoint a, LayoutPoint b, LayoutPoint p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

int crossings(const StateGraph& graph, const std::vector<LayoutPoint>& positions) {
    int total = 0;
    const auto& edges = graph.edges();
    for (std::size_t i = 0; i < edges.size(); ++i) {
        for (std::size_t j = i + 1; j < edges.size(); ++j) {
            const auto& a = edges[i];
            const auto& b = edges[j];
            if (a.from == b.from || a.from == b.to || a.to == b.from || a.to == b.to) continue;
            const auto p = positions[a.from], q = positions[a.to];
            const auto r = positions[b.from], s = positions[b.to];
            if (side(p, q, r) * side(p, q, s) < 0 &&
                side(r, s, p) * side(r, s, q) < 0) ++total;
        }
    }
    return total;
}
}

int main() {
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
    for (std::size_t i = 0; i < graph.nodes().size(); ++i) {
        assert(optimized[i].x == graph.nodes()[i].depth * 185.0f);
        for (std::size_t j = i + 1; j < graph.nodes().size(); ++j)
            if (graph.nodes()[i].depth == graph.nodes()[j].depth)
                assert(optimized[i].y != optimized[j].y);
    }
    assert(crossings(graph, optimized) < crossings(graph, original));
    return 0;
}
