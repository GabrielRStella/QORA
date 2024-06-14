#pragma once

#include "Random.h"

template<typename T>
class ProbabilityDistribution
{
public:
	//takes n distributions
	//and weights them equally to produce one distribution
	//i.e., it adds them all together then divides all probabilities by n
	//(it actually just calls normalize, not /n)
	static ProbabilityDistribution<T> add(const std::set<ProbabilityDistribution<T>>& distributions);
	static ProbabilityDistribution<T> add(const ProbabilityDistribution<ProbabilityDistribution<T>>& distributions); //allows weighted combination

private:
	std::map<T, double> probabilities;
public:
	ProbabilityDistribution();
	ProbabilityDistribution(const T& singleton); //create a singleton distribution with P[t]=1
	//raw access to inner data
	void clear();
	size_t size() const;
	std::map<T, double>& getProbabilities();
	const std::map<T, double>& getProbabilities() const;
	//sample a random value from the distribution
	const T& sample(Random& random) const;
	const T& max() const;
	//
	double getTotalProbability() const; //sum the probabilities (hopefully this is 1 :))
	void normalize(); //treat all current "probabilities" as weights and normalize the sum to 1 [unless it is currently 0]
	//
	void setProbability(const T& item, double probability); //probabilities[item] = probability
	void addProbability(const T& item, double probability); //probabilities[item] += probability
	double getProbability(const T& item) const; //probabilities[item]
	void add(const T& item); //adds item with probability 1 so the distribution can later be normalized
	//for use in map (recursive probability distributions, for weighted combinations)
	bool operator==(const ProbabilityDistribution<T>& other) const;
	bool operator<(const ProbabilityDistribution<T>& other) const;
};

template<typename T>
static inline ProbabilityDistribution<T> ProbabilityDistribution<T>::add(const std::set<ProbabilityDistribution<T>>& distributions)
{
	ProbabilityDistribution<T> dist;
	for (const ProbabilityDistribution<T>& dist2 : distributions) {
		for (const auto& pair : dist2.probabilities) {
			dist.addProbability(pair.first, pair.second);
		}
	}
	dist.normalize();
	return dist;
}

template<typename T>
inline ProbabilityDistribution<T> ProbabilityDistribution<T>::add(const ProbabilityDistribution<ProbabilityDistribution<T>>& distributions)
{
	ProbabilityDistribution<T> dist;
	for (const auto& pair_dist : distributions.getProbabilities()) {
		const ProbabilityDistribution<T>& dist2 = pair_dist.first;
		double dist_probability = pair_dist.second;
		for (const auto& pair : dist2.probabilities) {
			dist.addProbability(pair.first, pair.second * dist_probability);
		}
	}
	dist.normalize();
	return dist;
}

template<typename T>
inline ProbabilityDistribution<T>::ProbabilityDistribution()
{
}

template<typename T>
inline ProbabilityDistribution<T>::ProbabilityDistribution(const T& singleton)
{
	probabilities[singleton] = 1;
}

template<typename T>
inline void ProbabilityDistribution<T>::clear()
{
	probabilities.clear();
}

template<typename T>
inline size_t ProbabilityDistribution<T>::size() const
{
	return probabilities.size();
}

template<typename T>
inline std::map<T, double>& ProbabilityDistribution<T>::getProbabilities()
{
	return probabilities;
}

template<typename T>
inline const std::map<T, double>& ProbabilityDistribution<T>::getProbabilities() const
{
	return probabilities;
}

template<typename T>
inline const T& ProbabilityDistribution<T>::sample(Random& random) const
{
	double rng = random.random_uniform() * getTotalProbability();
	for (const auto& pair : probabilities) {
		rng -= pair.second;
		if (rng <= 0) return pair.first;
	}
	assert(false);
}

template<typename T>
inline const T& ProbabilityDistribution<T>::max() const
{
	const T* max_ptr = nullptr;
	double max_val = -1;
	for (const auto& pair : probabilities) {
		if (pair.second > max_val) {
			max_ptr = &pair.first;
			max_val = pair.second;
		}
	}
	return *max_ptr;
}

template<typename T>
inline double ProbabilityDistribution<T>::getTotalProbability() const
{
	double sum = 0;
	for (const auto& pair : probabilities) {
		sum += pair.second;
	}
	return sum;
}

template<typename T>
inline void ProbabilityDistribution<T>::normalize()
{
	double total = getTotalProbability();
	if (total == 0) return;
	//
	for (auto& pair : probabilities) {
		pair.second /= total;
	}
}

template<typename T>
inline void ProbabilityDistribution<T>::setProbability(const T& item, double probability)
{
	if (probability == 0) {
		probabilities.erase(item);
	}
	else {
		probabilities[item] = probability;
	}
}

template<typename T>
inline void ProbabilityDistribution<T>::addProbability(const T& item, double probability)
{
	setProbability(item, getProbability(item) + probability);
}

template<typename T>
inline double ProbabilityDistribution<T>::getProbability(const T& item) const
{
	auto it = probabilities.find(item);
	if (it == probabilities.end()) return 0;
	return it->second;
}

template<typename T>
inline void ProbabilityDistribution<T>::add(const T& item)
{
	probabilities[item] = 1;
}

template<typename T>
inline bool ProbabilityDistribution<T>::operator==(const ProbabilityDistribution<T>& other) const
{
	return probabilities == other.probabilities;
}

template<typename T>
inline bool ProbabilityDistribution<T>::operator<(const ProbabilityDistribution<T>& other) const
{
	return probabilities < other.probabilities;
}
