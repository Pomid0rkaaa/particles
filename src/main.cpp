#include <raylib.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>

std::mt19937 rng(std::random_device{}());
static int randX() {
  std::uniform_int_distribution<int> dist(5, GetScreenWidth() - 5);
  return dist(rng);
}

static int randY() {
  std::uniform_int_distribution<int> dist(5, GetScreenHeight() - 5);
  return dist(rng);
}

static float randDir() {
  std::uniform_real_distribution<float> dist(-1, 1);
  return dist(rng);
}

class Point {
public:
  float x;
  float y;
  float r = 5;
  Vector2 d = { randDir(), randDir() };

  Point(float posX, float posY) : x(posX), y(posY) {}

  void draw() const { DrawCircle(x, y, r, Color{ 255, 201, 40, 255 }); }
  void move() {
    x += d.x;
    y += d.y;

    const float w = GetScreenWidth();
    const float h = GetScreenHeight();

    // CHATGPT MOMENT ALERT
    auto bounce = [](float& p, float& v, float lo, float hi) {
      if (p <= lo || p >= hi) {
        p = std::clamp(p, lo, hi);
        v = std::clamp(-v + randDir() * 0.2f, -3.0f, 3.0f);
      }
    };

    bounce(x, d.x, r, w - r);
    bounce(y, d.y, r, h - r);
    // CHATGPT END
  }
};

static void genPoints(std::vector<Point>& vec) {
  int size = 50;
  vec.reserve(vec.size() + size);
  for (int i = 0; i < size; i++)
    vec.emplace_back(randX(), randY());
}

static void drawLine(float ax, float ay, float bx, float by) {
  constexpr float MAX_DIST = 150.0f;
  constexpr float MAX_DIST_SQ = MAX_DIST * MAX_DIST;

  float dx = bx - ax;
  float dy = by - ay;
  float distSq = dx * dx + dy * dy;

  if (distSq <= MAX_DIST_SQ) {
    float distance = std::sqrt(distSq);
    float thickness = 4.0f - (distance / MAX_DIST) * 3.0f;
    thickness = std::max(thickness, 0.5f);

    float alpha = 1.0f - (distance / MAX_DIST);
    Color c = Fade(ORANGE, alpha);

    DrawLineEx({ ax, ay }, { bx, by }, thickness, c);
  }
}
static void drawLine(Point& a, Point& b) {
  drawLine(a.x, a.y, b.x, b.y);
}
static void drawLine(Point& a, Vector2 b) {
  drawLine(a.x, a.y, b.x, b.y);
}

enum Mode { PUSH, ATTRACT, ORBIT };
static void push(Point& a, Vector2 b, int mode) {
  static constexpr float MAX_DIST = 30.0f;
  static constexpr float MAX_DIST_SQ = MAX_DIST * MAX_DIST;
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  float px = -dy;
  float py = dx;
  float mag = std::sqrt(px * px + py * py);
  float distSq = dx * dx + dy * dy;
  float angle = std::atan2(dy, dx);
  if (distSq <= MAX_DIST_SQ) {
    switch (mode) {
      case Mode::PUSH: a.d = { std::cosf(angle) * 3.0f, std::sinf(angle) * 3.0f }; break;
      case Mode::ATTRACT: a.d = { std::cosf(angle) * 3.0f * -1, std::sinf(angle) * 3.0f * -1 }; break;
      case Mode::ORBIT: a.d = { px / mag ,py / mag };
    }
  }
}

int main(void)
{
  InitWindow(800, 600, "particles");
  SetTargetFPS(60);

  std::vector<Point> points;
  genPoints(points);

  bool isMove = true;
  bool isHelp = true;
  static constexpr float MAX_DIST_SQ = 150.0f * 150.0f;
  int mode = Mode::PUSH;

  while (!WindowShouldClose())
  {
    if (IsKeyPressed(KEY_SPACE)) isMove = !isMove;
    if (IsKeyPressed(KEY_H)) isHelp = !isHelp;
    if (IsKeyPressed(KEY_G)) mode = ++mode % 3;
    Vector2 mouse = GetMousePosition();
    BeginDrawing();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

    for (size_t i = 0; i < points.size(); i++) {
      Point& a = points[i];
      if (isMove) a.move();
      a.draw();
      for (size_t j = i + 1; j < points.size(); j++) {
        drawLine(a, points[j]);
      }
      push(a, mouse, mode);
      drawLine(a, mouse);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
