#pragma once

#include "Random.h"
#include "util.h"
#include "ProbabilityDistribution.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//actions
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef int ActionId;
typedef std::string ActionName;

struct Action {
	ActionId id;
	ActionName name;
	//some "standard actions" for convenience
	constexpr static int ID_NOOP = 0;
	constexpr static int ID_MOVE_LEFT = 1;
	constexpr static int ID_MOVE_RIGHT = 2;
	constexpr static int ID_MOVE_UP = 3;
	constexpr static int ID_MOVE_DOWN = 4;
	static Action NOOP;
	static Action MOVE_LEFT;
	static Action MOVE_RIGHT;
	static Action MOVE_UP;
	static Action MOVE_DOWN;
};

void to_json(json& j, const Action& t);
void from_json(const json& j, Action& t);

///////////////////////////////////////////////////////////////////////////////////////////////////
//types (attribute + object)
///////////////////////////////////////////////////////////////////////////////////////////////////

struct AttributeType {
	std::string name;
	int size;
	//
	int id = -1;
};

bool operator<(const AttributeType& a, const AttributeType& b);
bool operator==(const AttributeType& a, const AttributeType& b);

void to_json(json& j, const AttributeType& t);
void from_json(const json& j, AttributeType& t);

struct ObjectType {
	std::string name;
	std::set<int> attribute_types;
	//
	int id = -1;
};

bool operator<(const ObjectType& a, const ObjectType& b);
bool operator==(const ObjectType& a, const ObjectType& b);

void to_json(json& j, const ObjectType& t);
void from_json(const json& j, ObjectType& t);

///////////////////////////////////////////////////////////////////////////////////////////////////
//state
///////////////////////////////////////////////////////////////////////////////////////////////////

//basically just a simple vector type, that supports element-wise addition/subtraction for convenience
class AttributeValue {
	int sz;
	int* data;
	//
public:
	const static AttributeValue UP;
	const static AttributeValue DOWN;
	const static AttributeValue LEFT;
	const static AttributeValue RIGHT;
	const static std::vector<AttributeValue> DEFAULT_NEIGHBORS;
public:
	AttributeValue(int sz = 0);
	AttributeValue(std::initializer_list<int> values);
	AttributeValue(const AttributeValue& other);
	AttributeValue& operator=(const AttributeValue& other);
	~AttributeValue();
	//
	int size() const;
	int& get(int index);
	int get(int index) const;
	int& operator[](int index);
	int operator[](int index) const;
	void set(int index, int value);
	//
	int* ptr();
	const int* ptr() const;
	void copy(int* dst) const;
	//
	AttributeValue operator+(const AttributeValue& other) const;
	AttributeValue operator-(const AttributeValue& other) const;
	AttributeValue operator*(int scalar) const;
	AttributeValue& operator+=(const AttributeValue& other);
	AttributeValue& operator-=(const AttributeValue& other);
	AttributeValue& operator*=(int scalar);
	//
	bool operator<(const AttributeValue& b) const;
	bool operator==(const AttributeValue& b) const;
	//
	int length() const; //manhattan distance (sum of abs)
	std::string to_string() const;
};

void to_json(json& j, const AttributeValue& p);
void from_json(const json& j, AttributeValue& p);

class Object {
public:
	static ProbabilityDistribution<Object> combine(const ProbabilityDistribution<Object>& obj_base, int attribute_id, const AttributeValue& attribute_value);
	static ProbabilityDistribution<Object> combine(const ProbabilityDistribution<Object>& obj_base, int attribute_id, const ProbabilityDistribution<AttributeValue>& attribute_values);

private:
	int type; //the class of this object (player, goal, wall, ...)
	int id; //the unique id of this object
	//
	std::map<int, AttributeValue> attributes;
public:
	Object(int type_id = -1, int object_id = -1);
	int getTypeId() const;
	int getObjectId() const;
	//
	AttributeValue& addAttribute(int id, int size);
	AttributeValue& getAttribute(int id);
	const AttributeValue& getAttribute(int id) const;
	void setAttribute(int id, const AttributeValue& value);
	std::map<int, AttributeValue>& getAttributes();
	const std::map<int, AttributeValue>& getAttributes() const;
	bool hasAttribute(int id) const;

	bool operator<(const Object& b) const; //for usage in map
	bool operator==(const Object& b) const; //for usage in map
	//
	int distance(const Object& other) const;
};

class State {
private:
	int nextObjectId;
	std::map<int, Object> objects;
public:
	State();
	//
	void clear();
	int getNextObjectId();
	Object& add(const Object& object); //add a copy of this object and return a reference to the stored copy
	void remove(int objectId);
	//
	std::map<int, Object>& getObjects();
	const std::map<int, Object>& getObjects() const;
	Object& getObject(int id);
	const Object& getObject(int id) const;
	//
	std::set<Object*> getObjectsOfClass(int type);
	std::set<const Object*> getObjectsOfClass(int type) const;
	//
	std::map<int, std::set<const Object*>> getObjectsByType() const;
	//return set of attributes that have changed
	//each Object in the returned state actually contains the derivatives of its values, not the values themselves
	//this can be used repeatedly to get nth derivatives
	State diff(const State& prev) const;
	int length() const; //return the sum of abs of each attribute of each object (so diff->length gives error of prediction)
	int error(const State& other) const; //gives error including object set mismatches

	//
	bool operator==(const State& other) const;
	bool operator<(const State& other) const;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//probabilistic states
///////////////////////////////////////////////////////////////////////////////////////////////////

class StateDistribution {
	std::map<int, ProbabilityDistribution<Object>> objects; //for each object id, keep track of a distribution over the attribute values for that object

	//helper
	static double calc_EarthMoversDistance(const ProbabilityDistribution<Object>& objs1, const ProbabilityDistribution<Object>& objs2);
public:
	StateDistribution();
	StateDistribution(const State& state); //create a singleton distribution for each object
	//object manipulation
	void addObject(int type_id, int obj_id); //adds a new empty object to the distribution
	void addObject(const ProbabilityDistribution<Object>& distribution); //adds a new object with distribution over values
	void addObjectAttribute(int obj_id, int attribute_id, const AttributeValue& attribute_value); //used to extend an existing object's distribution
	void addObjectAttribute(int obj_id, int attribute_id, const ProbabilityDistribution<AttributeValue>& attribute_values); //used to extend an existing object's distribution
	//error calculation
	double error(const StateDistribution& other) const;
	double error(const State& other) const;
	//
	State sample(Random& random) const;
	ProbabilityDistribution<State> toDistribution() const; //multiply out all possibilities to get a big state
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//environment
///////////////////////////////////////////////////////////////////////////////////////////////////

class Types {
private:
	std::vector<AttributeType> attribute_types;
	std::map<std::string, int> attribute_type_names;
	std::vector<ObjectType> object_types;
	std::map<std::string, int> object_type_names;
	//
	std::vector<Action> actions;
	std::map<ActionName, ActionId> action_names;
	//
public:
	Types();
	//
	int addAttributeType(const std::string& name, int size);
	AttributeType& getAttributeType(int id);
	const AttributeType& getAttributeType(int id) const;
	AttributeType& getAttributeType(const std::string& name);
	const AttributeType& getAttributeType(const std::string& name) const;
	std::vector<AttributeType>& getAttributeTypes();
	const std::vector<AttributeType>& getAttributeTypes() const;
	//
	int addObjectType(const std::string& name);
	ObjectType& getObjectType(int id);
	const ObjectType& getObjectType(int id) const;
	ObjectType& getObjectType(const std::string& name);
	const ObjectType& getObjectType(const std::string& name) const;
	std::vector<ObjectType>& getObjectTypes();
	const std::vector<ObjectType>& getObjectTypes() const;
	//create a blank object (init all attributes to 0)
	Object createObject(int type_id, int object_id) const;
	//
	void addAction(const Action& action);
	void addStandardActions(); //noop and movement
	ActionId newAction(const ActionName& name); //auto-assign id
	std::vector<Action>& getActions();
	const std::vector<Action>& getActions() const;
	ActionId getActionByName(const ActionName& name) const;
	//
	void print(const State& state) const;
	//
	json to_json(const Object& obj) const;
	void from_json(Object& obj, const json& j) const;
	json to_json(const State& state) const;
	void from_json(State& s, const json& j) const;
	void to_json(const ProbabilityDistribution<State>& states, json& j) const;
	void from_json(ProbabilityDistribution<State>& states, const json& j) const;
	void to_json(const StateDistribution& states, json& j) const;
	void from_json(StateDistribution& states, const json& j) const;
	//
	void print() const; //print out info about the types
	json to_json() const; //export info about the types
	//
	//flatten a state into a grid representation for NN use; objects without a position component are tacked on at the end as a 1d vector
	//the tensor has several layers:
	//first, if there are n distinct classes with a position component, there are n layers, each 0/1 encoding whether an object of that type is present there
	//second, for each position-containing class, there is a series of layers to store all of their other-attribute data
	//note: although this works often, *in general* there is no way to collapse the object-based states to any fixed-size (tensor) representation, due to the possibility of overlapping objects
	std::pair<IntTensor, std::vector<int>> flatten(const State& state, int w, int h, int ATTR_POS) const;
};

class Environment
{
protected:
	int width;
	int height;
	int ATTR_POS; //id of the position attribute, if there is one; should be set by subclass
	//
	//
	Types types;
	//
	Environment(int width, int height);
	//level generation helpers
	AttributeValue random_position(Random& random) const;
	void create_border(int object_type_id, int attribute_type_id, State& state) const; //creates a border of an object type in a rectangle of a given size, using the given attribute as position
	void create_border(int object_type_id, int attribute_type_id, State& state, int thickness) const; //creates a border of an object type in a rectangle of a given size, using the given attribute as position
	bool is_empty(int attribute_type_id, AttributeValue pos, const State& state) const; //checks to ensure there are no objects with a given attribute type equal to a given value (e.g., no objects at a position)
	AttributeValue find_random_empty_position(int attribute_type_id, const State& state, Random& random) const;

	bool check_path_recursive(const IntTensor& walls, const AttributeValue& start_pos, const AttributeValue& end_pos) const;
	bool is_connected(int attribute_type_id, const State& state, const std::vector<AttributeValue>& neighbor_deltas) const; //check if all empty squares in the rectangle are connected to each other
public:
	//get information about the environment
	Types& getTypes();
	const Types& getTypes() const;
	//
	int getWidth() const;
	int getHeight() const;
	int getPositionAttribute() const; //which attribute denotes position in this environment?
	//create random starting state
	virtual State createRandomState(Random& random) const = 0;
	//create random goal state, starting from some state
	//if possible, the number of actions required to get to the goal should be limited to max_moves (unless max_moves == 0)
	//default behavior: just choose some # in {1, ..., max_moves} random actions and execute/sample each
	virtual State createRandomGoalState(const State& start, int max_moves, Random& random) const;
	//act(State, Action) -> return empty state if game is over and should restart
	//returning a probability distribution over every possible future state may SEEM like a bad idea,
	//but that's because it is :)
	//but most of the current domains have only 1 or a small number of possible future states,
	//so it won't be a problem currently
	//and more complex domains can just return a single state (sampled) to prevent issues
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const = 0;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const = 0; //print to stdout

	//for non-Markovian environments: generate the observable State (by e.g. setting some attribute values to 0 so the learner gets no information about them)
	//default: do nothing (Markovian)
	virtual State hideInformation(const State& state) const;

	//helpers for things like DOORMAX and NNs

	//a helper that converts to a WxHx? grid of entries for use with NNs
	//default behavior uses Types::flatten, but can be overridden
	virtual std::pair<IntTensor, std::vector<int>> flatten(const State& state) const;
};

