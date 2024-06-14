#include "pch.h"
#include "Learner.h"

Learner::Learner(const std::string& name, const Types& types) : name(name), types(types)
{
}

const std::string& Learner::getName() const
{
	return name;
}

Oracle::Oracle(const Environment* env) : Learner("oracle", env->getTypes()), env(env)
{
}

void Oracle::reset()
{
	//noop
}

void Oracle::restart()
{
	//noop
}

StateDistribution Oracle::predictTransition(const State& state, ActionId action, Random& random) const
{
	return env->act(state, action, random);
}

void Oracle::observeTransition(const State& prevState, ActionId action, const State& nextState)
{
	//noop
}

void Oracle::print(FILE* f) const
{
	//noop
}

json Oracle::to_json() const
{
	//noop
	return json();
}

void Oracle::from_json(const json& j)
{
	//noop
}
