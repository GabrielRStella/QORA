#include "pch.h"
#include "Environment.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//actions
///////////////////////////////////////////////////////////////////////////////////////////////////

Action Action::NOOP{ ID_NOOP, "NOOP" };
Action Action::MOVE_LEFT{ ID_MOVE_LEFT, "MOVE_LEFT" };
Action Action::MOVE_RIGHT{ ID_MOVE_RIGHT, "MOVE_RIGHT" };
Action Action::MOVE_UP{ ID_MOVE_UP, "MOVE_UP" };
Action Action::MOVE_DOWN{ ID_MOVE_DOWN, "MOVE_DOWN" };

///////////////////////////////////////////////////////////////////////////////////////////////////
//types (attribute + object)
///////////////////////////////////////////////////////////////////////////////////////////////////

void to_json(json& j, const Action& t)
{
    j = t.name;
}

void from_json(const json& j, Action& t)
{
    j.get_to(t.name);
}

bool operator<(const AttributeType& a, const AttributeType& b)
{
    if (a.name < b.name) return true;
    if (a.name > b.name) return false;
    return a.size < b.size;
}

bool operator==(const AttributeType& a, const AttributeType& b)
{
    return a.size == b.size && a.name == b.name;
}

void to_json(json& j, const AttributeType& t)
{
    j = json{
        {"name", t.name},
        {"size", t.size}
    };
}

void from_json(const json& j, AttributeType& t)
{
    j.at("name").get_to(t.name);
    j.at("size").get_to(t.size);
}

bool operator<(const ObjectType& a, const ObjectType& b)
{
    if (a.name < b.name) return true;
    if (a.name > b.name) return false;
    return a.attribute_types < b.attribute_types;
}

bool operator==(const ObjectType& a, const ObjectType& b)
{
    return a.name == b.name && a.attribute_types == b.attribute_types;
}

void to_json(json& j, const ObjectType& t)
{
    j = json{
        {"name", t.name},
        {"attributes", t.attribute_types}
    };
}

void from_json(const json& j, ObjectType& t)
{
    j.at("name").get_to(t.name);
    j.at("attributes").get_to(t.attribute_types);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//state
///////////////////////////////////////////////////////////////////////////////////////////////////

const AttributeValue AttributeValue::UP{0, -1};
const AttributeValue AttributeValue::DOWN{0, 1};
const AttributeValue AttributeValue::LEFT{-1, 0};
const AttributeValue AttributeValue::RIGHT{1, 0};
const std::vector<AttributeValue> AttributeValue::DEFAULT_NEIGHBORS{ AttributeValue::UP, AttributeValue::DOWN, AttributeValue::LEFT, AttributeValue::RIGHT };

AttributeValue::AttributeValue(int sz) : sz(sz), data(new int[sz])
{
    if (sz > 0) memset(data, 0, sz * sizeof(*data));
}

AttributeValue::AttributeValue(std::initializer_list<int> values) : sz(values.size()), data(new int[sz])
{
    int index = 0;
    for (int i : values) data[index++] = i;
}

AttributeValue::AttributeValue(const AttributeValue& other)
{
    sz = other.sz;
    data = new int[sz];
    memcpy(data, other.data, sz * sizeof(*data));
}

AttributeValue& AttributeValue::operator=(const AttributeValue& other)
{
    delete[] data;
    sz = other.sz;
    data = new int[sz];
    memcpy(data, other.data, sz * sizeof(*data));
    return *this;
}

AttributeValue::~AttributeValue()
{
    delete[] data;
}

int AttributeValue::size() const
{
    return sz;
}

int& AttributeValue::get(int index)
{
    return data[index];
}

int AttributeValue::get(int index) const
{
    return data[index];
}

int& AttributeValue::operator[](int index)
{
    return data[index];
}

int AttributeValue::operator[](int index) const
{
    return data[index];
}

void AttributeValue::set(int index, int value)
{
    data[index] = value;
}

int* AttributeValue::ptr()
{
    return data;
}

const int* AttributeValue::ptr() const
{
    return data;
}

void AttributeValue::copy(int* dst) const
{
    memcpy(dst, data, sz * sizeof(*data));
}

AttributeValue AttributeValue::operator+(const AttributeValue& other) const
{
    AttributeValue val(sz);
    for (int i = 0; i < sz; i++) val.data[i] = data[i] + other.data[i];
    return val;
}

AttributeValue AttributeValue::operator-(const AttributeValue& other) const
{
    AttributeValue val(sz);
    for (int i = 0; i < sz; i++) val.data[i] = data[i] - other.data[i];
    return val;
}

AttributeValue AttributeValue::operator*(int scalar) const
{
    AttributeValue val(sz);
    for (int i = 0; i < sz; i++) val.data[i] = data[i] * scalar;
    return val;
}

AttributeValue& AttributeValue::operator+=(const AttributeValue& other)
{
    for (int i = 0; i < sz; i++) data[i] += other.data[i];
    return *this;
}

AttributeValue& AttributeValue::operator-=(const AttributeValue& other)
{
    for (int i = 0; i < sz; i++) data[i] -= other.data[i];
    return *this;
}

AttributeValue& AttributeValue::operator*=(int scalar)
{
    for (int i = 0; i < sz; i++) data[i] *= scalar;
    return *this;
}

bool AttributeValue::operator<(const AttributeValue& b) const
{
    if (sz < b.sz) return true;
    if (sz > b.sz) return false;
    //lexicographic ordering on the data...
    for (int i = 0; i < sz; i++) {
        int delta = data[i] - b.data[i];
        if (delta < 0) return true;
        if (delta > 0) return false;
    }
    //all components equal
    return false;
}

bool AttributeValue::operator==(const AttributeValue& b) const
{
    if (sz != b.sz) return false;
    for (int i = 0; i < sz; i++) {
        if (data[i] != b.data[i]) return false;
    }
    return true;
}

int AttributeValue::length() const
{
    int len = 0;
    for (int i = 0; i < sz; i++) {
        len += abs(data[i]);
    }
    return len;
}

std::string AttributeValue::to_string() const
{
    std::string s = "(";
    for (int i = 0; i < sz; i++) {
        s += std::to_string(data[i]);
        if (i < sz - 1) s += ", ";
    }
    return s + ')';
}

void to_json(json& j, const AttributeValue& p)
{
    j = json();
    for (int i = 0; i < p.size(); i++) {
        j.push_back(p[i]);
    }
}

void from_json(const json& j, AttributeValue& p)
{
    int n = j.size();
    p = AttributeValue(n);
    for (int i = 0; i < n; i++) {
        j[i].get_to(p[i]);
    }
}

ProbabilityDistribution<Object> Object::combine(const ProbabilityDistribution<Object>& obj_base, int attribute_id, const AttributeValue& attribute_value)
{
    ProbabilityDistribution<Object> obj;
    for (const auto& pair : obj_base.getProbabilities()) {
        Object o = pair.first;
        o.setAttribute(attribute_id, attribute_value);
        obj.setProbability(o, pair.second);
    }
    return obj;
}

ProbabilityDistribution<Object> Object::combine(const ProbabilityDistribution<Object>& obj_base, int attribute_id, const ProbabilityDistribution<AttributeValue>& attribute_values)
{
    ProbabilityDistribution<Object> obj;
    for (const auto& pair : obj_base.getProbabilities()) {
        double p_obj = pair.second;
        for (const auto& pair2 : attribute_values.getProbabilities()) {
            double p_attr = pair2.second;
            //new copy of object
            Object o = pair.first;
            //
            o.setAttribute(attribute_id, pair2.first);
            obj.setProbability(o, p_obj * p_attr);
        }
    }
    return obj;
}

Object::Object(int type_id, int object_id) : type(type_id), id(object_id)
{
}

int Object::getTypeId() const
{
    return type;
}

int Object::getObjectId() const
{
    return id;
}

AttributeValue& Object::addAttribute(int id, int size)
{
    return attributes[id] = AttributeValue(size);
}

AttributeValue& Object::getAttribute(int id)
{
    return attributes[id];
}

const AttributeValue& Object::getAttribute(int id) const
{
    return attributes.at(id);
}

void Object::setAttribute(int id, const AttributeValue& value)
{
    attributes[id] = value;
}

std::map<int, AttributeValue>& Object::getAttributes()
{
    return attributes;
}

const std::map<int, AttributeValue>& Object::getAttributes() const
{
    return attributes;
}

bool Object::hasAttribute(int id) const
{
    return attributes.find(id) != attributes.end();
}

bool Object::operator<(const Object& b) const
{
    if (type < b.type) return true;
    if (type > b.type) return false;
    //
    if (id < b.id) return true;
    if (id > b.id) return false;
    //can't really compare the attributes, so just leave them
    return attributes < b.attributes;
}

bool Object::operator==(const Object& b) const
{
    return b.type == type && b.id == id && attributes == b.attributes;
}

int Object::distance(const Object& other) const
{
    int sum = 0;
    //
    const std::map<int, AttributeValue>& other_attributes = other.attributes;
    for (const auto& pair : attributes) {
        const AttributeValue& v1 = pair.second;
        const AttributeValue& v2 = other_attributes.at(pair.first);
        //
        sum += (v1 - v2).length();
    }
    return sum;
}

State::State() : nextObjectId(0)
{
}

void State::clear()
{
    objects.clear();
    nextObjectId = 0;
}

int State::getNextObjectId()
{
    return nextObjectId++;
}

Object& State::add(const Object& object)
{
    return objects[object.getObjectId()] = object;
}

void State::remove(int objectId)
{
    objects.erase(objectId);
}

std::map<int, Object>& State::getObjects()
{
    return objects;
}

const std::map<int, Object>& State::getObjects() const
{
    return objects;
}

Object& State::getObject(int id)
{
    return objects.at(id);
}

const Object& State::getObject(int id) const
{
    return objects.at(id);
}

std::set<Object*> State::getObjectsOfClass(int type)
{
    std::set<Object*> objs;
    for (auto& pair : objects) {
        Object& obj = pair.second;
        if (obj.getTypeId() == type) objs.insert(&obj);
    }
    return objs;
}

std::set<const Object*> State::getObjectsOfClass(int type) const
{
    std::set<const Object*> objs;
    for (auto& pair : objects) {
        const Object& obj = pair.second;
        if (obj.getTypeId() == type) objs.insert(&obj);
    }
    return objs;
}

std::map<int, std::set<const Object*>> State::getObjectsByType() const
{
    std::map<int, std::set<const Object*>> objects_by_type;
    for (auto& obj_pair : objects) {
        int object_id = obj_pair.first;
        const Object* obj = &obj_pair.second;
        int type_id = obj->getTypeId();
        objects_by_type[type_id].insert(obj);
    }
    return objects_by_type;
}

State State::diff(const State& prev) const
{
    State diff;
    diff.nextObjectId = nextObjectId;
    for (const auto& pair : objects) {
        int id = pair.first;
        const Object& obj = pair.second;
        const Object& prevObj = prev.objects.at(id); //this will crash if it's not there :^) hehehe
        //
        Object diffObj(obj.getTypeId(), id);
        //
        for (const auto& pair : obj.getAttributes()) {
            diffObj.getAttribute(pair.first) = pair.second - prevObj.getAttribute(pair.first);
        }
        //
        diff.objects[id] = diffObj;
    }
    return diff;
}

int State::length() const
{
    int len = 0;
    for (const auto& pair : objects) {
        const Object& obj = pair.second;
        //
        for (const auto& pair : obj.getAttributes()) {
            len += pair.second.length();
        }
    }
    return len;
}

int State::error(const State& other) const
{
    //TODO: handle mismatch in object sets
    return diff(other).length();
}

bool State::operator==(const State& other) const
{
    return objects == other.objects;
}

bool State::operator<(const State& other) const
{
    return objects < other.objects;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//probabilistic states
///////////////////////////////////////////////////////////////////////////////////////////////////

double StateDistribution::calc_EarthMoversDistance(const ProbabilityDistribution<Object>& objs1, const ProbabilityDistribution<Object>& objs2)
{
    double err = 0;
    //https://en.wikipedia.org/wiki/Minimum-cost_flow_problem
    //current impl is just a greedy approximation that matches the two closest Objects until none are left
    //it'll be an upper bound on the true error (since the optimal matching would have lowest possible error)
    //and in my use cases, it'll be close or exact
    ProbabilityDistribution<Object> d1 = objs1;
    ProbabilityDistribution<Object> d2 = objs2;
    while (d1.size() > 0 && d2.size() > 0) {
        Object best_o1;
        Object best_o2;
        int best_dist = -1;
        //find closest pair of objects
        for (auto& pair1 : d1.getProbabilities()) {
            const Object& o1 = pair1.first;
            for (auto& pair2 : d2.getProbabilities()) {
                const Object& o2 = pair2.first;
                //
                int d = o1.distance(o2);
                if (best_dist < 0 || d < best_dist) {
                    best_o1 = o1;
                    best_o2 = o2;
                    best_dist = d;
                }
            }
        }
        //add error and subtract probabilities
        double d = std::min(d1.getProbability(best_o1), d2.getProbability(best_o2));
        err += d * best_dist;
        //
        d1.addProbability(best_o1, -d);
        d2.addProbability(best_o2, -d);
    }
    //
    return err;
}

StateDistribution::StateDistribution()
{
}

StateDistribution::StateDistribution(const State& state)
{
    //iterate over each object and add singleton
    for (const auto& pair : state.getObjects()) {
        objects[pair.first] = ProbabilityDistribution<Object>(pair.second);
    }
}

void StateDistribution::addObject(int type_id, int obj_id)
{
    objects[obj_id] = ProbabilityDistribution<Object>(Object(type_id, obj_id));
}

void StateDistribution::addObject(const ProbabilityDistribution<Object>& distribution)
{
    assert (distribution.size() > 0);
    //
    int obj_id = distribution.getProbabilities().begin()->first.getObjectId();
    objects[obj_id] = distribution;
}

void StateDistribution::addObjectAttribute(int obj_id, int attribute_id, const AttributeValue& attribute_value)
{
    ProbabilityDistribution<Object>& distr = objects.at(obj_id);
    distr = Object::combine(distr, attribute_id, attribute_value);
}

void StateDistribution::addObjectAttribute(int obj_id, int attribute_id, const ProbabilityDistribution<AttributeValue>& attribute_values)
{
    ProbabilityDistribution<Object>& distr = objects.at(obj_id);
    distr = Object::combine(distr, attribute_id, attribute_values);
}

double StateDistribution::error(const StateDistribution& other) const
{
    double err = 0;
    //for each object that is in both distributions,
    const std::map<int, ProbabilityDistribution<Object>>& other_objects = other.objects;
    for (const auto& pair : objects) {
        int id = pair.first;
        const ProbabilityDistribution<Object>& objs = pair.second;
        if (other_objects.find(id) != other_objects.end()) {
            //object exists in both
            const ProbabilityDistribution<Object>& objs2 = other_objects.at(id);
            //
            //now, calc EMD between the two object distributions :^)
            err += calc_EarthMoversDistance(objs, objs2);
        }
    }
    return err;
}

double StateDistribution::error(const State& other) const
{
    double err = 0;
    //
    const std::map<int, Object>& other_objects = other.getObjects();
    for (const auto& pair : objects) {
        int id = pair.first;
        const ProbabilityDistribution<Object>& objs = pair.second;
        if (other_objects.find(id) != other_objects.end()) {
            //object exists in both, calc weighted errors
            const Object& o = other_objects.at(id);
            for (const auto& opair : objs.getProbabilities()) {
                err += o.distance(opair.first) * opair.second;
            }
        }
    }
    return err;
}

State StateDistribution::sample(Random& random) const
{
    //for each object, sample and add to a state
    State s;
    for (const auto& pair : objects) {
        s.add(pair.second.sample(random));
    }
    return s;
}

ProbabilityDistribution<State> StateDistribution::toDistribution() const
{
    //time for cartesian product...
    assert(false); //TMP TODO
    return ProbabilityDistribution<State>();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//environment
///////////////////////////////////////////////////////////////////////////////////////////////////

Types::Types() : attribute_types(), object_types()
{
}

int Types::addAttributeType(const std::string& name, int size)
{
    attribute_types.push_back(AttributeType{ name, size, -1 });
    int index = attribute_types.size() - 1;
    attribute_types[index].id = index;
    attribute_type_names[name] = index;
    return index;
}

AttributeType& Types::getAttributeType(int id)
{
    return attribute_types.at(id);
}

const AttributeType& Types::getAttributeType(int id) const
{
    return attribute_types.at(id);
}

AttributeType& Types::getAttributeType(const std::string& name)
{
    return getAttributeType(attribute_type_names.at(name));
}

const AttributeType& Types::getAttributeType(const std::string& name) const
{
    return getAttributeType(attribute_type_names.at(name));
}

std::vector<AttributeType>& Types::getAttributeTypes()
{
    return attribute_types;
}

const std::vector<AttributeType>& Types::getAttributeTypes() const
{
    return attribute_types;
}

int Types::addObjectType(const std::string& name)
{
    object_types.push_back(ObjectType{ name, {}, -1 });
    int index = object_types.size() - 1;
    object_types[index].id = index;
    object_type_names[name] = index;
    return index;
}

ObjectType& Types::getObjectType(int id)
{
    return object_types.at(id);
}

const ObjectType& Types::getObjectType(int id) const
{
    return object_types.at(id);
}

ObjectType& Types::getObjectType(const std::string& name)
{
    return getObjectType(object_type_names.at(name));
}

const ObjectType& Types::getObjectType(const std::string& name) const
{
    return getObjectType(object_type_names.at(name));
}

std::vector<ObjectType>& Types::getObjectTypes()
{
    return object_types;
}

const std::vector<ObjectType>& Types::getObjectTypes() const
{
    return object_types;
}

Object Types::createObject(int type_id, int object_id) const
{
    Object obj(type_id, object_id);
    //add all attributes as blank
    for (int attribute_type : object_types[type_id].attribute_types) {
        obj.addAttribute(attribute_type, attribute_types[attribute_type].size);
    }
    //
    return obj;
}

void Types::addAction(const Action& action)
{
    ActionId id = action.id;
    if (actions.size() <= id) actions.resize(id + 1);
    actions[id] = action;
    action_names[action.name] = id;
}

void Types::addStandardActions()
{
    assert(actions.empty());
    //
    actions.push_back(Action::NOOP);
    actions.push_back(Action::MOVE_LEFT);
    actions.push_back(Action::MOVE_RIGHT);
    actions.push_back(Action::MOVE_UP);
    actions.push_back(Action::MOVE_DOWN);
    //
    for (const Action& a : actions) {
        action_names[a.name] = a.id;
    }
}

ActionId Types::newAction(const ActionName& name)
{

    ActionId id = actions.size();
    actions.push_back(Action{ id, name });
    action_names[name] = id;
    return id;
}

std::vector<Action>& Types::getActions()
{
    return actions;
}

const std::vector<Action>& Types::getActions() const
{
    return actions;
}

ActionId Types::getActionByName(const ActionName& name) const
{
    return action_names.at(name);
}

void Types::print(const State& state) const
{
    const auto& objs = state.getObjects();
    printf("Objects (%zu):\n", objs.size());
    for (const auto& pair : objs) {
        int id = pair.first;
        const Object& obj = pair.second;
        const ObjectType& obj_type = object_types.at(obj.getTypeId());
        //id
        printf(" [%02d] ", pair.first);
        //type
        printf("%s:", obj_type.name.c_str());
        //attributes
        for (int attribute_id : obj_type.attribute_types) {
            printf(" %s=%s", attribute_types.at(attribute_id).name.c_str(), obj.getAttribute(attribute_id).to_string().c_str());
        }
        //
        printf("\n");
    }
}

json Types::to_json(const Object& obj) const
{
    json attributes;
    for (const auto& apair : obj.getAttributes()) {
        const std::string& aname = attribute_types.at(apair.first).name;
        const AttributeValue& val = apair.second;
        int n = val.size();
        std::vector<int> avalue(n);
        for (int i = 0; i < n; i++) {
            avalue[i] = val[i];
        }
        attributes[aname] = avalue;
    }
    //
    return json{
        {"id", obj.getObjectId()},
        {"type", object_types.at(obj.getTypeId()).name},
        {"attributes", attributes}
    };
}

void Types::from_json(Object& obj, const json& jobj) const
{
    int id = jobj["id"].get<int>();
    const std::string& type = jobj["type"].get<std::string>();
    const json& jattributes = jobj["attributes"];
    //
    obj = Object(object_type_names.at(type), id);
    //add all attributes
    for (auto& el : jattributes.items()) {
        const std::string& aname = el.key();
        int atype = attribute_type_names.at(aname);
        const json& avalue = el.value();
        AttributeValue& val = obj.addAttribute(atype, attribute_types.at(atype).size);
        int n = val.size();
        for (int i = 0; i < n; i++) {
            val[i] = avalue[i].get<int>();
        }
    }
}

json Types::to_json(const State& state) const
{
    json j;
    for (const auto& pair : state.getObjects()) {
        const Object& obj = pair.second;
        j.push_back(to_json(obj));
    }
    return j;
}

void Types::from_json(State& s, const json& j) const
{
    s.clear();
    for (const json& jobj : j) {
        //
        Object obj;
        from_json(obj, jobj);
        //
        s.add(obj);
    }
}

void Types::to_json(const ProbabilityDistribution<State>& states, json& j) const
{
    for (const auto& pair : states.getProbabilities()) {
        j.push_back(json{
            {"state", to_json(pair.first)},
            {"probability", pair.second}
        });
    }
}

void Types::from_json(ProbabilityDistribution<State>& states, const json& j) const
{
    states.clear();
    for (const json& jstate : j) {
        State s;
        from_json(s, jstate.at("state"));
        states.setProbability(s, jstate.at("probability").get<double>());
    }
}

void Types::to_json(const StateDistribution& states, json& j) const
{
}

void Types::from_json(StateDistribution& states, const json& j) const
{
}

void Types::print() const
{
    //actions
    printf("Actions:\n");
    for (const Action& a : actions) {
        printf(" [%d] %s\n", a.id, a.name.c_str());
    }
    //attributes
    printf("Attributes:\n");
    for (const AttributeType& t : attribute_types) {
        printf(" [%d] %s (%d)\n", t.id, t.name.c_str(), t.size);
    }
    //objects
    printf("Classes:\n");
    for (const ObjectType& t : object_types) {
        printf(" [%d] %s\n", t.id, t.name.c_str());
        for (int attr_id : t.attribute_types) {
            printf("  [%d] %s\n", attr_id, attribute_types.at(attr_id).name.c_str());
        }
    }
}

json Types::to_json() const
{
    /*
    * attribute types
    *  array of {name, size}
    * object types
    *  array of {name, [attribute ids]}
    * actions
    *  array of names
    */
    json object_data;
    for (const ObjectType& t : object_types) {
        json attrs;
        for (int attr_id : t.attribute_types) {
            attrs.push_back(attribute_types.at(attr_id).name);
        }
        object_data.push_back(json{
            {"name", t.name},
            {"attributes", attrs}
        });
    }
    return json{
        {"attributes", attribute_types},
        {"classes", object_data},
        {"actions", actions}
    };
}

std::pair<IntTensor, std::vector<int>> Types::flatten(const State& state, int w, int h, int ATTR_POS) const
{
    //first, figure out what the depth is:
    //each position-having class gets 1+n layers, where n is the total size of that class' other attributes
    int d = 0;
    std::map<int, int> class_index; //the starting (depth) index that objects of each class are stored at (first a 1 to signal their presence, then their other attribute values)
    std::map<int, std::map<int, int>> class_attr_index; //depth that each attribute goes into for each class; [class][attrib] = d
    std::set<int> positionless_types; //obj types that don't have a position
    for (const ObjectType& t : object_types) {
        if (t.attribute_types.find(ATTR_POS) != t.attribute_types.end()) {
            //has position
            class_index[t.id] = d;
            d++;
            class_attr_index[t.id] = {}; //just in case there are no non-pos attribs
            for (int attr_id : t.attribute_types) {
                if (attr_id != ATTR_POS) {
                    class_attr_index[t.id][attr_id] = d;
                    d += attribute_types.at(attr_id).size;
                }
            }
        }
        else {
            //no position, objs of this class go to the end
            positionless_types.insert(t.id);
        }
    }
    //
    IntTensor grid(w, h, d);
    std::vector<int> data;
    //
    for (const auto& p : state.getObjects()) {
        const Object& o = p.second;
        int obj_type = o.getTypeId();
        if (positionless_types.find(obj_type) != positionless_types.end()) {
            //this is a positionless object, tack its info on to the data vector
            for (int attr_id : object_types.at(obj_type).attribute_types) {
                const AttributeValue& val = o.getAttribute(attr_id);
                //push val into data
                data.insert(data.end(), val.ptr(), val.ptr() + val.size());
            }
        }
        else {
            //this object has position data, put it in the grid
            const AttributeValue& pos = o.getAttribute(ATTR_POS);
            int x = pos[0], y = pos[1];
            int* arr = grid.block(x, y);
            arr[class_index.at(obj_type)] = 1;
            //and put its attributes there too
            auto& indices = class_attr_index.at(obj_type);
            for (int attr_id : object_types.at(obj_type).attribute_types) {
                if (attr_id != ATTR_POS) {
                    int idx = indices.at(attr_id);
                    const AttributeValue& val = o.getAttribute(attr_id);
                    //copy
                    val.copy(arr + idx);
                }
            }
        }
    }
    //
    return {grid, data};
}

Environment::Environment(int width, int height) : width(width), height(height), ATTR_POS(-1)
{
}

AttributeValue Environment::random_position(Random& random) const
{
    return AttributeValue{random.random_int(0, width - 1), random.random_int(0, height - 1)};
}

void Environment::create_border(int object_type_id, int attribute_type_id, State& state) const
{
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                AttributeValue pos{ x, y };
                Object& obj = state.add(types.createObject(object_type_id, state.getNextObjectId()));
                obj.setAttribute(attribute_type_id, pos);
            }
        }
    }
}

void Environment::create_border(int object_type_id, int attribute_type_id, State& state, int thickness) const
{
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            if (x < thickness || x >= width - thickness || y < thickness || y >= height - thickness) {
                AttributeValue pos{ x, y };
                Object& obj = state.add(types.createObject(object_type_id, state.getNextObjectId()));
                obj.setAttribute(attribute_type_id, pos);
            }
        }
    }
}

bool Environment::is_empty(int attribute_type_id, AttributeValue pos, const State& state) const
{
    for (const auto& pair : state.getObjects()) {
        if (pair.second.hasAttribute(attribute_type_id) && pair.second.getAttribute(attribute_type_id) == pos) return false;
    }
    return true;
}

AttributeValue Environment::find_random_empty_position(int attribute_type_id, const State& state, Random& random) const
{
    AttributeValue pos;
    do {
        pos = random_position(random);
    } while (!is_empty(attribute_type_id, pos, state));
    return pos;
}

bool Environment::check_path_recursive(const IntTensor& walls, const AttributeValue& start_pos, const AttributeValue& end_pos) const
{
    if (start_pos[0] < 0 || start_pos[0] >= width || start_pos[1] < 0 || start_pos[1] >= height) return false;
    //wall at current position?
    if (walls.at(start_pos[0], start_pos[1], 0) > 0) return false;
    //at destination?
    if (start_pos == end_pos) return true;
    //
    AttributeValue delta = end_pos - start_pos;
    int dx = delta[0];
    int dy = delta[1];
    //check one step in each direction
    if (dx < 0) {
        if (check_path_recursive(walls, start_pos + AttributeValue({ -1, 0 }), end_pos)) return true;
    }
    if (dx > 0) {
        if (check_path_recursive(walls, start_pos + AttributeValue({ 1, 0 }), end_pos)) return true;
    }
    if (dy < 0) {
        if (check_path_recursive(walls, start_pos + AttributeValue({ 0, -1 }), end_pos)) return true;
    }
    if (dy > 0) {
        if (check_path_recursive(walls, start_pos + AttributeValue({ 0, 1 }), end_pos)) return true;
    }
    //nope
    return false;
}

bool Environment::is_connected(int attribute_type_id, const State& state, const std::vector<AttributeValue>& neighbor_deltas) const
{
    //graph search over entire grid to ensure all non-wall squares are reachable from each other
    //is this the most optimized algorithm? no. does it matter enough to optimize more? probably not

    IntTensor walls(width, height, 1); //UPDATE: need to check more-than-one-step paths for knight domain

    //start by adding all positions,
    std::set<AttributeValue> open; //set of all open spaces
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            open.insert(AttributeValue{x, y});
        }
    }
    //then remove the ones with objects in them
    for (const auto& pair : state.getObjects()) {
        if (pair.second.hasAttribute(attribute_type_id)) {
            const AttributeValue& pos = pair.second.getAttribute(attribute_type_id);
            walls.at(pos[0], pos[1], 0) = 1;
            open.erase(pos);
        }
    }
    //are there any open spaces at all?
    if (!open.size()) return false; //what heck happened?
    //now start the process
    //basically we just visit all cells connected to some arbitrary starting position and remove them from the set
    //once all connected cells have been visited, we check if the set is empty
    std::queue<AttributeValue> frontier;
    AttributeValue start = *open.begin(); //likely near the bottom-left corner, but it doesn't really matter -- some arbitrary starting cell
    frontier.push(start);
    open.erase(start);
    while (frontier.size()) {
        AttributeValue curr = frontier.front();
        frontier.pop();
        //
        for (AttributeValue delta : neighbor_deltas) {
            AttributeValue next = curr + delta;
            auto iter = open.find(next);
            //skip neighbors that either: aren't open, or have been seen (and therefore removed)
            if (iter == open.end()) continue;
            //make sure it's actually reachable from here
            if (!check_path_recursive(walls, curr, next)) continue;
            //if it is open and unseen, then:
            //erase from open (since it's a direct neighbor of a reachable open square, it must also be reachable)
            open.erase(iter);
            //add it to frontier
            frontier.push(next);
        }
    }
    //check if all of the open squares have been seen
    return open.empty();
}

Types& Environment::getTypes()
{
    return types;
}

const Types& Environment::getTypes() const
{
    return types;
}

int Environment::getWidth() const
{
    return width;
}

int Environment::getHeight() const
{
    return height;
}

int Environment::getPositionAttribute() const
{
    return ATTR_POS;
}

State Environment::createRandomGoalState(const State& start, int max_moves, Random& random) const
{
    State curr = start;
    //
    do {
        int num_actions = max_moves; // random.random_int(max_moves) + 1;
        while (num_actions > 0) {
            //
            const Action& a = random.sample(types.getActions());
            //printf("Action: %s\n", a.name.c_str());
            StateDistribution futures = act(curr, a.id, random);
            curr = futures.sample(random);
            //
            num_actions--;
        }
    } while (curr == start);
    return curr;
}

State Environment::hideInformation(const State& state) const
{
    return state; //default: do nothing (Markovian)
}

std::pair<IntTensor, std::vector<int>> Environment::flatten(const State& state) const
{
    return types.flatten(state, width, height, ATTR_POS);
}