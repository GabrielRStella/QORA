#pragma once

struct ConfidenceInterval {
	double lower = 0;
	double upper = 1;
	//
	void print(FILE* f) const;
	//
	bool operator>(double d) const; //is lower > d?
	bool operator<(double d) const; //is upper < d?
	//
	bool operator>(const ConfidenceInterval& b) const; //is a.lower > b.lower?
	bool operator<(const ConfidenceInterval& b) const; //is a.upper < b.upper?
	//bool operator==(const ConfidenceInterval& b) const;
	double operator-(const ConfidenceInterval& b) const; //max (a.upper - b.upper, a.lower - b.lower)
};

class Statistics
{
public:
	static ConfidenceInterval estimate_binomial_interval(std::size_t n, std::size_t ns, double alpha); //n = total observations, ns = successes, alpha = confidence level (a=0.05 -> 95% confidence interval)
};


