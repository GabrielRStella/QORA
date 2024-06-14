#pragma once

#include "Environment.h"

//helper to iterate over concise or verbose pre-generated files
//produces (s, a, s') triplets
class ObservationIterator {
	//
	std::istream& input;
	const Environment* env;
	const Types& types;
	//
	bool is_verbose; //false = concise, true = verbose
	Random random; //seeded based on the input file
	int n; //concise: # levels; verbose: # observations
	int m; //concise: # actions per level; verbose: 0
	//
	int i; //number of observations returned so far
	//concise: current level+actions; verbose: next observation
	json current_object;
	//
	State s;
	ActionName a;
	StateDistribution s_primes;
	State s_prime;
public:
	ObservationIterator(std::istream& input, const Environment* env, const json& file_info); //uses file_info to check if concise/verbose and extract m,n
	//global info
	const Environment* getEnvironment() const;
	//
	bool next(); //return true if there is another observation
	int index(); //current observation index (starts at 0 after calling next)
	int count(); //total number of observations
	//
	const State& getStartState() const;
	const ActionName& getAction() const;
	const StateDistribution& getNextStates() const;
	const State& getNextState() const;
};