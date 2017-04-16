#include <deque>
#include <SFML/Graphics.hpp>

#define WIN_WIDTH 640
#define WIN_HEIGHT 480
#define size_factor 40
#define COLUMNS (WIN_WIDTH / size_factor)
#define ROWS (WIN_HEIGHT / size_factor)

sf::Vector2i handle_input(const sf::Event event) {
    switch (event.key.code) {
        case sf::Keyboard::Left:
        case sf::Keyboard::H:
            return sf::Vector2i{-1, 0};
        case sf::Keyboard::Up:
        case sf::Keyboard::K:
            return sf::Vector2i{0, -1};
        case sf::Keyboard::Right:
        case sf::Keyboard::L:
            return sf::Vector2i{1, 0};
        case sf::Keyboard::Down:
        case sf::Keyboard::J:
            return sf::Vector2i{0, 1};
        default:
            return sf::Vector2i{0, 0};
    }
}

sf::Vector2i clamp(const sf::Vector2i val, const sf::Vector2i boundary) {
    if (val.x == boundary.x)
        return clamp(sf::Vector2i{0, val.y}, boundary);
    else if (val.x < 0)
        return clamp(sf::Vector2i{boundary.x - 1, val.y}, boundary);
    if (val.y == boundary.y)
        return clamp(sf::Vector2i{val.x, 0}, boundary);
    else if (val.y < 0)
        return clamp(sf::Vector2i{val.x, boundary.y - 1}, boundary);
    return val;
}

template<typename Container, class T>
bool contains(const Container & container, const T & t) {
    for (const auto element : container) {
        if (element == t)
            return true;
    }
    return false;
}

struct Board {
    sf::Vector2f cell_size{WIN_WIDTH / COLUMNS, WIN_HEIGHT / ROWS};
    std::deque<sf::Vector2i> snake;
    sf::Vector2i apple;
    sf::Vector2i direction;
    int rng_seed = 0;
};

std::array<sf::Vertex, 4> quad(const sf::Vector2f pos, const sf::Vector2f size,
        const sf::Color color)
{
    const float x = size.x, y = size.y;
    return std::array<sf::Vertex, 4>{
        sf::Vertex{pos, color}, sf::Vertex{pos + sf::Vector2f{x, 0}, color},
        sf::Vertex{pos + size, color},
        sf::Vertex{pos + sf::Vector2f{0, x}, color}};
}

sf::VertexArray vertices(const Board & board) {
    sf::VertexArray vertices{sf::Quads};
    const float pad = 1;
    const auto cell_color = sf::Color{200, 210, 240};
    const auto snake_color = sf::Color{110, 240, 100};
    const auto apple_color = sf::Color{240, 100, 110};
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLUMNS; x++) {
            const auto size = board.cell_size;
            const auto pos = sf::Vector2f(x * size.x, y * size.y);
            const auto color = contains(board.snake, sf::Vector2i{x, y})
                             ? snake_color : cell_color;
            for (const auto vertex : quad(pos + sf::Vector2f{pad, pad},
                                     size - sf::Vector2f{2 * pad, 2 * pad},
                                     color))
                vertices.append(vertex);
            if (board.apple == sf::Vector2i{x, y}) {
                for (const auto vert : quad(pos + sf::Vector2f{pad, pad},
                                       size - sf::Vector2f{2 * pad, 2 * pad},
                                       apple_color))
                    vertices.append(vert);
            }
        }
    } 
    return vertices;
}

Board init_board(const sf::Vector2i dir) {
    Board board;
    board.direction = dir;
    return board;
}

template<typename T>
Board init_snake(const Board & board, const T t) {
    Board _board = board;
    _board.snake.push_front(t);
    return _board;
}

template<typename T, typename... Args>
Board init_snake(const Board & board, const T t, Args... args) {
    return init_snake(init_snake(board, t), args...);
}

Board init_apple(const Board & board, const std::size_t seed) {
    std::deque<sf::Vector2i> available_cells;
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLUMNS; x++) {
            if (!contains(board.snake, sf::Vector2i{x, y}))
                available_cells.emplace_back(x, y);
        }
    }
    std::mt19937 rng;
    rng.seed(seed);
    std::uniform_int_distribution<int> dist(0, available_cells.size() - 1);
    auto _board = board;
    _board.apple = available_cells[dist(rng)];
    return _board;
}

Board update_snake(const Board & board, const sf::Vector2i dir) {
    auto next = board;
    if (board.snake.front() == board.apple) {
        next.rng_seed++;
        next.snake.push_back(next.snake.back()); // counteract pop_back
        return update_snake(init_apple(next, next.rng_seed), dir);
    }
    if (dir != sf::Vector2i{0, 0} && dir != -1 * board.direction)
        next.direction = dir;
    next.snake.push_front(clamp(board.snake.front() + next.direction,
                          sf::Vector2i{COLUMNS, ROWS}));
    next.snake.pop_back();
    return next;
}

bool game_over(const Board & board) {
    for (auto it = ++board.snake.cbegin(); it != board.snake.cend(); it++) {
        if (*it == board.snake.front()) // crashed into itself
            return true;
    }
    if (board.snake.size() + 1 == COLUMNS * ROWS) // win
        return true;
    else
        return false;
}

int main() {
    sf::RenderWindow window{sf::VideoMode{WIN_WIDTH, WIN_HEIGHT}, "funkysnake"};
    window.setFramerateLimit(60);

START:
    auto board = init_apple(init_snake(init_board(
        sf::Vector2i{1, 0}), // initial direction <- right
        sf::Vector2i{0, 0}), // initial snake segment coordinates
        std::random_device()()); // initialize board seed and apple position

    while (window.isOpen()) {
        sf::Clock clock;
        sf::Vector2i cmd{0, 0};
        while (clock.getElapsedTime() < sf::milliseconds(100)) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Q
                    || event.key.code == sf::Keyboard::Space
                    || event.key.code == sf::Keyboard::Return)
                        window.close();
                    else if (event.key.code == sf::Keyboard::R)
                        goto START;
                    else
                        cmd = handle_input(event);
                }
            }
            window.clear(sf::Color::White);
            window.draw(vertices(board));
            window.display();
        }
        if (game_over(board))
            goto START;
        else
            board = update_snake(board, cmd);
    }
}
