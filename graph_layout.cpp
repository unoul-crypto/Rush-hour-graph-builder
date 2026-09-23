#include "graph_layout.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

namespace {
constexpr float layerGap = 185.0f;
constexpr float nodeGap = 66.0f;

double squaredDistance(LayoutPoint a, LayoutPoint b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

double side(LayoutPoint a, LayoutPoint b, LayoutPoint c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool crosses(LayoutPoint a, LayoutPoint b, LayoutPoint c, LayoutPoint d) {
    if (std::max(a.x, b.x) < std::min(c.x, d.x) || std::max(c.x, d.x) < std::min(a.x, b.x) ||
        std::max(a.y, b.y) < std::min(c.y, d.y) || std::max(c.y, d.y) < std::min(a.y, b.y)) return false;
    const double ac = side(a, b, c);
    const double ad = side(a, b, d);
    const double ca = side(c, d, a);
    const double cb = side(c, d, b);
    if (ac * ad < -0.001 && ca * cb < -0.001) return true;
    auto onSegment = [](LayoutPoint p, LayoutPoint start, LayoutPoint end) {
        return p.x >= std::min(start.x, end.x) - 0.001f &&
               p.x <= std::max(start.x, end.x) + 0.001f &&
               p.y >= std::min(start.y, end.y) - 0.001f &&
               p.y <= std::max(start.y, end.y) + 0.001f;
    };
    return (std::abs(ac) < 0.001 && onSegment(c, a, b)) ||
           (std::abs(ad) < 0.001 && onSegment(d, a, b)) ||
           (std::abs(ca) < 0.001 && onSegment(a, c, d)) ||
           (std::abs(cb) < 0.001 && onSegment(b, c, d));
}

double pointToSegmentSquared(LayoutPoint p, LayoutPoint a, LayoutPoint b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double lengthSquared = dx * dx + dy * dy;
    if (lengthSquared < 0.001) return squaredDistance(p, a);
    const double t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / lengthSquared, 0.0, 1.0);
    const LayoutPoint nearest{static_cast<float>(a.x + t * dx), static_cast<float>(a.y + t * dy)};
    return squaredDistance(p, nearest);
}

int countCrossings(const StateGraph& graph, const std::vector<LayoutPoint>& positions) {
    int count = 0;
    const auto& edges = graph.edges();
    for (std::size_t i = 0; i < edges.size(); ++i) {
        const auto& a = edges[i];
        for (std::size_t j = i + 1; j < edges.size(); ++j) {
            const auto& b = edges[j];
            if (a.from == b.from || a.from == b.to || a.to == b.from || a.to == b.to) continue;
            if (crosses(positions[a.from], positions[a.to], positions[b.from], positions[b.to])) ++count;
        }
    }
    return count;
}

void improveOnPlane(const StateGraph& graph, std::vector<LayoutPoint>& positions,
                    const std::vector<std::vector<int>>& neighbors) {
    const auto& edges = graph.edges();
    const int nodeCount = static_cast<int>(positions.size());
    const int edgeCount = static_cast<int>(edges.size());
    // The exact crossing checks below are useful for small and medium graphs.
    // Keep large, still-growing graphs responsive by using the layered heuristic.
    if (nodeCount > 400 || edgeCount > 1600 ||
        (!graph.complete() && (nodeCount > 160 || edgeCount > 500))) return;

    std::vector<std::vector<int>> incident(nodeCount);
    for (int i = 0; i < edgeCount; ++i) {
        incident[edges[i].from].push_back(i);
        incident[edges[i].to].push_back(i);
    }
    std::vector<int> order;
    order.reserve(nodeCount - 1);
    for (int i = 1; i < nodeCount; ++i) order.push_back(i); // Keep the starting state at the origin.
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return incident[a].size() > incident[b].size();
    });

    auto localScore = [&](int node, LayoutPoint candidate) {
        double score = 0;
        for (int edgeId : incident[node]) {
            const GraphEdge& edge = edges[edgeId];
            const int other = edge.from == node ? edge.to : edge.from;
            const LayoutPoint end = positions[other];
            score += 0.001 * squaredDistance(candidate, end);
            for (int i = 0; i < edgeCount; ++i) {
                const GraphEdge& compared = edges[i];
                if (compared.from == node || compared.to == node ||
                    compared.from == other || compared.to == other) continue;
                if (crosses(candidate, end, positions[compared.from], positions[compared.to]))
                    score += 100000.0;
            }
        }
        for (int i = 0; i < nodeCount; ++i) {
            if (i == node) continue;
            const double d2 = squaredDistance(candidate, positions[i]);
            if (d2 < 55.0 * 55.0) score += 250.0 * (1.0 - std::sqrt(d2) / 55.0);
        }
        for (int i = 0; i < edgeCount; ++i) {
            const GraphEdge& edge = edges[i];
            if (edge.from == node || edge.to == node) continue;
            const LayoutPoint a = positions[edge.from];
            const LayoutPoint b = positions[edge.to];
            if (candidate.x < std::min(a.x, b.x) - 25 || candidate.x > std::max(a.x, b.x) + 25 ||
                candidate.y < std::min(a.y, b.y) - 25 || candidate.y > std::max(a.y, b.y) + 25) continue;
            const double d2 = pointToSegmentSquared(candidate, a, b);
            if (d2 < 25.0 * 25.0) score += 150.0 * (1.0 - std::sqrt(d2) / 25.0);
        }
        return score;
    };

    const std::vector<LayoutPoint> startingPositions = positions;
    const int originalCrossings = countCrossings(graph, positions);
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(graph.complete() ? (nodeCount <= 100 ? 400 : 250) : 30);
    bool timedOut = false;
    const int passes = edgeCount <= 500 ? 2 : 1;
    const std::vector<float> steps = edgeCount <= 500 ? std::vector<float>{130, 65, 32}
                                                      : std::vector<float>{100, 45};
    for (float step : steps) {
        for (int pass = 0; pass < passes; ++pass) {
            bool changed = false;
            for (int node : order) {
                if (std::chrono::steady_clock::now() >= deadline) {
                    timedOut = true;
                    break;
                }
                const LayoutPoint current = positions[node];
                LayoutPoint best = current;
                double bestScore = localScore(node, current);
                auto tryPoint = [&](LayoutPoint candidate) {
                    const double score = localScore(node, candidate);
                    if (score + 0.01 < bestScore) {
                        bestScore = score;
                        best = candidate;
                    }
                };
                for (int dx = -1; dx <= 1; ++dx)
                    for (int dy = -1; dy <= 1; ++dy)
                        if (dx || dy) tryPoint({current.x + dx * step, current.y + dy * step});
                if (!neighbors[node].empty()) {
                    LayoutPoint average{};
                    for (int other : neighbors[node]) {
                        average.x += positions[other].x;
                        average.y += positions[other].y;
                    }
                    average.x /= neighbors[node].size();
                    average.y /= neighbors[node].size();
                    tryPoint(average);
                    tryPoint({(current.x + average.x) / 2, (current.y + average.y) / 2});
                }
                if (best.x != current.x || best.y != current.y) {
                    positions[node] = best;
                    changed = true;
                }
            }
            if (timedOut || !changed) break;
            std::reverse(order.begin(), order.end());
        }
        if (timedOut) break;
    }
    auto crossingsForNode = [&](int node, LayoutPoint candidate) {
        int total = 0;
        for (int edgeId : incident[node]) {
            const GraphEdge& edge = edges[edgeId];
            const int other = edge.from == node ? edge.to : edge.from;
            for (const GraphEdge& compared : edges) {
                if (compared.from == node || compared.to == node ||
                    compared.from == other || compared.to == other) continue;
                if (crosses(candidate, positions[other], positions[compared.from], positions[compared.to]))
                    ++total;
            }
        }
        return total;
    };

    // Bounded annealing lets a vertex temporarily move through a worse drawing
    // to escape a local minimum. Crossing deltas only inspect incident edges.
    if (graph.complete() && nodeCount <= 100 && edgeCount <= 500 && nodeCount > 2) {
        unsigned int randomState = 0x9e3779b9u;
        auto randomUnit = [&]() {
            randomState = randomState * 1664525u + 1013904223u;
            return static_cast<double>(randomState) / 4294967296.0;
        };
        int currentCrossings = countCrossings(graph, positions);
        int bestCrossings = currentCrossings;
        std::vector<LayoutPoint> bestPositions = positions;
        for (int attempt = 0; attempt < 20000 && std::chrono::steady_clock::now() < deadline; ++attempt) {
            const int first = 1 + static_cast<int>(randomUnit() * (nodeCount - 1));
            const LayoutPoint before = positions[first];
            const int beforeLocal = crossingsForNode(first, before);
            const float step = 100.0f * (1.0f - static_cast<float>(attempt) / 20000.0f) + 25.0f;
            LayoutPoint candidate{before.x + static_cast<float>((randomUnit() - 0.5) * 2 * step),
                                  before.y + static_cast<float>((randomUnit() - 0.5) * 2 * step)};
            bool tooClose = false;
            for (int i = 0; i < nodeCount; ++i)
                if (i != first && squaredDistance(candidate, positions[i]) < 30.0 * 30.0) tooClose = true;
            if (tooClose) continue;
            const int delta = crossingsForNode(first, candidate) - beforeLocal;
            const double temperature = 4.0 * (1.0 - static_cast<double>(attempt) / 20000.0) + 0.15;
            const bool accept = delta <= 0 || randomUnit() < std::exp(-delta / temperature);
            if (accept) {
                positions[first] = candidate;
                currentCrossings += delta;
                if (currentCrossings < bestCrossings) {
                    bestCrossings = currentCrossings;
                    bestPositions = positions;
                }
            }
        }
        positions = std::move(bestPositions);
    }
    // Never return a drawing with more actual crossings than the layered start.
    if (countCrossings(graph, positions) > originalCrossings) positions = startingPositions;
}
}

int countEdgeIntersections(const StateGraph& graph, const std::vector<LayoutPoint>& positions) {
    return countCrossings(graph, positions);
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
    improveOnPlane(graph, result, neighbors);
    return result;
}
