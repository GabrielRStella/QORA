#pragma once

#include "Environment.h"
#include "ProbabilityDistribution.h"


//superclass for learning algorithms
class Learner
{
	//the name of this learning alg, for human readability (and comparison of algs)
	std::string name;

protected:
	//the types in the environment this learner is working in
	Types types;

public:

	Learner(const std::string& name, const Types& types);

	const std::string& getName() const;

	//learner-specific functions

	//just in case I need this
	//clear all learned parameters
	virtual void reset() = 0;

	virtual void restart() = 0; //erase any stored history, if necessary; this is called before a new episode is started

	//return a probability distribution over future states P(s, a, s')
	virtual StateDistribution predictTransition(const State& state, ActionId action, Random& random) const = 0;

	//observe (s, a, s')
	virtual void observeTransition(const State& prevState, ActionId action, const State& nextState) = 0;

	//print some info about current observations and rules
	virtual void print(FILE* f) const = 0;

	//save/load model (json)
	virtual json to_json() const = 0;
	virtual void from_json(const json& j) = 0;
};

//uses the environment to produce ground-truth predictions
class Oracle : public Learner
{
	const Environment* env;
public:

	Oracle(const Environment* env);

	//learner-specific functions

	//just in case I need this
	//clear all learned parameters
	virtual void reset();

	virtual void restart(); //erase any stored history, if necessary; this is called before a new episode is started

	//return a probability distribution over future states P(s, a, s')
	virtual StateDistribution predictTransition(const State& state, ActionId action, Random& random) const;

	//observe (s, a, s')
	virtual void observeTransition(const State& prevState, ActionId action, const State& nextState);

	//print some info about current observations and rules
	virtual void print(FILE* f) const;

	//save/load model (json)
	virtual json to_json() const;
	virtual void from_json(const json& j);
};

