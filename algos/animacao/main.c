#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

const int IWIDTH = 80;
const int IHEIGHT = 22;
const float WIDTH = (float)IWIDTH;
const float HEIGHT = (float)IHEIGHT;

typedef enum
{
    PIXEL_BG = 0,
    PIXEL_FG = 1,
} Pixel;

static Pixel *screen;

typedef struct
{
    float x;
    float y;
} Vec2;

Vec2 vec2_sub(Vec2 self, Vec2 other)
{
    return (Vec2){self.x - other.x, self.y - other.y};
}
Vec2 vec2_add(Vec2 self, Vec2 other)
{
    return (Vec2){self.x + other.x, self.y + other.y};
}
Vec2 vec2_scale(Vec2 self, float scalar)
{
    return (Vec2){self.x * scalar, self.y * scalar};
}

typedef struct
{
    Vec2 center;
    Vec2 velocity;
    float radius;
} Ball;

Ball new_ball(float radius, Vec2 center, Vec2 velocity)
{
    return (Ball){center, velocity, radius};
}

void ball_draw(Ball self)
{
    // find our circle's boundaries
    const Vec2 radius_vec = (Vec2){self.radius, self.radius};
    const Vec2 top_l = vec2_sub(self.center, radius_vec);
    const Vec2 bot_r = vec2_add(self.center, radius_vec);

    for (float y = floor(top_l.y); y < ceil(bot_r.y); y += 1)
    {
        if (y < 0 || y > HEIGHT)
            continue;

        for (float x = floor(top_l.x); x < ceil(bot_r.x); x += 1)
        {
            if (x < 0 || x >= WIDTH)
                continue;
            // calc the distance between the center and the position
            const Vec2 pos = (Vec2){x + 0.5, y + 0.5};
            const Vec2 d = vec2_sub(self.center, pos);
            // check if the distance is inside the circle
            // x^2 + y^2 <= r^2
            if (d.x * d.x + d.y * d.y <= self.radius * self.radius)
            {
                screen[(int)(y * WIDTH + x)] = PIXEL_FG;
            }
        }
    }
}

typedef struct
{
    Vec2 start;
    Vec2 end;

    float angle;
    float slope;
    float y_intercept;
} Line;

Line new_line(Vec2 start, Vec2 end)
{
    float dy = end.y - start.y;
    float dx = end.x - start.x;
    float angle = atan2(dy, dx);
    // m = (y-y0)/(x-x0)
    float slope = dy / dx;

    // y = m(x-x0) + y0
    float y_intercept = slope * (0 - start.x) + start.y;

    return (Line){start, end, angle, slope, y_intercept};
}

float line_fx(Line self, float x)
{
    const float lx = MIN(self.end.x, self.start.x);
    const float rx = MAX(self.end.x, self.start.x);

    if (x >= lx || x <= rx)
    {
        return self.slope * x + self.y_intercept;
    }
    return INFINITY;
}

void line_draw(Line self)
{
    const Vec2 top_l = (Vec2){MIN(self.end.x, self.start.x), MIN(self.end.y, self.start.y)};
    const Vec2 bot_r = (Vec2){MAX(self.end.x, self.start.x), MAX(self.end.y, self.start.y)};

    const int bar_width = 2;

    for (float y = floor(top_l.y); y < ceil(bot_r.y); y += 1)
    {
        if (y < 0 || y >= HEIGHT)
            continue;

        for (float x = floor(top_l.x); x < ceil(bot_r.x); x += 1)
        {
            if (x < 0 || x >= WIDTH)
                continue;

            const float maybe_fx = line_fx(self, x);
            if (maybe_fx != INFINITY)
            {
                const float yx = maybe_fx;
                const float pos = yx - y;
                if (pos >= -bar_width && pos <= 0)
                {
                    screen[(int)(y * WIDTH + x)] = PIXEL_FG;
                }
            }
        }
    }
}

void fill_screen(Pixel pixel)
{
    memset(screen, pixel, WIDTH * HEIGHT * sizeof(Pixel));
}

// fg the screen state compressing each two rows into a single row
// as follow:
//
// | first row | second row | results in |
// | :-------: | :--------: | :--------: |
// |     .     |     .      |   <SPACE>  |
// |     *     |     .      |      ^     |
// |     .     |     *      |      _     |
// |     *     |     *      |      S     |
//
//  For instance, if we have
//
// *.***....*.*
// .****...*.**
//
//  then, it becomes
//
// ^_SSS   _^_^
void render()
{
    const char char_table[] = " _^S";

    for (int y = 0; y < IHEIGHT; y += 2)
    {

        for (int x = 0; x < IWIDTH; x += 1)
        {
            const int top = screen[(y + 0) * IWIDTH + x];
            const int bot = screen[(y + 1) * IWIDTH + x];
            // we can safely trust that `top` and `bot` will be either 0 or 1
            const char ch = char_table[top * 2 + bot];
            printf("%c", ch);
        }
        printf("\n");
    }
}

void clear()
{
    int r;
#ifdef WIN32
    r = system("cls");
#else
    r = system("clear");
#endif
}

int main()
{
    screen = (Pixel *)malloc(WIDTH * HEIGHT * sizeof(Pixel));

    const int fps = 30;
    const Vec2 gravity = (Vec2){0, 120};
    const float dt = 1.0 / (float)(fps);

    const float radius = HEIGHT / 4;
    const Vec2 circle_pos = (Vec2){radius, 0};

    const Vec2 lb_start = (Vec2){-radius, radius};
    const Vec2 lb_end = (Vec2){WIDTH / 2, HEIGHT};

    const Line lbar = new_line(lb_start, lb_end);
    // const Line rbar = new_line((Vec2){WIDTH / 2, HEIGHT}, (Vec2){WIDTH + radius, radius});
    Ball ball = new_ball(radius, circle_pos, (Vec2){0, 0});

    // printf("right bar slope: %f\n", rbar.slope);

    while (1)
    {
        clear();

        // render the current frame
        fill_screen(PIXEL_BG);

        ball_draw(ball);
        line_draw(lbar);
        // line_draw(rbar);

        render();
        // reset_cursor();

        // update the variables
        // V = V0 + g*Δt
        ball.velocity = vec2_add(ball.velocity, vec2_scale(gravity, dt));
        // S = S0 + V*Δt
        ball.center = vec2_add(ball.center, vec2_scale(ball.velocity, dt));
        // collide with the ground
        if (ball.center.y >= HEIGHT - ball.radius)
        {
            ball.center.y = HEIGHT - ball.radius;
            ball.velocity.y *= -0.98;
            ball.velocity.x *= 0.98;
        }

        // collide with the left slope
        {
            const float fx = line_fx(lbar, ball.center.x);
            if (fx != INFINITY)
            {
                if (ball.center.y >= fx - ball.radius)
                {
                    ball.center.y = fx - ball.radius;

                    const float vx = sin(lbar.angle) * (ball.velocity.x + lbar.slope * gravity.y / 2) * 1.2;
                    const float vy = -cos(lbar.angle) * ball.velocity.y * 1.8;

                    ball.velocity = vec2_add(ball.velocity, (Vec2){vx, vy});
                    ball.velocity.x *= 0.99;
                }
            }
        }

        // collide with the right slope
        // {
        //     const float fx = line_fx(rbar, ball.center.x);
        //     if (fx != INFINITY)
        //     {
        //         if (ball.center.y >= fx - ball.radius)
        //         {
        //             ball.center.y = fx - ball.radius;

        //             const float vx = sin(rbar.angle) * (ball.velocity.x + rbar.slope * gravity.y / 2) * 1.2;
        //             const float vy = -cos(rbar.angle) * ball.velocity.y * 1.8;

        //             ball.velocity = vec2_add(ball.velocity, (Vec2){vx, vy});

        //             printf("velocity: %f,%f\n", vx, ball.velocity.y);
        //             ball.velocity.x *= 0.99;
        //         }
        //     }
        // }

        // wait for the next frame
        // sleep_ms(1000 / fps);

        if (ball.center.x > WIDTH + ball.radius + 2)
        {
            // wait 500 ms to release next ball
            // sleep_ms(500);
            ball.center = (Vec2){ball.radius, 0};
            ball.velocity = (Vec2){0, 0};
        }

#ifdef WIN32
        getch();
#else
        getchar();
#endif
    }

    return 0;
}