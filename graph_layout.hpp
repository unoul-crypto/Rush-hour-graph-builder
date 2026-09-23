#pragma once

#include "graph.hpp"

#include <vector>

enum class LayoutMode { FewerCrossings, ForceDirected, Original };

LayoutMode nextLayoutMode(LayoutMode mode);
const char* layoutModeName(LayoutMode mode);

struct LayoutPoint {
    float x = 0;
    float y = 0;
};

// Layout only changes where states are drawn. Graph topology stays in StateGraph.
std::vector<LayoutPoint> buildLayout(const StateGraph& graph, LayoutMode mode);

// Includes proper crossings and collinear overlaps of unrelated edges.
int countEdgeIntersections(const StateGraph& graph, const std::vector<LayoutPoint>& positions);
