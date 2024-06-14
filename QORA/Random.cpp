#include "pch.h"
#include "Random.h"

#include "util.h"

Random::Random() : current_seed(0)
{
}

void Random::seed(std::default_random_engine::result_type seed)
{
    eng.seed(seed);
    current_seed = seed;
}

void Random::seed_time(std::default_random_engine::result_type offset)
{
    seed(QPC() + offset);
}

std::default_random_engine::result_type Random::get_seed() const
{
    return current_seed;
}

int Random::random_int(int end)
{
    return random_int(0, end - 1);
}

int Random::random_int(int a, int b)
{
    return std::uniform_int_distribution<int>(a, b)(eng);
}

double Random::random_uniform()
{
    return std::uniform_real_distribution<double>()(eng);
}
