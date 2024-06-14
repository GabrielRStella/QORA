#include "pch.h"
#include "Statistics.h"

void ConfidenceInterval::print(FILE* f) const
{
    fprintf(f, "(%05.3f, %05.3f)", lower, upper);
}

bool ConfidenceInterval::operator>(double d) const
{
    return lower > d;
}

bool ConfidenceInterval::operator<(double d) const
{
    return upper < d;
}

bool ConfidenceInterval::operator>(const ConfidenceInterval& b) const
{
    return lower > b.upper;
}

bool ConfidenceInterval::operator<(const ConfidenceInterval& b) const
{
    return upper < b.lower;
}

double ConfidenceInterval::operator-(const ConfidenceInterval& b) const
{
    return std::max(upper - b.upper, lower - b.lower);
}

//https://en.wikipedia.org/wiki/Binomial_proportion_confidence_interval#Wilson_score_interval
//https://www.fourmilab.ch/rpkp/experiments/analysis/zCalc.js
//https://www.econometrics.blog/post/the-wilson-confidence-interval-for-a-proportion/

double Z_MAX = 6;
double Z_EPSILON = 0.000001;

//P(z) approximation for normal distribution
double poz(double z) {
    double y, x, w;

    if (z == 0.0) {
        x = 0.0;
    }
    else {
        y = 0.5 * abs(z);
        if (y > (Z_MAX * 0.5)) {
            x = 1.0;
        }
        else if (y < 1.0) {
            w = y * y;
            x = ((((((((0.000124818987 * w
                - 0.001075204047) * w + 0.005198775019) * w
                - 0.019198292004) * w + 0.059054035642) * w
                - 0.151968751364) * w + 0.319152932694) * w
                - 0.531923007300) * w + 0.797884560593) * y * 2.0;
        }
        else {
            y -= 2.0;
            x = (((((((((((((-0.000045255659 * y
                + 0.000152529290) * y - 0.000019538132) * y
                - 0.000676904986) * y + 0.001390604284) * y
                - 0.000794620820) * y - 0.002034254874) * y
                + 0.006549791214) * y - 0.010557625006) * y
                + 0.011630447319) * y - 0.009279453341) * y
                + 0.005353579108) * y - 0.002141268741) * y
                + 0.000535310849) * y + 0.999936657524;
        }
    }
    return z > 0.0 ? ((x + 1.0) * 0.5) : ((1.0 - x) * 0.5);
}

//inverse of above; find z s.t. P(z) ~ p
double normal_critical_value(double p) {
    double minz = -Z_MAX;
    double maxz = Z_MAX;
    double zval = 0.0;

    if (p < 0.0 || p > 1.0) {
        return 0;
    }

    while ((maxz - minz) > Z_EPSILON) {
        double pval = poz(zval);
        if (pval > p) {
            maxz = zval;
        }
        else {
            minz = zval;
        }
        zval = (maxz + minz) * 0.5;
    }
    return zval;
}

ConfidenceInterval Statistics::estimate_binomial_interval(std::size_t n, std::size_t ns, double alpha)
{
    std::size_t nf = n - ns;
    double z = normal_critical_value(1 - alpha / 2);
    double z2 = z * z;
    double center = (ns + z2 / 2) / (n + z2);
    double root_part = (double(ns) * double(nf) / n) + (z2 / 4);
    double radius = (z / (n + z2)) * sqrt(root_part);
    //printf("root part: %f, radius: %f, ns * nf: %f\n", root_part, radius, double(ns * nf));
    return ConfidenceInterval{ center - radius, center + radius };
}