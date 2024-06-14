#include "pch.h"
#include "FrequencyTable.h"

#include "util.h"

////////////////////////////////////////////////////////////////////////////////
//tabular function approximator
////////////////////////////////////////////////////////////////////////////////

FrequencyTable::FrequencyTable(std::size_t m) : m(m)
{
}

void FrequencyTable::reset()
{
    count_total = 0;
    count_m.clear();
    count_k.clear();
    count_joint.clear();
    frequency_m.clear();
    frequency_k.clear();
    frequency_joint.clear();
    prediction_score = 0;
    success_interval = { 0, 1 };
}

size_t FrequencyTable::getInputStates() const
{
    return m;
}

std::set<std::size_t> FrequencyTable::getObservedInputStates() const
{
    std::set<std::size_t> s;
    for (const auto& pair : count_m) s.insert(pair.first);
    return s;
}

size_t FrequencyTable::getOutputStates() const
{
    return k;
}

size_t FrequencyTable::getCountTotal() const
{
    return count_total;
}

size_t FrequencyTable::getCountInput(std::size_t state_in) const
{
    auto it = count_m.find(state_in);
    if (it != count_m.end()) {
        return it->second;
    }
    return 0;
}

size_t FrequencyTable::getCountOutput(size_t state_out) const
{
    auto it = count_k.find(state_out);
    if (it != count_k.end()) {
        return it->second;
    }
    return 0;
}

size_t FrequencyTable::getCount(std::size_t state_in, size_t state_out) const
{
    auto it = count_joint.find(std::pair<std::size_t, size_t>{state_in, state_out});
    if (it != count_joint.end()) {
        return it->second;
    }
    return 0;
}

double FrequencyTable::getFrequencyInput(std::size_t state_in) const
{
    auto it = frequency_m.find(state_in);
    if (it != frequency_m.end()) {
        return it->second;
    }
    return 0.0;
}

double FrequencyTable::getFrequencyOutput(size_t state_out) const
{
    auto it = frequency_k.find(state_out);
    if (it != frequency_k.end()) {
        return it->second;
    }
    return 0.0;
}

double FrequencyTable::getFrequency(std::size_t state_in, size_t state_out) const
{
    auto it = frequency_joint.find(std::pair<std::size_t, size_t>{state_in, state_out});
    if (it != frequency_joint.end()) {
        return it->second;
    }
    return 0.0;
}

double FrequencyTable::getFrequencyConditional(std::size_t state_in, size_t state_out) const
{
    return getFrequency(state_in, state_out) / getFrequencyInput(state_in);
}

ProbabilityDistribution<size_t> FrequencyTable::getConditionalDistribution(std::size_t state_in) const
{
    ProbabilityDistribution<size_t> dist;

    for (size_t i = 0; i < k; i++) {
        double prob = getFrequencyConditional(state_in, i);
        if (prob > 0) dist.setProbability(i, prob);
    }
    return dist;
}

FrequencyTable FrequencyTable::slice(std::size_t in) const
{
    FrequencyTable c(1);
    //
    //increase k if necessary
    c.k = k;
    //counts
    if (count_m.find(in) != count_m.end()) {
        c.count_total = c.count_m[0] = count_m.at(in);
        for (size_t i = 0; i < k; i++) {
            auto it = count_joint.find({ in, i });
            if (it != count_joint.end()) {
                c.count_joint[{0, i}] = c.count_k[i] = it->second;
            }
        }
    }
    //
    return c;
}

double FrequencyTable::getPredictionScore() const
{
    return prediction_score;
}

ConfidenceInterval FrequencyTable::getSuccessInterval() const
{
    return success_interval;
}

void FrequencyTable::observe(std::size_t state_in, size_t state_out)
{
    //increase k if necessary
    if (state_out >= k) k = state_out + 1;
    //counts
    count_total++;
    count_m[state_in]++;
    count_k[state_out]++;
    count_joint[{state_in, state_out}]++;
}

void FrequencyTable::recalculate(double a)
{
    if (count_total == 0) return;
    //
    for (auto& pair : count_joint) {
        frequency_joint[pair.first] = double(pair.second) / count_total;
    }
    for (auto& pair : count_m) {
        frequency_m[pair.first] = double(pair.second) / count_total;
    }
    for (auto& pair : count_k) {
        frequency_k[pair.first] = double(pair.second) / count_total;
    }
    //recalculate score with paper's formula
    prediction_score = 0;
    for (auto& pair : frequency_m) {
        size_t in = pair.first;
        double f = pair.second;
        //
        double term = 0;
        auto it = frequency_m.find(in);
        if (it != frequency_m.end()) {
            double f = it->second;
            if (f > 0) {
                for (size_t out = 0; out < k; out++) {
                    if (frequency_joint.find({ in, out }) != frequency_joint.end()) {
                        double x = frequency_joint.at({ in, out });
                        term += x * x;
                    }
                }
                prediction_score += term / f;
            }
        }
    }
    //printf("prediction score: %f, count: %zu\n", prediction_score, count_total);
    success_interval = Statistics::estimate_binomial_interval(count_total, prediction_score * count_total, a);
}

size_t FrequencyTable::predict(std::size_t state_in) const
{
    size_t best = 0;
    double best_prob = -1;
    for (size_t i = 0; i < k; i++) {
        double prob = getFrequencyConditional(state_in, i);
        if (prob > best_prob) {
            best = i;
            best_prob = prob;
        }
    }
    return best;
}

double FrequencyTable::confidence(std::size_t state_in) const
{
    size_t best = 0;
    double best_prob = -1;
    for (size_t i = 0; i < k; i++) {
        double prob = getFrequencyConditional(state_in, i);
        if (prob > best_prob) {
            best = i;
            best_prob = prob;
        }
    }
    return best_prob;
}

void FrequencyTable::print(FILE* f) const
{
    //print out m and k, then the table of joint frequencies
    fprintf(f, "Counter(%zu x %zu), success: ", m, k);
    success_interval.print(f);
}

bool FrequencyTable::operator>(const FrequencyTable& b) const
{
    return success_interval > b.success_interval;
}

bool FrequencyTable::operator<(const FrequencyTable& b) const
{
    return success_interval < b.success_interval;
}

void FrequencyTable::to_json(json& j) const
{
    j = json{
        {"m", m},
        {"k", k},
        {"count_total", count_total},
        {"count_m", count_m},
        {"count_k", count_k},
        {"prediction_score", prediction_score}
    };
    json& j_joint = j["count_joint"];
    for (const auto& pair : count_joint) {
        std::string key = std::to_string(pair.first.first) + "," + std::to_string(pair.first.second); //input,outcome
        int value = pair.second; //count
        j_joint[key] = value;
    }
}

void FrequencyTable::from_json(const json& j)
{
    j.at("m").get_to(m);
    j.at("k").get_to(k);
    j.at("count_total").get_to(count_total);
    j.at("count_m").get_to(count_m);
    j.at("count_k").get_to(count_k);
    j.at("prediction_score").get_to(prediction_score);
    count_joint.clear();
    const json& j_joint = j.at("count_joint");
    for (json::const_iterator it = j_joint.cbegin(); it != j_joint.cend(); ++it) {
    	const std::string& key = it.key(); //input,outcome
    	const json& value = it.value(); //count
    	//
        std::vector<std::string> key_split = str_split(key, ",");
        count_joint[std::pair<std::size_t, int>{std::stoll(key_split[0]), std::stoi(key_split[1])}] = value.get<int>();
    }
}
