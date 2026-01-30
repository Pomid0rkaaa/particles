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

static Color randColor() {
  std::uniform_int_distribution<int> dist(63, 255);
  return {
    (unsigned char)dist(rng),
    (unsigned char)dist(rng),
    (unsigned char)dist(rng),
    255
  };
}

class Point {
public:
  float x;
  float y;
  float r = 5;
  Color color = {140, 220, 140, 255};
  Vector2 d = { randDir(), randDir() };

  Point(float posX, float posY) : x(posX), y(posY) {}
  Point(float posX, float posY, Color c) : x(posX), y(posY), color(c) {}

  void draw() const { DrawCircle(x, y, r, color); }
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
    vec.emplace_back(randX(), randY(), randColor());
}

static float distance(Vector2 a, Vector2 b) {
  float dx = b.x - a.x;
  float dy = b.y - a.y;
  return dx * dx + dy * dy;
}
static float distance(Point& a, Point&b) {
  return distance({a.x, a.y}, {b.x, b.y});
}
static float distance(Point& a, Vector2 b) {
  return distance({a.x, a.y}, b);
}

static void deletePoint(Vector2 coord, std::vector<Point>& points) {
    constexpr float MAX_DIST_SQ = 50.0f * 50.0f;

    if (points.empty()) return;

    int closestIndex = -1;
    float closestDistSq = MAX_DIST_SQ;

    for (int i = 0; i < (int)points.size(); ++i) {
        float dx = points[i].x - coord.x;
        float dy = points[i].y - coord.y;
        float distSq = dx*dx + dy*dy;

        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closestIndex = i;
        }
    }

    if (closestIndex != -1) {
        points.erase(points.begin() + closestIndex);
    }
}

Color ColorLerp(Color color1, Color color2, float t) {
    Color result;
    result.r = color1.r + (color2.r - color1.r) * t;
    result.g = color1.g + (color2.g - color1.g) * t;
    result.b = color1.b + (color2.b - color1.b) * t;
    result.a = color1.a + (color2.a - color1.a) * t;
    return result;
}

static void drawLine(Point& a, Point& b) {
  constexpr float MAX_DIST = 150.0f;
  constexpr float MAX_DIST_SQ = MAX_DIST * MAX_DIST;

  float dist = distance(a, b);
  if (dist <= MAX_DIST_SQ) {
    float distance = std::sqrt(dist);
    float thickness = 4.0f - (distance / MAX_DIST) * 3.0f;
    thickness = std::max(thickness, 0.5f);

    float alpha = 1.0f - (distance / MAX_DIST);
    int segments = (int)distance;
    for (int i = 0; i < segments; i++) {
      float t = (float)i / (float)segments;
      Color currentColor = Fade(ColorLerp(a.color, b.color, t), alpha);
      Vector2 segmentStart = { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
      Vector2 segmentEnd = { a.x + (b.x - a.x) * ((float)i + 1) / (float)segments, a.y + (b.y - a.y) * ((float)i + 1) / (float)segments };
      DrawLineEx(segmentStart, segmentEnd, thickness, currentColor);
    }
  }
}
static void drawLine(Point& a, Vector2 b) {
  Point p(b.x, b.y);
  drawLine(a, p);
}

enum Mode { PUSH, ATTRACT, ORBIT, NONE };
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
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(800, 600, "particles");
  SetTargetFPS(60);

  std::vector<Point> points;
  genPoints(points);

  bool isMove = true;
  bool isHelp = true;
  static constexpr float MAX_DIST_SQ = 150.0f * 150.0f;
  int mode = Mode::NONE;

  while (!WindowShouldClose())
  {
    Vector2 mouse = GetMousePosition();

    switch (GetKeyPressed()) {
      case KEY_SPACE: isMove = !isMove; break;
      case KEY_C: points.clear(); break;
      case KEY_ONE: mode = Mode::NONE; break;
      case KEY_TWO: mode = Mode::PUSH; break;
      case KEY_THREE: mode = Mode::ATTRACT; break;
      case KEY_FOUR: mode = Mode::ORBIT; break;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) points.push_back(Point(mouse.x, mouse.y, randColor()));
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) deletePoint(mouse, points);

    BeginDrawing();
    ClearBackground(BLACK);
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
