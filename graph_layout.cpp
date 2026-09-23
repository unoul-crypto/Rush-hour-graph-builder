#include "graph_layout.hpp"

#include <algorithm>
#include <vector>

namespace {
constexpr float layerGap = 185.0f;
constexpr float nodeGap = 66.0f;
}

std::vector<LayoutPoint> buildLayout(const StateGraph& graph, LayoutMode mode) {
    const auto& nodes = graph.nodes();
    std::vector<LayoutPoint> result(nodes.size());
    if (mode == LayoutMode::Original) {
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            const int rank = nodes[i].layerIndex;
            const int signedRank = rank == 0 ? 0 : (rank % 2 ? (rank + 1) / 2 : -rank / 2);
            result[i] = {nodes[i].depth * layerGap, signedRank * nodeGap};
        }
        return result;
    }

    int maxDepth = 0;
    for (const GraphNode& node : nodes) maxDepth = std::max(maxDepth, node.depth);
    std::vector<std::vector<int>> layers(maxDepth + 1);
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) layers[nodes[i].depth].push_back(i);

    std::vector<std::vector<int>> neighbors(nodes.size());
    for (const GraphEdge& edge : graph.edges()) {
        neighbors[edge.from].push_back(edge.to);
        neighbors[edge.to].push_back(edge.from);
    }
    std::vector<int> rank(nodes.size());
    std::vector<double> score(nodes.size());
    for (const auto& layer : layers)
        for (int i = 0; i < static_cast<int>(layer.size()); ++i) rank[layer[i]] = i;

    // Barycentric sweeps reduce crossings between adjacent BFS layers.
    // Ties keep their previous order, which prevents needless rearrangement.
    auto reorder = [&](int depth, int adjacent) {
        auto& layer = layers[depth];
        for (int id : layer) {
            double sum = 0;
            int count = 0;
            for (int other : neighbors[id]) {
                if (nodes[other].depth == adjacent) {
                    sum += rank[other];
                    ++count;
                }
            }
            score[id] = count ? sum / count : rank[id];
        }
        std::stable_sort(layer.begin(), layer.end(), [&](int left, int right) {
            return score[left] < score[right];
        });
        for (int i = 0; i < static_cast<int>(layer.size()); ++i) rank[layer[i]] = i;
    };
    for (int pass = 0; pass < 4; ++pass) {
        for (int depth = 1; depth <= maxDepth; ++depth) reorder(depth, depth - 1);
        for (int depth = maxDepth - 1; depth >= 0; --depth) reorder(depth, depth + 1);
    }

    for (int depth = 0; depth <= maxDepth; ++depth) {
        const float center = (static_cast<float>(layers[depth].size()) - 1.0f) / 2.0f;
        for (int id : layers[depth]) result[id] = {depth * layerGap, (rank[id] - center) * nodeGap};
    }
    return result;
}
