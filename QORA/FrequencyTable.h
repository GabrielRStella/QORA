#pragma once

#include "Statistics.h"
#include "ProbabilityDistribution.h"

////////////////////////////////////////////////////////////////////////////////
//tabular function approximator
////////////////////////////////////////////////////////////////////////////////

class FrequencyTable {
	//
	std::size_t m; //number of input states
	size_t k = 2; //number of outcomes (may change dynamically as observations are made, begins at 2 since that's the minimum interesting number)
	//counts, from calls to observe()
	std::size_t count_total = 0; //total number of calls to observe()
	std::map<std::size_t, size_t> count_m; //number of calls to observe() per input value
	std::map<size_t, size_t> count_k; //number of calls to observer() per outcome value
	std::map<std::pair<std::size_t, size_t>, size_t> count_joint; //counts indexed by [input value, outcome value]
	//frequencies, estimated from above
	std::map<std::size_t, double> frequency_m; //number of calls to observe() per input value
	std::map<size_t, double> frequency_k; //number of calls to observer() per outcome value
	std::map<std::pair<std::size_t, size_t>, double> frequency_joint; //counts indexed by [input value, outcome value]
	//the prediction score (S score from the paper)
	double prediction_score = 0;
	//prediction success confidence interval (estimate of how often it predicts the outcome correctly)
	ConfidenceInterval success_interval;
public:
	FrequencyTable(std::size_t m); //m = number of inputs
	//
	void reset();
	//
	size_t getInputStates() const;
	std::set<std::size_t> getObservedInputStates() const; //keys of count_m
	size_t getOutputStates() const;
	size_t getCountTotal() const;
	size_t getCountInput(std::size_t state_in) const;
	size_t getCountOutput(size_t state_out) const;
	size_t getCount(std::size_t state_in, size_t state_out) const;
	//
	double getFrequencyInput(std::size_t state_in) const;
	double getFrequencyOutput(size_t state_out) const;
	double getFrequency(std::size_t state_in, size_t state_out) const;
	double getFrequencyConditional(std::size_t state_in, size_t state_out) const; //P(y | x) = P(x, y) / P(x)
	ProbabilityDistribution<size_t> getConditionalDistribution(std::size_t state_in) const; //P(y|x) for all y
	FrequencyTable slice(std::size_t in) const;
	//
	//size_t getPredictionCount() const;
	double getPredictionScore() const;
	ConfidenceInterval getSuccessInterval() const;
	//make an observation
	void observe(std::size_t state_in, size_t state_out); //may need to auto-adjust the k parameter if a new outcome is suddenly observed
	//recalculate all of the frequencies and confidence interval
	void recalculate(double alpha); //success confidence interval alpha value (e.g. 0.01 for a 99% confidence interval)
	//
	size_t predict(std::size_t state_in) const; //return the max-likelihood outcome for the given input state
	double confidence(std::size_t state_in) const; //return the probability of the max-likelihood outcome for the given input state (1 = it is the only observed outcome, 0 = never observed, 0<x<1 -> multiple outcomes observed)
	//
	void print(FILE* f) const;
	//
	bool operator>(const FrequencyTable& b) const; //to keep them in sorted order, based on success interval
	bool operator<(const FrequencyTable& b) const; //to keep them in sorted order, based on success interval
	//
	void to_json(json& j) const;
	void from_json(const json& j);
};