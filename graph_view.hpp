#pragma once

#include "graph.hpp"
#include "raylib.h"

struct GraphView {
    Vector2 pan{135, 410};
    float zoom = 1.0f;

    // Coordinates are in the 1100x820 virtual UI used by the editor.
    // Returns true when the user asks to return to the editor.
    bool draw(const StateGraph& graph, Vector2 mouse);
};
