#pragma once

#include "Learner.h"
#include "FrequencyTable.h"

namespace l_qora {

	struct EffectType {
		int object_type = -1; //the class that this effect applies to
		int attribute_type = -1; //the attribute that will be modified

		//so they can be used as set/map keys/values
		bool operator<(const EffectType& e) const;
		bool operator==(const EffectType& e) const;
	};

	void to_json(json& j, const EffectType& p, const Types& types);
	void from_json(const json& j, EffectType& p, const Types& types);

	typedef AttributeValue Effect; //represents a delta

	struct Predicate {
		int attribute_type = -1;
		bool is_relative = false;
		bool is_target = true; //if not relative, then it's either a value of the target or of the "other"
		AttributeValue value;
		//
		bool evaluate(const Object& target) const; //will return false if it needs an "other" object
		bool evaluate(const Object& target, const Object& other) const;
		//
		void print(FILE* f, const Types& types) const;
		//
		bool operator<(const Predicate& b) const; //for usage in map
		bool operator==(const Predicate& b) const; //for usage in map
	};

	void to_json(json& j, const Predicate& p, const Types& types);
	void from_json(const json& j, Predicate& p, const Types& types);

	//a collection of PredicateBases, evaluated over a single target-other pair
	struct RelationGroup {
		int other_object_type = -1;
		std::set<Predicate> predicates; //set so they're always in some consistent (sorted) order; number of predicates in this set is 'm'
		//single-pair state size
		std::size_t size() const; //m
		std::size_t stateSize() const; //2^m
		//all-pairs state size = all possible set combinations of single-pair state size
		std::size_t completeStateSize() const; //2^(2^m)
		//return a state from 0 to (2^m)-1, where m is the number of predicates
		std::size_t evaluate_single(const Object& target) const; //if other_object_type = -1
		std::size_t evaluate_single(const Object& target, const Object& other) const;
		//calculate all possible evaluations over all possible target-other assignments
		//returns a state combo in 0..(2^(2^m))-1
		std::size_t evaluate_all(const Object& target, const std::map<int, std::set<const Object*>>& objects_by_type) const;
		//
		void print(FILE* f, const Types& types) const;
		void printCaseInfo(FILE* f, const Types& types, std::size_t value) const;
		//
		bool operator<(const RelationGroup& b) const; //for usage in map
		bool operator==(const RelationGroup& b) const; //for usage in map
	};

	void to_json(json& j, const RelationGroup& p, const Types& types);
	void from_json(const json& j, RelationGroup& p, const Types& types);

	//a set of predicates, without information about how they are used
	struct Condition {
		std::set<RelationGroup> groups; //only 1 may have -1 as "other type", for consistency
		//
		//std::size_t size() const; //# groups
		std::size_t stateSize() const; //prod(group sizes)
		//return a state from 0 to stateSize-1, representing some existential/universal predicate group stufff
		std::size_t evaluate(const Object& target, const std::map<int, std::set<const Object*>>& objects_by_type) const;
		//
		void print(FILE* f, const Types& types, int target_object_type) const;
		void printCaseInfo(FILE* f, const Types& types, int target_object_type, std::size_t input_case) const;
		//union two CompoundPredicates
		Condition operator+(const Condition& other) const;
		//
		bool operator<(const Condition& b) const; //for usage in map
		bool operator==(const Condition& b) const; //for usage in map
	};

	void to_json(json& j, const Condition& p, const Types& types);
	void from_json(const json& j, Condition& p, const Types& types);

	//helper struct to deal with going from Predicates and Effects to (int input, int outcome) for the FrequencyCounter
	struct Candidate {
		Condition condition;
		FrequencyTable table;
		//
		void observe(const Object& target, const std::map<int, std::set<const Object*>>& objects_by_type, int effect);
		//
		void print(FILE* f, const Types& types, EffectType type, const std::vector<Effect>& effects) const;
		//
		bool operator>(const Candidate& b) const; //to keep them in sorted order, based on success interval
		bool operator<(const Candidate& b) const; //to keep them in sorted order, based on success interval
	};

	void to_json(json& j, const Candidate& p, const Types& types);
	void from_json(const json& j, Candidate& p, const Types& types);

	//
	class StochasticEffectPredictor {
		//
		double alpha; //confidence level for counters
		//
		std::set<Condition> observed; //all CompoundPredicates ever used, so they don't get repeated
		std::list<Candidate> working; //current list of predicates that are being tested (some will eventually go to hypotheses)
		std::vector<Candidate> hypotheses; //list of predicates that given more information than baseline/random guess, kept in approximately sorted order (s.t. the first element is always the "best")
		FrequencyTable baseline; //a counter that keeps track of baseline performance, and is used as the predictor until a PredicateCounter gets into the hypotheses set
		//
		int effect_count = 0; //number of observed effects, each of which is given a unique index starting at 0
		std::map<Effect, size_t> effect_indices; //Effect -> int mapping
		std::vector<Effect> effects; //int -> Effect mapping
		//
		void test_add_pairs(const Types& types, int target_object_type, const Condition& a, const Condition& b);
		void test_add(const Types& types, int target_object_type, const Condition& cp); //if not in observed, add to observed + current
		//
	public:
		StochasticEffectPredictor(double alpha = 0.01); //default = 99% confidence interval
		//
		void observe(const Types& types, const Object& target, const std::map<int, std::set<const Object*>>& objects_by_type, const Effect& effect);
		ProbabilityDistribution<Effect> predict(const Object& target, const std::map<int, std::set<const Object*>>& objects_by_type) const;
		//
		const std::set<Condition>& getPredicatesObserved() const;
		size_t getCountPredicatesObserved() const;
		size_t getCountPredicatesTracked() const;
		size_t getCountHypothesesTracked() const;
		//
		void print(FILE* f, const Types& types, EffectType type) const;
		//
		json to_json(const Types& types) const;
		void from_json(const Types& types, const json& j);
	};

	class LearnerQORA : public Learner
	{
		double alpha; //confidence level

		std::map<std::pair<EffectType, ActionId>, std::set<Effect>> effects_observed; //the possible effects observed for each <object type, action> pair; once the set increases to >1 element, it gets an entry in the below map
		std::map<std::pair<EffectType, ActionId>, StochasticEffectPredictor> predictors;

		std::size_t last_predicates_observed = 0;

	public:

		LearnerQORA(const Types& types, double alpha);

		//learner-specific functions
		double getAlpha() const;
		std::size_t countTotalPredicatesObserved() const; //sum the number of predicates observed by each predictor (this will double-count many of them)
		std::size_t countUniquePredicatesObserved() const; //sum the number of predicates observed by each predictor (will not double-count)
		std::size_t countLastPredicatesObserved() const; //sum the number of predicates observed by each predictor, only if it was used in the last call to observe()

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


}
