#include "pch.h"
#include "LearnerQORA.h"

namespace l_qora {

	bool EffectType::operator<(const EffectType& e) const
	{
		if (object_type < e.object_type) return true;
		if (object_type > e.object_type) return false;
		return attribute_type < e.attribute_type;
	}

	bool EffectType::operator==(const EffectType& e) const
	{
		return object_type == e.object_type && attribute_type == e.attribute_type;
	}

	void to_json(json& j, const EffectType& p, const Types& types)
	{
		j = json{
			{"object_type", types.getObjectTypes().at(p.object_type).name},
			{"attribute_type", types.getAttributeTypes().at(p.attribute_type).name}
		};
	}

	void from_json(const json& j, EffectType& p, const Types& types)
	{
		p.object_type = types.getObjectType(j.at("object_type").get<std::string>()).id;
		p.attribute_type = types.getAttributeType(j.at("attribute_type").get<std::string>()).id;
	}

	bool Predicate::evaluate(const Object& target) const
	{
		return !is_relative && is_target && target.getAttribute(attribute_type) == value;
	}

	bool Predicate::evaluate(const Object& target, const Object& other) const
	{
		if (is_relative) {
			return (other.getAttribute(attribute_type) - target.getAttribute(attribute_type)) == value;
		}
		else if (is_target) {
			return target.getAttribute(attribute_type) == value;
		}
		else {
			return other.getAttribute(attribute_type) == value;
		}
		return false; //??
	}

	void Predicate::print(FILE* f, const Types& types) const
	{
		std::string attrib_name = types.getAttributeType(attribute_type).name;
		std::string value_str = value.to_string();
		char target_name = 'x';
		char other_name = 'y';// +other_object_entry;
		if (is_relative) {
			fprintf(f, "%c.%s - %c.%s = %s", other_name, attrib_name.c_str(), target_name, attrib_name.c_str(), value_str.c_str());
		}
		else {
			if (is_target) {
				fprintf(f, "%c.%s = %s", target_name, attrib_name.c_str(), value_str.c_str());
			}
			else {
				fprintf(f, "%c.%s = %s", other_name, attrib_name.c_str(), value_str.c_str());
			}
		}
	}

	bool Predicate::operator<(const Predicate& b) const
	{
		if (attribute_type < b.attribute_type) return true;
		if (attribute_type > b.attribute_type) return false;
		//
		if (is_relative != b.is_relative) return b.is_relative;
		if (is_target != b.is_target) return is_target;
		//
		return value < b.value;
	}

	bool Predicate::operator==(const Predicate& b) const
	{
		return attribute_type == b.attribute_type && is_relative == b.is_relative && is_target == b.is_target && value == b.value;
	}

	void to_json(json& j, const Predicate& p, const Types& types)
	{
		j = json{
			{"attribute_type", types.getAttributeTypes().at(p.attribute_type).name},
			{"is_relative", p.is_relative},
			{"is_target", p.is_target},
			{"value", p.value}
		};
	}

	void from_json(const json& j, Predicate& p, const Types& types)
	{
		p.attribute_type = types.getAttributeType(j.at("attribute_type").get<std::string>()).id;
		j.at("is_relative").get_to(p.is_relative);
		j.at("is_target").get_to(p.is_target);
		j.at("value").get_to(p.value);
	}

	std::size_t RelationGroup::size() const
	{
		return predicates.size();
	}

	std::size_t RelationGroup::stateSize() const
	{
		return std::size_t(1) << predicates.size();
	}

	std::size_t RelationGroup::completeStateSize() const
	{
		return std::size_t(1) << (std::size_t(1) << predicates.size());
	}

	std::size_t RelationGroup::evaluate_single(const Object& target) const
	{
		std::size_t value = 0;
		int place = 0;
		for (const Predicate& p : predicates) {
			value += (std::size_t(p.evaluate(target)) << place);
			place++;
		}
		return value;
	}

	std::size_t RelationGroup::evaluate_single(const Object& target, const Object& other) const
	{
		std::size_t value = 0;
		int place = 0;
		for (const Predicate& p : predicates) {
			value += (std::size_t(p.evaluate(target, other)) << place);
			place++;
		}
		return value;
	}

	std::size_t RelationGroup::evaluate_all(const Object& target, const std::map<int, std::set<const Object*>>& objects_by_type) const
	{
		std::size_t result = 0;
		//go over all possible pairs, evaluate, and union into result
		if (other_object_type == -1) {
			//actually syke, no pairs lol
			result = (std::size_t(1) << evaluate_single(target));
		}
		else {
			//ok, pairs
			for (const Object* other : objects_by_type.at(other_object_type)) {
				result |= (std::size_t(1) << evaluate_single(target, *other));
			}
		}
		//
		return result;
	}

	void RelationGroup::print(FILE* f, const Types& types) const
	{
		if (other_object_type >= 0) {
			fprintf(f, "[%s y: ", types.getObjectType(other_object_type).name.c_str());
		}
		else {
			fprintf(f, "[");
		}
		auto begin = predicates.begin();
		auto end = predicates.end();
		auto it = begin;
		while (it != end) {
			if (it != begin) fprintf(f, "; ");
			//
			it->print(f, types);
			//
			++it;
		}
		fprintf(f, "]");
	}

	void RelationGroup::printCaseInfo(FILE* f, const Types& types, std::size_t value) const
	{
		//the number of bits in a single evaluation = # of predicates = n, max val = (2^n)-1
		//so number of bits in value to check = (2^n)-1 bc each evaluation possibility becomes a single bit
		std::size_t n = predicates.size();
		std::size_t m = (1LLU << n);
		fprintf(f, "%{");
		bool needs_comma = false;
		for (std::size_t i = 0; i < m; i++) {
			if ((value & (1LLU << i)) != 0) {
				//this bit is active, meaning this possibility occurred and needs to be printed
				if (needs_comma) fprintf(f, ", ");
				//predicates are put into i in forwards order, i.e., the highest-value bit corresponds to the last predicate in the list
				for (std::size_t j = 0; j < n; j++) {
					bool b = (i & (1LLU << j)) != 0;
					fprintf(f, "%c", b ? 'T' : 'F');
				}
				//fprintf(f, "%zu", i);
				//
				needs_comma = true;
			}
		}
		fprintf(f, "}");
	}

	bool RelationGroup::operator<(const RelationGroup& b) const
	{
		if (other_object_type < b.other_object_type) return true;
		if (other_object_type > b.other_object_type) return false;
		return predicates < b.predicates;
	}

	bool RelationGroup::operator==(const RelationGroup& b) const
	{
		return other_object_type == b.other_object_type && predicates == b.predicates;
	}

	void to_json(json& j, const RelationGroup& p, const Types& types)
	{
		j = json{
			{"other_object_type", p.other_object_type == -1 ? json{} : types.getObjectTypes().at(p.other_object_type).name}
		};
		//
		json& j_predicates = j["predicates"];
		for (const Predicate& pred : p.predicates) {
			json j_pred;
			to_json(j_pred, pred, types);
			j_predicates.push_back(j_pred);
		}
	}

	void from_json(const json& j, RelationGroup& p, const Types& types)
	{
		const json& jj = j.at("other_object_type");
		p.other_object_type = jj.is_null() ? -1 : types.getObjectType(jj.get<std::string>()).id;
		//
		p.predicates.clear();
		for (const json& j_pred : j.at("predicates")) {
			Predicate pred;
			from_json(j_pred, pred, types);
			p.predicates.insert(pred);
		}
	}

	std::size_t Condition::stateSize() const
	{
		std::size_t sz = 1;
		for (const RelationGroup& group : groups) sz *= group.completeStateSize();
		return sz;
	}

	std::size_t Condition::evaluate(const Object& target, const std::map<int, std::set<const Object*>>& objects_by_type) const
	{
		std::size_t value = 0;
		std::size_t multiplier = 1;
		for (const RelationGroup& group : groups) {
			value += group.evaluate_all(target, objects_by_type) * multiplier;
			multiplier *= group.completeStateSize();
		}
		return value;
	}

	void Condition::print(FILE* f, const Types& types, int target_object_type) const
	{
		fprintf(f, "%s x: ", types.getObjectType(target_object_type).name.c_str());
		auto begin = groups.begin();
		auto end = groups.end();
		auto it = begin;
		while (it != end) {
			if (it != begin) fprintf(f, ", ");
			//
			it->print(f, types);
			//
			++it;
		}
	}

	void Condition::printCaseInfo(FILE* f, const Types& types, int target_object_type, std::size_t input_case) const
	{
		//figure out value for each relation group
		std::vector<std::size_t> group_values;
		{
			auto it = groups.begin();
			auto end = groups.end();
			while (it != end) {
				const RelationGroup& group = *it;
				//
				std::size_t multiplier = group.completeStateSize();
				//group_values.insert(group_values.begin(), input_case % multiplier);
				group_values.push_back(input_case % multiplier);
				input_case = input_case / multiplier;
				//
				++it;
			}
		}
		//print each relation group's thing
		{
			auto begin = groups.begin();
			auto end = groups.end();
			auto it = begin;
			std::size_t index = 0;
			while (it != end) {
				if (it != begin) fprintf(f, ", ");
				//
				it->printCaseInfo(f, types, group_values[index++]);
				//
				++it;
			}
		}
	}

	Condition Condition::operator+(const Condition& other) const
	{

		std::map<int, RelationGroup> groups;
		for (const RelationGroup& group : this->groups) {
			groups[group.other_object_type] = group;
		}
		for (const RelationGroup& group : other.groups) {
			RelationGroup& g = groups[group.other_object_type];
			g.other_object_type = group.other_object_type;
			g.predicates.insert(group.predicates.begin(), group.predicates.end());
		}
		Condition cp;
		for (auto& pair : groups) {
			cp.groups.insert(pair.second);
		}
		return cp;
	}

	bool Condition::operator<(const Condition& b) const
	{
		return groups < b.groups;
	}

	bool Condition::operator==(const Condition& b) const
	{
		return groups == b.groups;
	}

	void to_json(json& j, const Condition& p, const Types& types)
	{
		j = json{};
		for (const RelationGroup& pred : p.groups) {
			json j_pred;
			to_json(j_pred, pred, types);
			j.push_back(j_pred);
		}
	}

	void from_json(const json& j, Condition& p, const Types& types)
	{
		p.groups.clear();
		for (const json& j_pred : j) {
			RelationGroup pred;
			from_json(j_pred, pred, types);
			p.groups.insert(pred);
		}
	}

	void Candidate::observe(const Object& target, const std::map<int, std::set<const Object*>>& objects_by_type, int effect)
	{
		std::size_t state_in = condition.evaluate(target, objects_by_type);
		table.observe(state_in, effect);
	}

	void Candidate::print(FILE* f, const Types& types, EffectType type, const std::vector<Effect>& effects) const
	{
		fprintf(f, "     ");
		condition.print(f, types, type.object_type);
		fprintf(f, "\n     ");
		table.print(f);
		fprintf(f, "\n");
		//pretty-print observed cases
		for (int input : table.getObservedInputStates()) {
			fprintf(f, "      ");
			condition.printCaseInfo(f, types, type.object_type, input);
			fprintf(f, "\n       ");
			//print distribution from counter, using actual effect values
			std::size_t n_effects = effects.size();
			bool needs_comma = false;
			for (std::size_t i = 0; i < n_effects; i++) {
				double p = table.getFrequencyConditional(input, i);
				if (p > 0) {
					if (needs_comma) fprintf(f, "; ");
					fprintf(f, "%s %.2f%%", effects.at(i).to_string().c_str(), p * 100);
					needs_comma = true;
				}
			}
			fprintf(f, "\n");
		}
	}

	bool Candidate::operator>(const Candidate& b) const
	{
		return table > b.table;
	}

	bool Candidate::operator<(const Candidate& b) const
	{
		return table < b.table;
	}

	void to_json(json& j, const Candidate& p, const Types& types)
	{
		j = json{};
		to_json(j["predicates"], p.condition, types);
		p.table.to_json(j["counter"]);
	}

	void from_json(const json& j, Candidate& p, const Types& types)
	{
		from_json(j.at("predicates"), p.condition, types);
		p.table.from_json(j.at("counter"));
	}

	void StochasticEffectPredictor::test_add_pairs(const Types& types, int target_object_type, const Condition& a, const Condition& b)
	{
		test_add(types, target_object_type, a + b);
	}

	void StochasticEffectPredictor::test_add(const Types& types, int target_object_type, const Condition& cp)
	{
		if (observed.find(cp) == observed.end()) {
			observed.insert(cp);
			working.push_back(Candidate{ cp, FrequencyTable(cp.stateSize()) });
		}
	}

	StochasticEffectPredictor::StochasticEffectPredictor(double alpha) : alpha(alpha), baseline(1)
	{
	}

	void StochasticEffectPredictor::observe(const Types& types, const Object& target, const std::map<int, std::set<const Object*>>& objects_by_type, const Effect& effect)
	{
		int target_object_type = target.getTypeId();
		auto target_type_obj = types.getObjectType(target_object_type);

		//convert Effect to int
		size_t effect_index = -1;
		{
			auto it = effect_indices.find(effect);
			if (it != effect_indices.end()) {
				effect_index = it->second;
			}
			else {
				effect_index = effect_count++;
				effect_indices[effect] = effect_index;
				effects.push_back(effect);
			}
		}

		//call observe() on baseline
		baseline.observe(0, effect_index);
		baseline.recalculate(alpha);

		//update all hypotheses
		for (Candidate& pc : hypotheses) {
			Condition& predicates = pc.condition;
			FrequencyTable& counter = pc.table;
			//
			std::size_t state_in = predicates.evaluate(target, objects_by_type);
			counter.observe(state_in, effect_index);
			counter.recalculate(alpha);
		}
		//'bubble up' the best counters in 'hypotheses'
		for (int i = hypotheses.size() - 2; i > 0; i--) {
			Candidate& a = hypotheses[i];
			Candidate& b = hypotheses[i + 1];
			if (b > a) std::swap(a, b); //swap the larger one up the list (larger = higher score confidence interval estimate)
		}
		//swap the last (first) pair
		//if the top hypothesis is new, add pairs with it + every other hypothesis to 'current' (if they are not in 'observed')
		if (hypotheses.size() > 1) {
			Candidate& a = hypotheses[0];
			Candidate& b = hypotheses[1];
			if (b > a) {
				//swap
				std::swap(a, b);
				//reset all non-hypothesis counters, including baseline, so they will be conditional on the new best
				baseline.reset();
				for (Candidate& pc : working) {
					pc.table.reset();
				}
				//clear all current tracked
				//create all pairs with other hypotheses
				//we can skip the first, ofc
				for (int i = 2; i < hypotheses.size(); i++) {
					test_add_pairs(types, target_object_type, a.condition, hypotheses.at(i).condition);
				}
			}
		}

		//discard observations that are explained by the current best hypothesis to magnify the effect of secondary rules
		if (hypotheses.size() > 0) {
			Candidate& pc = hypotheses.at(0);
			Condition& predicates = pc.condition;
			FrequencyTable& counter = pc.table;
			//
			std::size_t state_in = predicates.evaluate(target, objects_by_type);
			if (counter.confidence(state_in) == 1 && counter.predict(state_in) == effect_index) {
				return;
			}
		}

		//create a list of singleton PredicateTerms in this state; if they are not in 'observed', add them to 'observed' and create a new corresponding Candidate in 'current'
		//add the non-paired (target-only) predicates
		for (int attribute : target_type_obj.attribute_types) {
			test_add(types, target_object_type, Condition{ {RelationGroup{-1, {Predicate{attribute, false, true, target.getAttribute(attribute)}} }} });
		}
		//for each valid pair of (target type, any other type), construct pairs
		for (const auto& pair : objects_by_type) {
			int other_object_type = pair.first;
			auto& other_type_obj = types.getObjectType(other_object_type);
			for (const Object* other : pair.second) {
				if (other->getObjectId() == target.getObjectId()) continue; //none of that!!
				//the pair is now (target, other)
				for (int attribute : target_type_obj.attribute_types) {
					if (other_type_obj.attribute_types.find(attribute) != other_type_obj.attribute_types.end()) {
						test_add(types, target_object_type, Condition{ {RelationGroup{other_object_type, {Predicate{attribute, true, false, other->getAttribute(attribute) - target.getAttribute(attribute)}} }} });
					}
				}
				for (int attribute : other_type_obj.attribute_types) {
					test_add(types, target_object_type, Condition{ {RelationGroup{other_object_type, {Predicate{attribute, false, false, other->getAttribute(attribute)}} }} });
				}
			}
		}

		//go through each Candidate in 'current' and 'hypotheses' and call observe() and recalculate()
		//if any counter in 'current' is now > baseline, move it to 'hypotheses'
		//and generate a new compound predicate based on it + the current hypothesis, if there is one (and it isn't already in 'observed')
		{
			ConfidenceInterval baseline_score = baseline.getSuccessInterval();
			auto it = working.begin();
			auto end = working.end();
			while (it != end) {
				Condition& predicates = it->condition;
				FrequencyTable& counter = it->table;
				//
				std::size_t state_in = predicates.evaluate(target, objects_by_type);
				counter.observe(state_in, effect_index);
				counter.recalculate(alpha);
				ConfidenceInterval interval = counter.getSuccessInterval();
				//
				if (interval > baseline_score) {
					//move to hypotheses
					Candidate pc = *it;

					pc.table.reset(); //since it'll now be getting more data
					hypotheses.push_back(pc);
					it = working.erase(it);
					//create pair of this + best hypothesis
					if (hypotheses.size() > 1) {
						test_add_pairs(types, target_object_type, hypotheses.at(0).condition, pc.condition);
					}
					else {
						//first hypothesis
						//reset all non-hypothesis counters, including baseline
						baseline.reset();
						baseline_score = baseline.getSuccessInterval();
						for (Candidate& pc : working) {
							pc.table.reset();
						}
					}
				}
				else {
					//no change, keep observing
					++it;
				}
			}
		}
	}

	ProbabilityDistribution<Effect> StochasticEffectPredictor::predict(const Object& target, const std::map<int, std::set<const Object*>>& objects_by_type) const
	{
		ProbabilityDistribution<size_t> prediction;
		//if there is no good hypothesis, use the baseline:
		if (hypotheses.empty()) {
			prediction = baseline.getConditionalDistribution(0);
		}
		//if there is a hypothesis, evaluate its predicates and use the counter to predict the outcome
		else {
			const Candidate& hypothesis = hypotheses[0];
			prediction = hypothesis.table.getConditionalDistribution(hypothesis.condition.evaluate(target, objects_by_type));
		}
		//if the hypotheses were just reset, they'll all be empty, so we need to guess something
		if (prediction.size() == 0) {
			prediction.addProbability(0, 1.0);
		}
		//
		ProbabilityDistribution<Effect> predicted_effects;
		for (const auto& pair : prediction.getProbabilities()) {
			predicted_effects.setProbability(effects[pair.first], pair.second);
		}
		return predicted_effects;
	}

	const std::set<Condition>& StochasticEffectPredictor::getPredicatesObserved() const
	{
		return observed;
	}

	size_t StochasticEffectPredictor::getCountPredicatesObserved() const
	{
		return observed.size();
	}

	size_t StochasticEffectPredictor::getCountPredicatesTracked() const
	{
		return working.size();
	}

	size_t StochasticEffectPredictor::getCountHypothesesTracked() const
	{
		return hypotheses.size();
	}

	void StochasticEffectPredictor::print(FILE* f, const Types& types, EffectType type) const
	{

		//print out the effect list
		fprintf(f, "   Effects:\n");
		for (int i = 0; i < effects.size(); i++) {
			fprintf(f, "    [%d] %s\n", i, effects.at(i).to_string().c_str());
		}

		//print out the hypotheses' predicate sets
		if (hypotheses.size() > 0) {
			fprintf(f, "   Hypotheses: %zu\n", hypotheses.size());
			for (int i = 0; i < std::min(hypotheses.size(), 3LLU); i++) {
				fprintf(f, "    [%d]\n", i);
				hypotheses.at(i).print(f, types, type, effects);
			}
		}
		else {
			fprintf(f, "   Hypotheses: none\n");
		}

		fprintf(f, "   Observed: %zu\n", observed.size());
		fprintf(f, "   Working set: %zu\n", working.size());

		//print out the baseline
		fprintf(f, "   Baseline:\n");
		fprintf(f, "    ");
		baseline.print(f);
		fprintf(f, "\n     ");
		//print distribution from counter, using actual effect values
		{
			std::size_t n_effects = effects.size();
			bool needs_comma = false;
			for (std::size_t i = 0; i < n_effects; i++) {
				double p = baseline.getFrequencyConditional(0, i);
				//if (p > 0) {
					if (needs_comma) fprintf(f, "; ");
					fprintf(f, "%s %.2f%%", effects.at(i).to_string().c_str(), p * 100);
					needs_comma = true;
				//}
			}
		}
		fprintf(f, "\n");
	}

	json StochasticEffectPredictor::to_json(const Types& types) const
	{
		json j;
		//std::set<CompoundPredicate> observed; //all CompoundPredicates ever used, so they don't get repeated
		json& j_observed = j["observed"];
		for (const Condition& cp : observed) {
			j_observed.push_back(json{});
			l_qora::to_json(j_observed.back(), cp, types);
		}
		//std::list<PredicateCounter> current; //current list of predicates that are being tested (some will eventually go to hypotheses)
		json& j_current = j["current"];
		for (const Candidate& cp : working) {
			j_current.push_back(json{});
			l_qora::to_json(j_current.back(), cp, types);
		}
		//std::vector<PredicateCounter> hypotheses; //list of predicates that given more information than baseline/random guess, kept in approximately sorted order (s.t. the first element is always the "best")
		json& j_hypotheses = j["hypotheses"];
		for (const Candidate& cp : hypotheses) {
			j_hypotheses.push_back(json{});
			l_qora::to_json(j_hypotheses.back(), cp, types);
		}
		//FrequencyCounter baseline; //a counter that keeps track of baseline performance, and is used as the predictor until a PredicateCounter gets into the hypotheses set
		baseline.to_json(j["baseline"]);
		//int effect_count = 0; //number of observed effects, each of which is given a unique index starting at 0
		//std::map<Effect, int> effect_indices; //Effect -> int mapping
		//std::vector<Effect> effects; //int -> Effect mapping
		j["effects"] = effects;
		//
		return j;
	}

	void StochasticEffectPredictor::from_json(const Types& types, const json& j)
	{
		//std::set<CompoundPredicate> observed; //all CompoundPredicates ever used, so they don't get repeated
		observed.clear();
		for (const json& j_ : j.at("observed")) {
			Condition cp;
			l_qora::from_json(j_, cp, types);
			observed.insert(cp);
		}
		//std::list<PredicateCounter> current; //current list of predicates that are being tested (some will eventually go to hypotheses)
		working.clear();
		for (const json& j_ : j.at("current")) {
			Candidate cp{Condition(), FrequencyTable(1)};
			l_qora::from_json(j_, cp, types);
			cp.table.recalculate(alpha);
			working.push_back(cp);
		}
		//std::vector<PredicateCounter> hypotheses; //list of predicates that given more information than baseline/random guess, kept in approximately sorted order (s.t. the first element is always the "best")
		hypotheses.clear();
		for (const json& j_ : j.at("hypotheses")) {
			Candidate cp{ Condition(), FrequencyTable(1) };
			l_qora::from_json(j_, cp, types);
			cp.table.recalculate(alpha);
			hypotheses.push_back(cp);
		}
		//FrequencyCounter baseline; //a counter that keeps track of baseline performance, and is used as the predictor until a PredicateCounter gets into the hypotheses set
		baseline.from_json(j.at("baseline"));
		baseline.recalculate(alpha);
		//int effect_count = 0; //number of observed effects, each of which is given a unique index starting at 0
		//std::map<Effect, int> effect_indices; //Effect -> int mapping
		//std::vector<Effect> effects; //int -> Effect mapping
		effects = j.at("effects").get<std::vector<Effect>>();
		effect_count = effects.size();
		for (int i = 0; i < effect_count; i++) {
			effect_indices[effects[i]] = i;
		}
	}

	LearnerQORA::LearnerQORA(const Types& types, double alpha) :
		Learner("qora", types), alpha(alpha)
	{
	}

	double LearnerQORA::getAlpha() const
	{
		return alpha;
	}

	size_t LearnerQORA::countTotalPredicatesObserved() const
	{
		size_t total_predicates = 0;
		for (auto& it : predictors) {
			total_predicates += it.second.getCountPredicatesObserved();
		}
		return total_predicates;
	}

	std::size_t LearnerQORA::countUniquePredicatesObserved() const
	{
		std::set<Condition> observed;
		for (auto& it : predictors) {
			auto& ps = it.second.getPredicatesObserved();
			observed.insert(ps.begin(), ps.end());
		}
		return observed.size();
	}

	std::size_t LearnerQORA::countLastPredicatesObserved() const
	{
		return last_predicates_observed;
	}

	void LearnerQORA::reset()
	{
		effects_observed.clear();
		predictors.clear();
	}

	void LearnerQORA::restart()
	{
		//no history is kept for this learner, so this is blank
	}

	StateDistribution LearnerQORA::predictTransition(const State& state, ActionId action, Random& random) const
	{
		std::map<int, std::set<const Object*>> objects_by_type = state.getObjectsByType();
		//add blank set for each type
		for (auto& type : types.getObjectTypes()) {
			objects_by_type[type.id];
		}

		StateDistribution newState; //this stores all the objects with a future distribution

		for (auto& pair : state.getObjects()) {
			const Object& obj = pair.second;
			int obj_id = obj.getObjectId();
			int type_id = obj.getTypeId();
			const ObjectType& type = types.getObjectType(obj.getTypeId());
			//
			newState.addObject(type_id, obj_id);
			for (int attribute : type.attribute_types) {
				//
				EffectType e_type{ obj.getTypeId(), attribute };
				std::pair<EffectType, ActionId> key{ e_type, action };
				auto it = effects_observed.find(key);
				if (it == effects_observed.end()) {
					//no idea what will happen, never seen this combination of [obj type, attribute id, action]
					//assume nothing will happen
					newState.addObjectAttribute(obj_id, attribute, obj.getAttribute(attribute));
				}
				else {
					auto& effects = it->second;
					if (effects.size() > 1) {
						//complex predictor

						ProbabilityDistribution<Effect> es = predictors.at(key).predict(obj, objects_by_type);
						ProbabilityDistribution<AttributeValue> newVals;
						for (const auto& e_pair : es.getProbabilities()) {
							newVals.addProbability(obj.getAttribute(attribute) + e_pair.first, e_pair.second);
						}
						newState.addObjectAttribute(obj_id, attribute, newVals);
					}
					else if (effects.size() == 1) {
						//singleton predictor
						Effect e = *effects.begin();
						newState.addObjectAttribute(obj_id, attribute, obj.getAttribute(attribute) + e);
					}
				}
			}
		}
		return newState;
	}

	void LearnerQORA::observeTransition(const State& prevState, ActionId action, const State& nextState)
	{
		last_predicates_observed = 0;

		std::map<int, std::set<const Object*>> objects_by_type = prevState.getObjectsByType();
		//add blank set for each type
		for (auto& type : types.getObjectTypes()) {
			objects_by_type[type.id];
		}

		//record all effects and let the Predictor class take care of predicates
		State delta = nextState.diff(prevState);
		for (auto& pair : delta.getObjects()) {
			int id = pair.first;
			Object& obj = pair.second; //object
			for (const auto& attribs : obj.getAttributes()) {
				EffectType e_type{ obj.getTypeId(), attribs.first };
				Effect e = attribs.second;
				//
				std::pair<EffectType, ActionId> key{ e_type, action };
				//add to effects set?
				auto& effects = effects_observed[key];
				if (effects.find(e) == effects.end()) {
					effects.insert(e);
					if (effects.size() == 2) {
						//just noticed the second effect
						predictors[key] = StochasticEffectPredictor(alpha);
					}
				}
				//update observer
				auto it = predictors.find(key);
				if (it != predictors.end()) {
					it->second.observe(types, prevState.getObject(id), objects_by_type, e);
					last_predicates_observed += it->second.getCountPredicatesObserved();
				}
			}
		}
	}

	void LearnerQORA::print(FILE* f) const
	{
		fprintf(f, "Observations:\n");
		if (effects_observed.empty()) {
			fprintf(f, " none\n");
		}
		//print out the "interesting" effect types and their rules
		for (auto& pair : effects_observed) {
			if (pair.second.size() > 1) {
				EffectType type = pair.first.first;
				ActionId action = pair.first.second;
				fprintf(f, " %s %s.%s:\n", types.getActions().at(action).name.c_str(), types.getObjectType(type.object_type).name.c_str(), types.getAttributeType(type.attribute_type).name.c_str());
				predictors.at(pair.first).print(f, types, type);
			}
		}
		//print out all of the other possible effect types and their singular observed effect
		for (auto& pair : effects_observed) {
			if (pair.second.size() == 1) {
				EffectType type = pair.first.first;
				ActionId action = pair.first.second;
				fprintf(f, " %s %s.%s += %s\n", types.getActions().at(action).name.c_str(), types.getObjectType(type.object_type).name.c_str(), types.getAttributeType(type.attribute_type).name.c_str(), pair.second.begin()->to_string().c_str());
			}
		}
	}

	json LearnerQORA::to_json() const
	{
		json data;
		//store all observed effects
		//std::map<std::pair<EffectType, ActionId>, std::set<Effect>> effects_observed
		//stored as list of tuples: [(EffectType, ActionId, [Effect])]
		json& data_effects = data["effects"];
		for (const auto& pair : effects_observed) {
			json j{
				//{"effect_type", pair.first.first},
				{"action", types.getActions().at(pair.first.second).name},
				{"effects", pair.second}
			};
			l_qora::to_json(j["effect_type"], pair.first.first, types);
			data_effects.push_back(j);
		}

		//store all predictors
		//std::map<std::pair<EffectType, ActionId>, StochasticEffectPredictor> predictors
		//stored as list of tuples: [(EffectType, ActionId, StochasticEffectPredictor)]
		json& data_predictors = data["predictors"];
		for (const auto& pair : predictors) {
			json j{
				//{"effect_type", pair.first.first},
				{"action", types.getActions().at(pair.first.second).name},
				{"predictor", pair.second.to_json(types)}
			};
			l_qora::to_json(j["effect_type"], pair.first.first, types);
			data_predictors.push_back(j);
		}

		//
		return data;
	}

	void LearnerQORA::from_json(const json& j)
	{
		//load all observed effects
		//std::map<std::pair<EffectType, ActionId>, std::set<Effect>> effects_observed
		//stored as list of tuples: [(EffectType, ActionId, [Effect])]
		effects_observed.clear();
		const json& data_effects = j["effects"];
		for (const json& effect_tuple : data_effects) {
			EffectType e_type;
			l_qora::from_json(effect_tuple.at("effect_type"), e_type, types);
			int action = types.getActionByName(effect_tuple.at("action").get<std::string>());
			//
			std::set<Effect> effects;
			for (const json& effect : effect_tuple.at("effects")) {
				effects.insert(effect.get<Effect>());
			}
			//
			std::pair<EffectType, ActionId> key{ e_type, action };
			//
			effects_observed[key] = effects;
		}

		//load all predictors
		//std::map<std::pair<EffectType, ActionId>, StochasticEffectPredictor> predictors
		//stored as list of tuples: [(EffectType, ActionId, StochasticEffectPredictor)]
		predictors.clear();
		const json& data_predictors = j["predictors"];
		for (const json& predictor_tuple : data_predictors) {
			EffectType e_type;
			l_qora::from_json(predictor_tuple.at("effect_type"), e_type, types);
			int action = types.getActionByName(predictor_tuple.at("action").get<std::string>());
			//
			std::pair<EffectType, ActionId> key{ e_type, action };
			//
			predictors[key] = StochasticEffectPredictor(alpha);
			predictors[key].from_json(types, predictor_tuple.at("predictor"));
		}
	}

}