#include "board.hpp"
#include "graph.hpp"
#include "graph_view.hpp"

#include "raylib.h"

#include <algorithm>
#include <memory>
#include <string>

namespace {
constexpr int windowWidth = 1100;
constexpr int windowHeight = 820;
constexpr int gridX = 48;
constexpr int gridY = 145;
constexpr int gridSpace = 624;
constexpr int panelX = 735;
constexpr const char* savePath = "board.txt";

const Color background{19, 25, 36, 255};
const Color panel{29, 38, 52, 255};
const Color muted{158, 174, 195, 255};
const Color accent{85, 174, 230, 255};
const Color targetColor{233, 103, 94, 255};
Vector2 uiMouse{};

struct Editor {
    Board board;
    Direction direction = Direction::Horizontal;
    int length = 2;
    int selected = -1;
    std::string message = "Click an empty cell to place a car.";
};

int cellSize(const Board& board) {
    return gridSpace / std::max(board.width(), board.height());
}

bool button(Rectangle bounds, const char* label, bool active = false) {
    const bool hovered = CheckCollisionPointRec(uiMouse, bounds);
    const Color fill = active ? Color{56, 117, 154, 255} : hovered ? Color{57, 72, 91, 255} : Color{43, 55, 71, 255};
    DrawRectangleRounded(bounds, 0.15f, 6, fill);
    DrawRectangleRoundedLinesEx(bounds, 0.15f, 6, 1.0f, active ? accent : Color{73, 91, 111, 255});
    const int fontSize = 20;
    DrawText(label, static_cast<int>(bounds.x + (bounds.width - MeasureText(label, fontSize)) / 2),
             static_cast<int>(bounds.y + (bounds.height - fontSize) / 2), fontSize, RAYWHITE);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void changeSize(Editor& editor, int width, int height) {
    if (editor.board.resize(width, height)) editor.message = "Board size changed.";
    else editor.message = "A car would be outside the board, or size is 3..12.";
}

void drawBoard(Editor& editor) {
    const Board& board = editor.board;
    const int cell = cellSize(board);
    const int boardWidth = board.width() * cell;
    const int boardHeight = board.height() * cell;

    DrawRectangle(gridX - 4, gridY - 4, boardWidth + 8, boardHeight + 8, Color{66, 85, 105, 255});
    DrawRectangle(gridX, gridY, boardWidth, boardHeight, Color{37, 48, 64, 255});
    for (int row = 0; row < board.height(); ++row) {
        for (int column = 0; column < board.width(); ++column) {
            const Rectangle rect{static_cast<float>(gridX + column * cell + 1),
                                 static_cast<float>(gridY + row * cell + 1),
                                 static_cast<float>(cell - 2), static_cast<float>(cell - 2)};
            DrawRectangleRec(rect, (row + column) % 2 ? Color{39, 51, 67, 255} : Color{43, 56, 73, 255});
        }
    }

    for (int i = 0; i < static_cast<int>(board.vehicles().size()); ++i) {
        const Vehicle& vehicle = board.vehicles()[i];
        const bool horizontal = vehicle.direction == Direction::Horizontal;
        const Rectangle rect{static_cast<float>(gridX + vehicle.column * cell + 4),
                             static_cast<float>(gridY + vehicle.row * cell + 4),
                             static_cast<float>((horizontal ? vehicle.length : 1) * cell - 8),
                             static_cast<float>((horizontal ? 1 : vehicle.length) * cell - 8)};
        const Color color = vehicle.target ? targetColor : Color{84, 166, 198, 255};
        DrawRectangleRounded(rect, 0.18f, 8, color);
        DrawRectangleRoundedLinesEx(rect, 0.18f, 8, i == editor.selected ? 4.0f : 1.0f,
                                    i == editor.selected ? RAYWHITE : Color{27, 52, 69, 255});
        const std::string label = vehicle.target ? "T" : std::to_string(i + 1);
        const int size = 23;
        DrawText(label.c_str(), static_cast<int>(rect.x + (rect.width - MeasureText(label.c_str(), size)) / 2),
                 static_cast<int>(rect.y + (rect.height - size) / 2), size, RAYWHITE);
    }

    const Vector2 mouse = uiMouse;
    const int column = static_cast<int>((mouse.x - gridX) / cell);
    const int row = static_cast<int>((mouse.y - gridY) / cell);
    const bool inside = mouse.x >= gridX && mouse.y >= gridY &&
                        column >= 0 && column < board.width() && row >= 0 && row < board.height();
    if (inside && board.vehicleAt(row, column) < 0) {
        Vehicle preview{row, column, editor.length, editor.direction, false};
        bool valid = (preview.direction == Direction::Horizontal ? column + preview.length <= board.width()
                                                                  : row + preview.length <= board.height());
        for (int offset = 0; valid && offset < preview.length; ++offset) {
            const int r = row + (preview.direction == Direction::Vertical ? offset : 0);
            const int c = column + (preview.direction == Direction::Horizontal ? offset : 0);
            valid = board.vehicleAt(r, c) < 0;
        }
        const Rectangle rect{static_cast<float>(gridX + column * cell + 5),
                             static_cast<float>(gridY + row * cell + 5),
                             static_cast<float>((preview.direction == Direction::Horizontal ? preview.length : 1) * cell - 10),
                             static_cast<float>((preview.direction == Direction::Vertical ? preview.length : 1) * cell - 10)};
        if (valid) DrawRectangleRounded(rect, 0.18f, 8, Color{108, 203, 153, 110});
    }

    if (inside && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const int index = board.vehicleAt(row, column);
        if (index >= 0) {
            editor.selected = index;
            editor.message = "Car selected. Press T for target, Del to remove.";
        } else if (editor.board.add(Vehicle{row, column, editor.length, editor.direction, false})) {
            editor.selected = static_cast<int>(editor.board.vehicles().size()) - 1;
            editor.message = "Car added.";
        } else editor.message = "Car does not fit here.";
    }
    if (inside && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        const int index = board.vehicleAt(row, column);
        if (index >= 0) {
            editor.board.remove(index);
            editor.selected = -1;
            editor.message = "Car removed.";
        }
    }
}
} // namespace

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(windowWidth, windowHeight, "Rush Hour - Board Editor");
    SetExitKey(KEY_NULL);
    SetWindowMinSize(700, 520);
    SetTargetFPS(60);
    Editor editor;
    std::unique_ptr<StateGraph> graph;
    GraphView graphView;

    while (!WindowShouldClose()) {
        if (!graph && IsKeyPressed(KEY_H)) editor.direction = Direction::Horizontal;
        if (!graph && IsKeyPressed(KEY_V)) editor.direction = Direction::Vertical;
        if (!graph && IsKeyPressed(KEY_MINUS)) editor.length = std::max(1, editor.length - 1);
        if (!graph && IsKeyPressed(KEY_EQUAL)) editor.length = std::min(Board::maxSize, editor.length + 1);
        if (IsKeyPressed(KEY_F11)) {
            if (IsWindowMaximized()) RestoreWindow();
            else MaximizeWindow();
        }
        if (!graph && IsKeyPressed(KEY_T) && editor.board.setTarget(editor.selected)) editor.message = "Target car selected.";
        if (!graph && IsKeyPressed(KEY_DELETE) && editor.selected >= 0) {
            editor.board.remove(editor.selected);
            editor.selected = -1;
            editor.message = "Car removed.";
        }
        if (!graph && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S))
            editor.message = editor.board.save(savePath) ? "Saved to board.txt." : "Could not save board.txt.";
        if (!graph && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) {
            if (Board::load(savePath, editor.board)) {
                editor.selected = -1;
                editor.message = "Loaded board.txt.";
            } else editor.message = "Could not load board.txt.";
        }

        const float scale = std::min(static_cast<float>(GetScreenWidth()) / windowWidth,
                                     static_cast<float>(GetScreenHeight()) / windowHeight);
        Camera2D camera{};
        camera.offset = { (GetScreenWidth() - windowWidth * scale) / 2.0f,
                          (GetScreenHeight() - windowHeight * scale) / 2.0f };
        camera.zoom = scale;
        uiMouse = GetScreenToWorld2D(GetMousePosition(), camera);
        if (graph && !graph->complete()) graph->step(100);

        BeginDrawing();
        ClearBackground(background);
        BeginMode2D(camera);
        if (graph) {
            if (graphView.draw(*graph, uiMouse)) graph.reset();
        } else {
        DrawText("RUSH HOUR  /  BOARD EDITOR", 48, 36, 30, RAYWHITE);
        DrawText("Create a starting position and build its state graph", 48, 79, 20, muted);
        if (button({static_cast<float>(panelX), 72, 300, 40},
                   graphView.mode == LayoutMode::FewerCrossings ? "Layout: fewer crossings" : "Layout: original")) {
            graphView.mode = graphView.mode == LayoutMode::FewerCrossings ? LayoutMode::Original : LayoutMode::FewerCrossings;
        }
        drawBoard(editor);

        DrawRectangle(panelX - 19, 130, 342, 642, panel);
        DrawText("BOARD SIZE", panelX, 151, 22, RAYWHITE);
        DrawText(TextFormat("Width: %d", editor.board.width()), panelX, 191, 20, muted);
        if (button({panelX + 193.0f, 183, 48, 36}, "-")) changeSize(editor, editor.board.width() - 1, editor.board.height());
        if (button({panelX + 251.0f, 183, 48, 36}, "+")) changeSize(editor, editor.board.width() + 1, editor.board.height());
        DrawText(TextFormat("Height: %d", editor.board.height()), panelX, 236, 20, muted);
        if (button({panelX + 193.0f, 228, 48, 36}, "-")) changeSize(editor, editor.board.width(), editor.board.height() - 1);
        if (button({panelX + 251.0f, 228, 48, 36}, "+")) changeSize(editor, editor.board.width(), editor.board.height() + 1);

        DrawText("NEW CAR", panelX, 300, 22, RAYWHITE);
        DrawText("Direction", panelX, 339, 20, muted);
        if (button({static_cast<float>(panelX), 371, 145, 42}, "Horizontal", editor.direction == Direction::Horizontal)) editor.direction = Direction::Horizontal;
        if (button({static_cast<float>(panelX + 155), 371, 145, 42}, "Vertical", editor.direction == Direction::Vertical)) editor.direction = Direction::Vertical;
        DrawText("Length", panelX, 432, 20, muted);
        if (button({static_cast<float>(panelX), 464, 74, 42}, "-")) editor.length = std::max(1, editor.length - 1);
        DrawText(TextFormat("%d cells", editor.length), panelX + 91, 475, 20, RAYWHITE);
        if (button({static_cast<float>(panelX + 226), 464, 74, 42}, "+")) editor.length = std::min(Board::maxSize, editor.length + 1);

        DrawText("SELECTED CAR", panelX, 545, 22, RAYWHITE);
        if (button({static_cast<float>(panelX), 582, 145, 43}, "Set target"))
            editor.message = editor.board.setTarget(editor.selected) ? "Target car selected." : "Select a car first.";
        if (button({static_cast<float>(panelX + 155), 582, 145, 43}, "Delete")) {
            if (editor.selected >= 0) {
                editor.board.remove(editor.selected);
                editor.selected = -1;
                editor.message = "Car removed.";
            } else editor.message = "Select a car first.";
        }
        if (button({static_cast<float>(panelX), 654, 145, 43}, "Save"))
            editor.message = editor.board.save(savePath) ? "Saved to board.txt." : "Could not save board.txt.";
        if (button({static_cast<float>(panelX + 155), 654, 145, 43}, "Load")) {
            if (Board::load(savePath, editor.board)) {
                editor.selected = -1;
                editor.message = "Loaded board.txt.";
            } else editor.message = "Could not load board.txt.";
        }
        if (button({static_cast<float>(panelX), 713, 300, 45}, "Build graph")) {
            graph = std::make_unique<StateGraph>(editor.board);
            graphView.resetForGraph();
        }

        DrawText("Left click: add/select     Right click: remove", 48, 749, 18, muted);
        DrawText(editor.message.c_str(), 48, 783, 18, accent);
        }
        EndMode2D();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
