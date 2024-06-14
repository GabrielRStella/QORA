#include "pch.h"
#include "Serialization.h"

ObservationIterator::ObservationIterator(std::istream& input, const Environment* env, const json& file_info) :
	input(input), env(env), types(env->getTypes()),
	is_verbose(file_info["identifier"] == "states_verbose"),
	n(file_info["n"]),
	m(is_verbose ? 0 : int(file_info["m"])),
	i(-1),
	current_object(json{})
{
	assert(file_info["identifier"] == "states_concise" || file_info["identifier"] == "states_verbose");
	random.seed(file_info["random_seed"].get<unsigned int>());
}

const Environment* ObservationIterator::getEnvironment() const
{
	return env;
}

bool ObservationIterator::next()
{
	//check
	if (is_verbose) {
		if (i + 1 >= n) {
			current_object = json{};
			return false;
		}
	}
	else {
		if (i + 1 >= n * m) {
			current_object = json{};
			return false;
		}
	}
	//
	i++;
	//move
	if (is_verbose) {
		//read next
		input >> current_object;
		//
		types.from_json(s, current_object["start"]);
		a = current_object["action"].get<std::string>();
		types.from_json(s_primes, current_object["nexts"]);
		types.from_json(s_prime, current_object["next"]);
	}
	else {
		//move to next?
		if (i % m == 0) {
			input >> current_object;
			types.from_json(s, current_object["level"]);
		}
		else {
			s = s_prime;
		}
		a = current_object["actions"][i % m].get<std::string>();
		int a_ = types.getActionByName(a);
		s_primes = env->act(s, a_, random);
		s_prime = s_primes.sample(random);
	}
	//done
	return true;
}

int ObservationIterator::index()
{
	return i;
}

int ObservationIterator::count()
{
	if (is_verbose) {
		return n;
	}
	else {
		return n * m;
	}
}

const State& ObservationIterator::getStartState() const
{
	return s;
}

const ActionName& ObservationIterator::getAction() const
{
	return a;
}

const StateDistribution& ObservationIterator::getNextStates() const
{
	return s_primes;
}

const State& ObservationIterator::getNextState() const
{
	return s_prime;
}
