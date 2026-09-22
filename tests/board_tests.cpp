#include "../board.hpp"

#include <cassert>
#include <fstream>
#include <string>

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string path = argv[1];
    Board board;
    assert(board.width() == 6 && board.height() == 6);
    assert(board.add({2, 1, 2, Direction::Horizontal, true}));
    assert(!board.add({2, 2, 3, Direction::Vertical, false}));
    assert(!board.add({0, 5, 2, Direction::Horizontal, false}));
    assert(board.add({0, 0, 3, Direction::Vertical, false}));
    assert(board.add({4, 4, 2, Direction::Horizontal, false}));
    assert(board.vehicleAt(2, 1) == 0);
    assert(board.vehicleAt(2, 0) == 1);
    assert(!board.resize(3, 3));
    assert(board.setTarget(1));
    assert(!board.vehicles()[0].target && board.vehicles()[1].target);
    assert(board.save(path));

    Board loaded;
    assert(Board::load(path, loaded));
    assert(loaded.vehicles().size() == 3);
    assert(loaded.vehicles()[1].target);
    assert(loaded.vehicleAt(2, 0) == 1);

    std::ofstream invalid(path, std::ios::trunc);
    invalid << "RUSH_HOUR_BOARD 1\n6 6\n2\n0 0 H 2 0\n0 1 V 2 0\n";
    invalid.close();
    assert(!Board::load(path, loaded));
    assert(loaded.vehicles().size() == 3);

    Board small(3, 3);
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column)
            assert(small.add({row, column, 1, Direction::Horizontal, false}));
    assert(small.save(path));
    assert(Board::load(path, loaded));
    assert(loaded.vehicles().size() == 9);

    Board large(12, 12);
    assert(large.add({0, 0, 12, Direction::Vertical, false}));
    assert(!large.add({0, 1, 13, Direction::Horizontal, false}));
    assert(large.save(path));
    assert(Board::load(path, loaded));
    assert(loaded.vehicles()[0].length == 12);
    return 0;
}
