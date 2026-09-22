#include "board.hpp"

#include <fstream>
#include <sstream>

Board::Board(int width, int height) : width_(width), height_(height) {}

bool Board::fits(const Vehicle& vehicle) const {
    if (vehicle.length < 1 || vehicle.length > maxSize || vehicle.row < 0 || vehicle.column < 0) return false;
    const int lastRow = vehicle.row + (vehicle.direction == Direction::Vertical ? vehicle.length - 1 : 0);
    const int lastColumn = vehicle.column + (vehicle.direction == Direction::Horizontal ? vehicle.length - 1 : 0);
    return lastRow < height_ && lastColumn < width_;
}

int Board::vehicleAt(int row, int column) const {
    for (int i = 0; i < static_cast<int>(vehicles_.size()); ++i) {
        const Vehicle& vehicle = vehicles_[i];
        for (int offset = 0; offset < vehicle.length; ++offset) {
            const int occupiedRow = vehicle.row + (vehicle.direction == Direction::Vertical ? offset : 0);
            const int occupiedColumn = vehicle.column + (vehicle.direction == Direction::Horizontal ? offset : 0);
            if (occupiedRow == row && occupiedColumn == column) return i;
        }
    }
    return -1;
}

bool Board::add(const Vehicle& vehicle) {
    if (!fits(vehicle)) return false;
    for (int offset = 0; offset < vehicle.length; ++offset) {
        const int row = vehicle.row + (vehicle.direction == Direction::Vertical ? offset : 0);
        const int column = vehicle.column + (vehicle.direction == Direction::Horizontal ? offset : 0);
        if (vehicleAt(row, column) >= 0) return false;
    }
    if (vehicle.target) {
        for (Vehicle& existing : vehicles_) existing.target = false;
    }
    vehicles_.push_back(vehicle);
    return true;
}

void Board::remove(int index) {
    if (index >= 0 && index < static_cast<int>(vehicles_.size())) vehicles_.erase(vehicles_.begin() + index);
}

bool Board::setTarget(int index) {
    if (index < 0 || index >= static_cast<int>(vehicles_.size())) return false;
    for (Vehicle& vehicle : vehicles_) vehicle.target = false;
    vehicles_[index].target = true;
    return true;
}

bool Board::resize(int width, int height) {
    if (width < minSize || width > maxSize || height < minSize || height > maxSize) return false;
    for (const Vehicle& vehicle : vehicles_) {
        const int lastRow = vehicle.row + (vehicle.direction == Direction::Vertical ? vehicle.length - 1 : 0);
        const int lastColumn = vehicle.column + (vehicle.direction == Direction::Horizontal ? vehicle.length - 1 : 0);
        if (lastRow >= height || lastColumn >= width) return false;
    }
    width_ = width;
    height_ = height;
    return true;
}

bool Board::save(const std::string& path) const {
    std::ofstream file(path, std::ios::trunc);
    if (!file) return false;
    file << "RUSH_HOUR_BOARD 1\n" << width_ << ' ' << height_ << '\n' << vehicles_.size() << '\n';
    for (const Vehicle& vehicle : vehicles_) {
        file << vehicle.row << ' ' << vehicle.column << ' '
             << (vehicle.direction == Direction::Horizontal ? 'H' : 'V') << ' '
             << vehicle.length << ' ' << static_cast<int>(vehicle.target) << '\n';
    }
    return file.good();
}

bool Board::load(const std::string& path, Board& output) {
    std::ifstream file(path);
    std::string signature;
    int version = 0;
    int width = 0;
    int height = 0;
    int count = 0;
    if (!(file >> signature >> version >> width >> height >> count) ||
        signature != "RUSH_HOUR_BOARD" || version != 1 ||
        width < minSize || width > maxSize || height < minSize || height > maxSize ||
        count < 0 || count > width * height) return false;

    Board loaded(width, height);
    bool hasTarget = false;
    for (int i = 0; i < count; ++i) {
        Vehicle vehicle;
        char direction = 0;
        int target = 0;
        if (!(file >> vehicle.row >> vehicle.column >> direction >> vehicle.length >> target) ||
            (direction != 'H' && direction != 'V') || (target != 0 && target != 1) ||
            (target == 1 && hasTarget)) return false;
        vehicle.direction = direction == 'H' ? Direction::Horizontal : Direction::Vertical;
        vehicle.target = target == 1;
        if (!loaded.add(vehicle)) return false;
        hasTarget |= vehicle.target;
    }
    std::string trailing;
    if (file >> trailing) return false;
    output = loaded;
    return true;
}
