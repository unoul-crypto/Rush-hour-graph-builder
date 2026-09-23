#pragma once

#include "graph.hpp"
#include "graph_layout.hpp"
#include "raylib.h"

#include <vector>

struct GraphView {
    Vector2 pan{135, 410};
    float zoom = 1.0f;
    LayoutMode mode = LayoutMode::FewerCrossings;

    void resetForGraph();

    // Coordinates are in the 1100x820 virtual UI used by the editor.
    // Returns true when the user asks to return to the editor.
    bool draw(const StateGraph& graph, Vector2 mouse);

private:
    std::vector<LayoutPoint> positions_;
    LayoutMode computedMode_ = LayoutMode::Original;
    std::size_t lastNodeCount_ = 0;
    std::size_t lastEdgeCount_ = 0;
    bool lastComplete_ = false;
    double lastUpdate_ = 0;
    bool fittedComplete_ = false;
    int intersectionCount_ = -1;

    Vector2 pointFor(const StateGraph& graph, std::size_t node) const;
};
