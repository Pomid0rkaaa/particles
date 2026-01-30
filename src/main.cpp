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

constexpr float MAX_DIST = 150.0f;
constexpr float MAX_DIST_SQ = MAX_DIST * MAX_DIST;
constexpr float MAX_DIST_MODE = 30.0f;
constexpr float MAX_DIST_MODE_SQ = MAX_DIST_MODE * MAX_DIST_MODE;

class Point {
public:
  float x;
  float y;
  float r = 5;
  inline static unsigned int count = 0;
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
  Point::count += size;
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
    if (points.empty()) return;

    int closestIndex = -1;
    float closestDistSq = MAX_DIST_MODE_SQ;

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
      --Point::count;
    }
}

static std::vector<int> closestPoints(Vector2 a, const std::vector<Point>& points, int maxCount = 5) {
  std::vector<std::pair<float,int>> distances;
  distances.reserve(points.size());

  for (int i = 0; i < (int)points.size(); i++) {
    auto b = points[i];
    float distSq = distance(b, a);
    if (distSq <= 0.1f) continue;
    distances.push_back({distSq, i});
  }

  std::sort(distances.begin(), distances.end(), [](auto& lhs, auto& rhs){ return lhs.first < rhs.first; });

  std::vector<int> result;
  for (int i = 0; i < (int)distances.size() && i < maxCount; i++) {
    result.push_back(distances[i].second);
  }
  return result;
}
static std::vector<int> closestPoints(Point& a, const std::vector<Point>& points, int maxCount = 5) {
  return closestPoints({a.x, a.y}, points, maxCount);
}

static void drawLine(Point& a, Point& b) {
  float dx = b.x - a.x;
  float dy = b.y - a.y;
  float dist = distance(a, b);
  if (dist > MAX_DIST_SQ) return;

  float distance = std::sqrt(dist);
  float thickness = 4.0f - (distance / MAX_DIST) * 3.0f;
  thickness = std::max(thickness, 0.5f);

  float alpha = 1.0f - (distance / MAX_DIST);
  int segments = std::max(1, (int)(distance * 0.5f));
  for (int i = 0; i < segments; i++) {
    float t = (float)i / (float)segments;
    Color currentColor = Fade(ColorLerp(a.color, b.color, t), alpha);
    Vector2 segmentStart = { a.x + dx * t, a.y + dy * t };
    Vector2 segmentEnd = { a.x + dx * ((float)i + 1) / (float)segments, a.y + dy * ((float)i + 1) / (float)segments };
    DrawLineEx(segmentStart, segmentEnd, thickness, currentColor);
  }
}
static void drawLine(Point& a, Vector2 b) {
  Point p(b.x, b.y);
  drawLine(a, p);
}

enum Mode { PUSH, ATTRACT, ORBIT, NONE };
static void push(Point& a, Vector2 b, int mode) {
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  float px = -dy;
  float py = dx;
  float mag = std::sqrt(px * px + py * py);
  float distSq = dx * dx + dy * dy;
  float angle = std::atan2(dy, dx);
  if (distSq <= MAX_DIST_MODE_SQ) {
    switch (mode) {
      case Mode::PUSH: a.d = { std::cosf(angle) * 3.0f, std::sinf(angle) * 3.0f }; break;
      case Mode::ATTRACT: a.d = { std::cosf(angle) * 3.0f * -1, std::sinf(angle) * 3.0f * -1 }; break;
      case Mode::ORBIT: if (mag > 0.01f) a.d = { (px / mag) * 2.0f, (py / mag) * 2.0f };
    }
  }
}

struct SplashText {
  std::string text;
  float timer = 0.0f;
  float duration = 1.5f;
} static splash;

int main(void)
{
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(800, 600, "particles");
  SetTargetFPS(60);

  std::vector<Point> points;
  genPoints(points);

  bool isMove = true;
  bool isCircle = false;
  bool isDebug = false;
  int mode = Mode::NONE;

  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    if (splash.timer < splash.duration) splash.timer += 2*dt;

    Vector2 mouse = GetMousePosition();

    switch (GetKeyPressed()) {
      case KEY_SPACE:
        isMove = !isMove;
        splash = {isMove ? "RESUME" : "PAUSE", 0.0f};
        break;
      case KEY_C:
        points.clear();
        splash = {"CLEAR", 0.0f};
        Point::count = 0;
        break;
      case KEY_ONE:
        mode = Mode::NONE;
        splash = {"MODE: NONE", 0.0f};
        break;
      case KEY_TWO:
        mode = Mode::PUSH;
        splash = {"MODE: PUSH", 0.0f};
        break;
      case KEY_THREE:
        mode = Mode::ATTRACT;
        splash = {"MODE: ATTRACT", 0.0f};
        break;
      case KEY_FOUR:
        mode = Mode::ORBIT;
        splash = {"MODE: ORBIT", 0.0f};
        break;
      case KEY_G:
        isCircle = !isCircle;
        break;
      case KEY_D:
        isDebug = !isDebug;
        break;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (IsKeyDown(KEY_LEFT_SHIFT)) {
        for (size_t i = 0; i < 5; i++)
          points.push_back(Point(mouse.x, mouse.y, randColor()));
        Point::count += 5;
      } else {
        points.push_back(Point(mouse.x, mouse.y, randColor()));
        ++Point::count;
      }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      if (IsKeyDown(KEY_LEFT_SHIFT))
        for (size_t i = 0; i < 5; i++)
          deletePoint(mouse, points);
      else deletePoint(mouse, points);
    }

    BeginDrawing();
    ClearBackground(BLACK);
    for (size_t i = 0; i < points.size(); i++) {
      Point& a = points[i];
      if (isMove) a.move();
      a.draw();
      for (int idx : closestPoints(a, points, 5)) {
        drawLine(a, points[idx]);
      }
      push(a, mouse, mode);
    }
    for (int idx : closestPoints(mouse, points, 5)) {
      drawLine(points[idx], mouse);
    }
    if (isCircle) DrawCircleLinesV(mouse, 30, Fade(WHITE, 0.2f));

    if (splash.timer < splash.duration) {
      float alpha = 1.0f - (splash.timer / splash.duration);
      Color c = Fade(WHITE, alpha);

      int fontSize = 30 - 10 * (splash.timer / splash.duration);
      int textWidth = MeasureText(splash.text.c_str(), fontSize);
      int y = GetScreenHeight() - 60 + (int)(20 * (splash.timer / splash.duration));

      DrawText(
        splash.text.c_str(),
        GetScreenWidth()/2 - textWidth/2,
        y,
        fontSize,
        c
      );
    }
    if (isDebug) {
      DrawFPS(4, 4);
      DrawText((std::string("Points: ") + std::to_string(Point::count)).c_str(), 4, 28, 20, RAYWHITE);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
