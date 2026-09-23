#include "graph_view.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr int width = 1100;
constexpr int height = 820;
constexpr int graphTop = 100;
constexpr int graphBottom = 766;
const Color muted{158, 174, 195, 255};
const Color accent{85, 174, 230, 255};
const Color targetColor{233, 103, 94, 255};

Vector2 nodePosition(const GraphNode& node) {
    const int rank = node.layerIndex;
    const int signedRank = rank == 0 ? 0 : (rank % 2 ? (rank + 1) / 2 : -rank / 2);
    return {static_cast<float>(node.depth * 185), static_cast<float>(signedRank * 66)};
}

bool graphButton(Rectangle box, const char* label, Vector2 mouse) {
    const bool hovered = CheckCollisionPointRec(mouse, box);
    DrawRectangleRounded(box, 0.16f, 6, hovered ? Color{57, 72, 91, 255} : Color{43, 55, 71, 255});
    DrawRectangleRoundedLinesEx(box, 0.16f, 6, 1, Color{73, 91, 111, 255});
    DrawText(label, static_cast<int>(box.x + (box.width - MeasureText(label, 20)) / 2),
             static_cast<int>(box.y + 11), 20, RAYWHITE);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void drawTooltip(const StateGraph& graph, int nodeId, Vector2 mouse) {
    const Board& board = graph.board();
    const int cell = std::min(25, 220 / std::max(board.width(), board.height()));
    const int boardWidth = board.width() * cell;
    const int boardHeight = board.height() * cell;
    const int boxWidth = std::max(180, boardWidth + 28);
    const int boxHeight = boardHeight + 82;
    const int x = std::clamp(static_cast<int>(mouse.x) + 20, 8, width - boxWidth - 8);
    const int y = std::clamp(static_cast<int>(mouse.y) + 20, graphTop + 8, height - boxHeight - 8);
    const Rectangle box{static_cast<float>(x), static_cast<float>(y),
                        static_cast<float>(boxWidth), static_cast<float>(boxHeight)};
    DrawRectangleRounded(box, 0.08f, 8, Color{31, 40, 55, 250});
    DrawRectangleRoundedLinesEx(box, 0.08f, 8, 2, accent);
    DrawText(TextFormat("State %d  /  depth %d", nodeId + 1, graph.nodes()[nodeId].depth), x + 14, y + 12, 18, RAYWHITE);

    const int originX = x + 14;
    const int originY = y + 45;
    DrawRectangle(originX - 1, originY - 1, boardWidth + 2, boardHeight + 2, Color{84, 102, 122, 255});
    for (int row = 0; row < board.height(); ++row)
        for (int column = 0; column < board.width(); ++column)
            DrawRectangle(originX + column * cell, originY + row * cell, cell - 1, cell - 1,
                          (row + column) % 2 ? Color{45, 58, 75, 255} : Color{51, 65, 83, 255});

    for (std::size_t i = 0; i < board.vehicles().size(); ++i) {
        const Vehicle vehicle = graph.positionedVehicle(nodeId, i);
        const bool horizontal = vehicle.direction == Direction::Horizontal;
        const Rectangle car{static_cast<float>(originX + vehicle.column * cell + 2),
                            static_cast<float>(originY + vehicle.row * cell + 2),
                            static_cast<float>((horizontal ? vehicle.length : 1) * cell - 5),
                            static_cast<float>((horizontal ? 1 : vehicle.length) * cell - 5)};
        DrawRectangleRounded(car, 0.15f, 6, vehicle.target ? targetColor : Color{84, 166, 198, 255});
    }
}
} // namespace

bool GraphView::draw(const StateGraph& graph, Vector2 mouse) {
    const Rectangle area{0, graphTop, width, graphBottom - graphTop};
    const bool overGraph = CheckCollisionPointRec(mouse, area);
    if (overGraph) {
        const float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            const float nextZoom = std::clamp(zoom * std::pow(1.17f, wheel), 0.03f, 5.0f);
            const Vector2 world{(mouse.x - pan.x) / zoom, (mouse.y - pan.y) / zoom};
            zoom = nextZoom;
            pan = {mouse.x - world.x * zoom, mouse.y - world.y * zoom};
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            const Vector2 delta = GetMouseDelta();
            // The caller scales the virtual UI uniformly to the window.
            const float uiScale = std::min(static_cast<float>(GetScreenWidth()) / width,
                                           static_cast<float>(GetScreenHeight()) / height);
            pan.x += delta.x / uiScale;
            pan.y += delta.y / uiScale;
        }
    }

    DrawRectangleRec(area, Color{22, 31, 44, 255});
    for (const GraphEdge& edge : graph.edges()) {
        const Vector2 a = nodePosition(graph.nodes()[edge.from]);
        const Vector2 b = nodePosition(graph.nodes()[edge.to]);
        const Vector2 start{pan.x + a.x * zoom, pan.y + a.y * zoom};
        const Vector2 end{pan.x + b.x * zoom, pan.y + b.y * zoom};
        if (std::max(start.x, end.x) < 0 || std::min(start.x, end.x) > width ||
            std::max(start.y, end.y) < graphTop || std::min(start.y, end.y) > graphBottom) continue;
        DrawLineEx(start, end, std::max(1.0f, zoom), Color{87, 112, 139, 170});
    }

    int hovered = -1;
    const float radius = std::max(3.0f, 12.0f * zoom);
    for (int i = 0; i < static_cast<int>(graph.nodes().size()); ++i) {
        const Vector2 world = nodePosition(graph.nodes()[i]);
        const Vector2 point{pan.x + world.x * zoom, pan.y + world.y * zoom};
        if (point.x < -radius || point.x > width + radius ||
            point.y < graphTop - radius || point.y > graphBottom + radius) continue;
        const Color fill = i == 0 ? targetColor : accent;
        DrawCircleV(point, radius, fill);
        if (zoom >= 0.65f) DrawText(TextFormat("%d", i + 1), static_cast<int>(point.x + radius + 3),
                                     static_cast<int>(point.y - 7), 13, muted);
        if (overGraph && CheckCollisionPointCircle(mouse, point, std::max(radius, 7.0f))) hovered = i;
    }

    DrawRectangle(0, 0, width, graphTop, Color{29, 38, 52, 255});
    DrawText("STATE GRAPH", 26, 18, 27, RAYWHITE);
    DrawText(TextFormat("%d states  /  %d transitions  /  %s", static_cast<int>(graph.nodes().size()),
                        static_cast<int>(graph.edges().size()), graph.complete() ? "Complete" : "Building..."),
             26, 59, 19, muted);
    const bool back = graphButton({805, 20, 267, 42}, "Back to editor", mouse);
    if (graphButton({695, 20, 98, 42}, "Fit", mouse) || IsKeyPressed(KEY_HOME)) {
        float maxX = 0;
        float maxY = 0;
        for (const GraphNode& node : graph.nodes()) {
            const Vector2 point = nodePosition(node);
            maxX = std::max(maxX, point.x);
            maxY = std::max(maxY, std::abs(point.y));
        }
        zoom = std::clamp(std::min(1000.0f / (maxX + 80), 580.0f / (2 * maxY + 80)), 0.03f, 5.0f);
        pan = {(width - maxX * zoom) / 2.0f, (graphTop + graphBottom) / 2.0f};
    }
    DrawRectangle(0, graphBottom, width, height - graphBottom, Color{29, 38, 52, 255});
    DrawText("Wheel: zoom    Drag: pan    Home: fit    Red: starting state", 26, graphBottom + 17, 18, muted);

    if (hovered >= 0) drawTooltip(graph, hovered, mouse);
    return back || IsKeyPressed(KEY_ESCAPE);
}
