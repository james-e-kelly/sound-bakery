#pragma once

#include <cmath>

namespace sbk::maths
{
    constexpr double PI = 3.1415926545;

    auto ease_in_sine(double t) -> double { return sin(1.5707963 * t); }

    auto ease_out_sine(double t) -> double { return 1 + sin(1.5707963 * (--t)); }

    auto ease_in_out_sine(double t) -> double { return 0.5 * (1 + sin(3.1415926 * (t - 0.5))); }

    auto ease_in_quad(double t) -> double { return t * t; }

    auto ease_out_quad(double t) -> double { return t * (2 - t); }

    auto ease_in_out_quad(double t) -> double { return t < 0.5 ? 2 * t * t : t * (4 - 2 * t) - 1; }

    auto ease_in_cubic(double t) -> double { return t * t * t; }

    auto ease_out_cubic(double t) -> double { return 1 + (--t) * t * t; }

    auto ease_in_out_cubic(double t) -> double { return t < 0.5 ? 4 * t * t * t : 1 + (--t) * (2 * (--t)) * (2 * t); }

    auto ease_in_quart(double t) -> double
    {
        t *= t;
        return t * t;
    }

    auto ease_out_quart(double t) -> double
    {
        t = (--t) * t;
        return 1 - t * t;
    }

    auto ease_in_out_quart(double t) -> double
    {
        if (t < 0.5)
        {
            t *= t;
            return 8 * t * t;
        }
        else
        {
            t = (--t) * t;
            return 1 - 8 * t * t;
        }
    }

    auto ease_in_quint(double t) -> double
    {
        double t2 = t * t;
        return t * t2 * t2;
    }

    auto ease_out_quint(double t) -> double
    {
        double t2 = (--t) * t;
        return 1 + t * t2 * t2;
    }

    auto ease_in_out_quint(double t) -> double
    {
        double t2;
        if (t < 0.5)
        {
            t2 = t * t;
            return 16 * t * t2 * t2;
        }
        else
        {
            t2 = (--t) * t;
            return 1 + 16 * t * t2 * t2;
        }
    }

    auto ease_in_expo(double t) -> double { return (pow(2, 8 * t) - 1) / 255; }

    auto ease_out_expo(double t) -> double { return 1 - pow(2, -8 * t); }

    auto ease_in_out_expo(double t) -> double
    {
        if (t < 0.5)
        {
            return (pow(2, 16 * t) - 1) / 510;
        }
        else
        {
            return 1 - 0.5 * pow(2, -16 * (t - 0.5));
        }
    }

    auto ease_in_circ(double t) -> double { return 1 - sqrt(1 - t); }

    auto ease_out_circ(double t) -> double { return sqrt(t); }

    auto ease_in_out_circ(double t) -> double
    {
        if (t < 0.5)
        {
            return (1 - sqrt(1 - 2 * t)) * 0.5;
        }
        else
        {
            return (1 + sqrt(2 * t - 1)) * 0.5;
        }
    }

    auto ease_in_back(double t) -> double { return t * t * (2.70158 * t - 1.70158); }

    auto ease_out_back(double t) -> double { return 1 + (--t) * t * (2.70158 * t + 1.70158); }

    auto ease_in_out_back(double t) -> double
    {
        if (t < 0.5)
        {
            return t * t * (7 * t - 2.5) * 2;
        }
        else
        {
            return 1 + (--t) * t * 2 * (7 * t + 2.5);
        }
    }

    auto ease_in_elastic(double t) -> double
    {
        double t2 = t * t;
        return t2 * t2 * sin(t * PI * 4.5);
    }

    auto ease_out_elastic(double t) -> double
    {
        double t2 = (t - 1) * (t - 1);
        return 1 - t2 * t2 * cos(t * PI * 4.5);
    }

    auto ease_in_out_elastic(double t) -> double
    {
        double t2;
        if (t < 0.45)
        {
            t2 = t * t;
            return 8 * t2 * t2 * sin(t * PI * 9);
        }
        else if (t < 0.55)
        {
            return 0.5 + 0.75 * sin(t * PI * 4);
        }
        else
        {
            t2 = (t - 1) * (t - 1);
            return 1 - 8 * t2 * t2 * sin(t * PI * 9);
        }
    }

    auto ease_in_bounce(double t) -> double { return pow(2, 6 * (t - 1)) * abs(sin(t * PI * 3.5)); }

    auto ease_out_bounce(double t) -> double { return 1 - pow(2, -6 * t) * abs(cos(t * PI * 3.5)); }

    auto ease_in_out_bounce(double t) -> double
    {
        if (t < 0.5)
        {
            return 8 * pow(2, 8 * (t - 1)) * abs(sin(t * PI * 7));
        }
        else
        {
            return 1 - 8 * pow(2, -8 * t) * abs(sin(t * PI * 7));
        }
    }
}  // namespace sbk::maths