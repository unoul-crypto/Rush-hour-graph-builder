#pragma once

#include <string>
#include <vector>

enum class Direction { Horizontal, Vertical };

struct Vehicle {
    int row = 0;
    int column = 0;
    int length = 2;
    Direction direction = Direction::Horizontal;
    bool target = false;
};

class Board {
public:
    static constexpr int minSize = 3;
    static constexpr int maxSize = 12;

    Board(int width = 6, int height = 6);

    int width() const { return width_; }
    int height() const { return height_; }
    const std::vector<Vehicle>& vehicles() const { return vehicles_; }

    bool resize(int width, int height);
    bool add(const Vehicle& vehicle);
    void remove(int index);
    bool setTarget(int index);
    int vehicleAt(int row, int column) const;
    bool save(const std::string& path) const;
    static bool load(const std::string& path, Board& output);

private:
    int width_;
    int height_;
    std::vector<Vehicle> vehicles_;

    bool fits(const Vehicle& vehicle) const;
};
