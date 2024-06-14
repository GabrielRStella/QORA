#include "pch.h"
#include "Domains.h"

constexpr double WALL_RATIO = 0.3;

DomainTest::DomainTest() : Environment(2, 2)
{
	ACTION_UP = types.newAction("UP");
	ACTION_DOWN = types.newAction("DOWN");
	//
	ATTR_POS = types.addAttributeType("position", 2);
	ATTR_COUNT = types.addAttributeType("count", 1);
	//
	CLASS_PLAYER = types.addObjectType("player");
	types.getObjectType(CLASS_PLAYER).attribute_types.insert(ATTR_POS);
	types.getObjectType(CLASS_PLAYER).attribute_types.insert(ATTR_COUNT);
	CLASS_WALL = types.addObjectType("wall");
	types.getObjectType(CLASS_WALL).attribute_types.insert(ATTR_POS);
}

State DomainTest::createRandomState(Random& random) const
{
	State state;
	//
	Object& wall = state.add(types.createObject(CLASS_WALL, state.getNextObjectId()));
	wall.setAttribute(ATTR_POS, AttributeValue{ 0, 0 });
	//
	Object& player = state.add(types.createObject(CLASS_PLAYER, state.getNextObjectId()));
	player.setAttribute(ATTR_POS, AttributeValue{ 1, 1 });
	player.setAttribute(ATTR_COUNT, AttributeValue{ random.random_int(-100, 100)}); //just to have some variety
	//
	return state;
}

StateDistribution DomainTest::act(const State& current, ActionId action, Random& random) const
{
	State state = current;
	//
	Object* const ptr = *state.getObjectsOfClass(CLASS_PLAYER).begin();
	Object& player = *ptr;
	//
	if (action == ACTION_UP) {
		player.getAttribute(ATTR_COUNT)[0]++;
	}
	else if (action == ACTION_DOWN) {
		player.getAttribute(ATTR_COUNT)[0]--;
	}
	return StateDistribution(state);
}

void DomainTest::print(const State& state) const
{
	const Object* const ptr = *state.getObjectsOfClass(CLASS_PLAYER).begin();
	const Object& player = *ptr;

	printf("%c \n P\nCount: %d\n", 219, player.getAttribute(ATTR_COUNT)[0]);
}

State DomainWalls::act_move(const State& current, AttributeValue direction) const
{
	State new_state = current;
	Object* const ptr = *new_state.getObjectsOfClass(CLASS_PLAYER).begin();
	Object& player = *ptr;
	//
	AttributeValue player_pos = player.getAttribute(ATTR_POS);
	if (is_empty(ATTR_POS, player_pos + direction, new_state)) {
		player.setAttribute(ATTR_POS, player_pos + direction);
	}
	//
	return new_state;
}

DomainWalls::DomainWalls(int width, int height) : Environment(width, height)
{
	//ensure the world has a reasonable size
	assert(width >= 5);
	assert(height >= 5);
	//
	types.addStandardActions();
	//
	ATTR_POS = types.addAttributeType("position", 2);
	CLASS_PLAYER = types.addObjectType("player");
	types.getObjectType(CLASS_PLAYER).attribute_types.insert(ATTR_POS);
	CLASS_WALL = types.addObjectType("wall");
	types.getObjectType(CLASS_WALL).attribute_types.insert(ATTR_POS);
}

State DomainWalls::createRandomState(Random& random) const
{
	State state;
	//create border around world
	create_border(CLASS_WALL, ATTR_POS, state);
	//add some random walls
	int walls = WALL_RATIO * ((width - 2) * (height - 2)); //the borders take 2 blocks off each dimension
	for (int i = 0; i < walls; i++) {
		//keep testing random positions until one is found that makes the room still fully connected
		while (true) {
			AttributeValue pos = find_random_empty_position(ATTR_POS, state, random);
			Object& wall = state.add(types.createObject(CLASS_WALL, state.getNextObjectId()));
			wall.setAttribute(ATTR_POS, pos);
			if (is_connected(ATTR_POS, state, AttributeValue::DEFAULT_NEIGHBORS)) {
				break;
			}
			state.remove(wall.getObjectId());
		}
	}
	//put player in random position
	AttributeValue player_pos = find_random_empty_position(ATTR_POS, state, random);
	Object& player = state.add(types.createObject(CLASS_PLAYER, state.getNextObjectId()));
	player.setAttribute(ATTR_POS, player_pos);
	//done
	return state;
}

StateDistribution DomainWalls::act(const State& current, ActionId action, Random& random) const
{
	State next;
	switch (action) {
	case Action::ID_NOOP:
		next = current;
		break;
	case Action::ID_MOVE_LEFT:
		next = act_move(current, AttributeValue::LEFT);
		break;
	case Action::ID_MOVE_RIGHT:
		next = act_move(current, AttributeValue::RIGHT);
		break;
	case Action::ID_MOVE_UP:
		next = act_move(current, AttributeValue::UP);
		break;
	case Action::ID_MOVE_DOWN:
		next = act_move(current, AttributeValue::DOWN);
		break;
	}
	return StateDistribution(next);
}

void DomainWalls::print(const State& state) const
{
	std::map<AttributeValue, const Object*> objects_by_pos;
	//find all objects and put them in a nice table
	for (const auto& pair : state.getObjects()) {
		const Object& obj = pair.second;
		if (!obj.hasAttribute(ATTR_POS)) continue;
		AttributeValue pos = obj.getAttribute(ATTR_POS);
		const Object* ptr = objects_by_pos[pos];
		if (ptr == nullptr || obj.getObjectId() < ptr->getObjectId()) {
			objects_by_pos[pos] = &obj;
		}
	}
	//
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			char c = ' ';
			//
			AttributeValue pos{ x, y };
			auto it = objects_by_pos.find(pos);
			if (it != objects_by_pos.end()) {
				const Object* ptr = it->second;
				if (ptr->getTypeId() == CLASS_PLAYER) {
					c = 'P';
				}
				else if (ptr->getTypeId() == CLASS_WALL) {
					c = 219;
				}
			}
			printf("%c", c);
		}
		printf("\n");
	}
}

DomainFish::DomainFish(int width, int height, int n_fish) : Environment(width, height), n_fish(n_fish)
{
	assert(width >= 5);
	assert(height >= 5);
	//
	ACTION_NOOP = types.newAction("NOOP");
	ACTION_MOVE = types.newAction("MOVE");
	//
	ATTR_POS = types.addAttributeType("position", 2);
	CLASS_FISH = types.addObjectType("fish");
	types.getObjectType(CLASS_FISH).attribute_types.insert(ATTR_POS);
	CLASS_WALL = types.addObjectType("wall");
	types.getObjectType(CLASS_WALL).attribute_types.insert(ATTR_POS);
}

State DomainFish::createRandomState(Random& random) const
{
	State state;
	//create border around world
	create_border(CLASS_WALL, ATTR_POS, state);
	//add some random walls
	int walls = WALL_RATIO * ((width - 2) * (height - 2)); //the borders take 2 blocks off each dimension
	for (int i = 0; i < walls; i++) {
		//keep testing random positions until one is found that makes the room still fully connected
		while (true) {
			AttributeValue pos = find_random_empty_position(ATTR_POS, state, random);
			Object& wall = state.add(types.createObject(CLASS_WALL, state.getNextObjectId()));
			wall.setAttribute(ATTR_POS, pos);
			if (is_connected(ATTR_POS, state, AttributeValue::DEFAULT_NEIGHBORS)) {
				break;
			}
			state.remove(wall.getObjectId());
		}
	}
	//put fish in random position
	for (int i = 0; i < n_fish; i++) {
		AttributeValue fish_pos = find_random_empty_position(ATTR_POS, state, random);
		Object& fish = state.add(types.createObject(CLASS_FISH, state.getNextObjectId()));
		fish.setAttribute(ATTR_POS, fish_pos);
	}
	//done
	return state;
}

StateDistribution DomainFish::act(const State& current, ActionId action, Random& random) const
{
	if (action == ACTION_NOOP) return current;
	if (action == ACTION_MOVE) {
		StateDistribution future(current);
		//find each fish
		for (const auto& pair : current.getObjects()) {
			const Object& o = pair.second;
			if (o.getTypeId() == CLASS_FISH) {
				//produce movement distribution based on walls around it
				ProbabilityDistribution<Object> fishes;
				AttributeValue vals[] = { AttributeValue::UP, AttributeValue::DOWN, AttributeValue::LEFT, AttributeValue::RIGHT };
				for (AttributeValue val : vals) {
					//new mutable copy of object
					Object fish = o;
					//try to move
					AttributeValue new_pos = fish.getAttribute(ATTR_POS) + val;
					bool is_blocked = false;
					for (const auto& pair2 : current.getObjects()) {
						const Object& o2 = pair2.second;
						if (o2.getTypeId() == CLASS_WALL && o2.getAttribute(ATTR_POS) == new_pos) {
							is_blocked = true;
							break;
						}
					}
					if (!is_blocked) {
						fish.setAttribute(ATTR_POS, new_pos);
					}
					//add
					fishes.add(fish);
				}
				fishes.normalize();
				//add to future states
				//this will replace original occurrences of this fish in all possible states
				future.addObject(fishes);
			}
		}
		return future;
	}
	return StateDistribution(State()); //unknown action
}

void DomainFish::print(const State& state) const
{
	std::map<AttributeValue, const Object*> objects_by_pos;
	//find all objects and put them in a nice table
	for (const auto& pair : state.getObjects()) {
		const Object& obj = pair.second;
		if (!obj.hasAttribute(ATTR_POS)) continue;
		AttributeValue pos = obj.getAttribute(ATTR_POS);
		const Object* ptr = objects_by_pos[pos];
		if (ptr == nullptr || obj.getObjectId() < ptr->getObjectId()) {
			objects_by_pos[pos] = &obj;
		}
	}
	//
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			char c = ' ';
			//
			AttributeValue pos{ x, y };
			auto it = objects_by_pos.find(pos);
			if (it != objects_by_pos.end()) {
				const Object* ptr = it->second;
				if (ptr->getTypeId() == CLASS_FISH) {
					c = '>';
				}
				else if (ptr->getTypeId() == CLASS_WALL) {
					c = 219;
				}
			}
			printf("%c", c);
		}
		printf("\n");
	}
}

State DomainWallsDoors::act_move(const State& current, AttributeValue direction) const
{
	State new_state = current;
	Object* const ptr = *new_state.getObjectsOfClass(CLASS_PLAYER).begin();
	Object& player = *ptr;
	//
	AttributeValue player_pos = player.getAttribute(ATTR_POS);
	AttributeValue target_pos = player_pos + direction;
	for (const auto& pair : new_state.getObjects()) {
		const Object& obj = pair.second;
		if (!obj.hasAttribute(ATTR_POS)) continue;
		AttributeValue pos = obj.getAttribute(ATTR_POS);
		if (pos == target_pos) {
			if (obj.getTypeId() == CLASS_WALL) return new_state; //blocked by wall, no movement
			if (obj.getTypeId() == CLASS_DOOR && !(obj.getAttribute(ATTR_COLOR) == player.getAttribute(ATTR_COLOR))) return new_state; //blocked by door, no movement
		}
	}
	//move
	player.setAttribute(ATTR_POS, player_pos + direction);
	//
	return new_state;
}

DomainWallsDoors::DomainWallsDoors(int width, int height, int n_doors, int n_colors) : Environment(width, height), n_doors(n_doors), n_colors(n_colors)
{
	//ensure the world has a reasonable size
	assert(width >= 6);
	assert(height >= 6);
	assert(n_doors >= 0);
	assert(1 <= n_colors && n_colors <= 2);
	//
	types.addStandardActions();
	ACTION_CHANGE_COLOR = types.newAction("CHANGE_COLOR");
	//
	ATTR_POS = types.addAttributeType("position", 2);
	ATTR_COLOR = types.addAttributeType("color", 1);
	CLASS_PLAYER = types.addObjectType("player");
	types.getObjectType(CLASS_PLAYER).attribute_types.insert(ATTR_POS);
	types.getObjectType(CLASS_PLAYER).attribute_types.insert(ATTR_COLOR);
	CLASS_WALL = types.addObjectType("wall");
	types.getObjectType(CLASS_WALL).attribute_types.insert(ATTR_POS);
	CLASS_DOOR = types.addObjectType("door");
	types.getObjectType(CLASS_DOOR).attribute_types.insert(ATTR_POS);
	types.getObjectType(CLASS_DOOR).attribute_types.insert(ATTR_COLOR);
}

State DomainWallsDoors::createRandomState(Random& random) const
{
	State state;
	//create border around world
	create_border(CLASS_WALL, ATTR_POS, state);
	//add some random walls
	int walls = WALL_RATIO * ((width - 2) * (height - 2)); //the borders take 2 blocks off each dimension
	for (int i = 0; i < walls; i++) {
		//keep testing random positions until one is found that makes the room still fully connected
		while (true) {
			AttributeValue pos = find_random_empty_position(ATTR_POS, state, random);
			Object& wall = state.add(types.createObject(CLASS_WALL, state.getNextObjectId()));
			wall.setAttribute(ATTR_POS, pos);
			if (is_connected(ATTR_POS, state, AttributeValue::DEFAULT_NEIGHBORS)) {
				break;
			}
			state.remove(wall.getObjectId());
		}
	}
	//create some doors
	for (int i = 0; i < n_doors; i++) {
		AttributeValue pos = find_random_empty_position(ATTR_POS, state, random);
		AttributeValue color = AttributeValue{ random.random_int(0, n_colors - 1) };
		//make sure there aren't any other-color doors around this one
		bool is_valid_pos = false;
		while (!is_valid_pos) {
			is_valid_pos = true;
			for (const auto& pair : state.getObjects()) {
				const Object& obj = pair.second;
				if (obj.getTypeId() == CLASS_DOOR && !(obj.getAttribute(ATTR_COLOR) == color) && (obj.getAttribute(ATTR_POS) - pos).length() <= 1) {
					is_valid_pos = false;
					break;
				}
			}
			if (!is_valid_pos) {
				pos = find_random_empty_position(ATTR_POS, state, random);
			}
		}
		//
		Object& door = state.add(types.createObject(CLASS_DOOR, state.getNextObjectId()));
		door.setAttribute(ATTR_POS, pos);
		door.setAttribute(ATTR_COLOR, color);
	}
	//put player in random position
	AttributeValue player_pos = find_random_empty_position(ATTR_POS, state, random);
	Object& player = state.add(types.createObject(CLASS_PLAYER, state.getNextObjectId()));
	player.setAttribute(ATTR_POS, player_pos);
	//done
	return state;
}

StateDistribution DomainWallsDoors::act(const State& current, ActionId action, Random& random) const
{
	State next;
	switch (action) {
	case Action::ID_NOOP:
		next = current;
		break;
	case Action::ID_MOVE_LEFT:
		next = act_move(current, AttributeValue::LEFT);
		break;
	case Action::ID_MOVE_RIGHT:
		next = act_move(current, AttributeValue::RIGHT);
		break;
	case Action::ID_MOVE_UP:
		next = act_move(current, AttributeValue::UP);
		break;
	case Action::ID_MOVE_DOWN:
		next = act_move(current, AttributeValue::DOWN);
		break;
	}
	if (action == ACTION_CHANGE_COLOR) {
		const Object* const ptr = *current.getObjectsOfClass(CLASS_PLAYER).begin();
		const Object& player = *ptr;
		AttributeValue player_pos = player.getAttribute(ATTR_POS);
		//change color if there is no door under the player
		for (const auto& pair : current.getObjects()) {
			const Object& obj = pair.second;
			if (!obj.hasAttribute(ATTR_POS)) continue;
			AttributeValue pos = obj.getAttribute(ATTR_POS);
			if (pos == player_pos && obj.getObjectId() != player.getObjectId()) {
				return current;
			}
		}
		//nothing under the player
		State new_state = current;
		Object& p = new_state.getObject(player.getObjectId());
		p.setAttribute(ATTR_COLOR, AttributeValue{1 - p.getAttribute(ATTR_COLOR)[0]});
		next = new_state;
	}
	return StateDistribution(next);
}

void DomainWallsDoors::print(const State& state) const
{
	std::map<AttributeValue, const Object*> objects_by_pos;
	//find all objects and put them in a nice table
	for (const auto& pair : state.getObjects()) {
		const Object& obj = pair.second;
		if (!obj.hasAttribute(ATTR_POS)) continue;
		AttributeValue pos = obj.getAttribute(ATTR_POS);
		const Object* ptr = objects_by_pos[pos];
		if (ptr == nullptr || obj.getObjectId() > ptr->getObjectId()) {
			objects_by_pos[pos] = &obj;
		}
	}
	//
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			char c = ' ';
			//
			AttributeValue pos{ x, y };
			auto it = objects_by_pos.find(pos);
			if (it != objects_by_pos.end()) {
				const Object* ptr = it->second;
				int type = ptr->getTypeId();
				if (type == CLASS_PLAYER) {
					c = ptr->getAttribute(ATTR_COLOR)[0] == 1 ? 'P' : 'p';
				}
				else if (type == CLASS_WALL) {
					c = 219;
				}
				else if (type == CLASS_DOOR) {
					c = ptr->getAttribute(ATTR_COLOR)[0] == 1 ? 'D' : 'd';
				}
			}
			printf("%c", c);
		}
		printf("\n");
	}
}

DomainLights::DomainLights(int min_lights, int max_lights) : Environment(1, 1),
	min_lights(min_lights), max_lights(max_lights)
{
	ACTION_NOOP = types.newAction("NOOP");
	ACTION_INCR = types.newAction("INCR");
	ACTION_DECR = types.newAction("DECR");
	ACTION_SWITCH = types.newAction("SWITCH");
	//
	ATTR_ID = types.addAttributeType("id", 1);
	ATTR_ON = types.addAttributeType("on", 1);
	//
	CLASS_SWITCH = types.addObjectType("switch");
	types.getObjectType(CLASS_SWITCH).attribute_types.insert(ATTR_ID);
	CLASS_LIGHT = types.addObjectType("light");
	types.getObjectType(CLASS_LIGHT).attribute_types.insert(ATTR_ID);
	types.getObjectType(CLASS_LIGHT).attribute_types.insert(ATTR_ON);
}

State DomainLights::createRandomState(Random& random) const
{
	//choose random number of lights
	int lights = min_lights;
	if (max_lights == 0) {
		//geometric distribution with E = 2*min_lights
		while (random.random_uniform() > (1.f / min_lights)) {
			lights++;
		}
	}
	else {
		//uniform distribution
		lights = random.random_int(min_lights, max_lights);
	}
	//
	State state;
	//add switch
	Object& o1 = state.add(types.createObject(CLASS_SWITCH, state.getNextObjectId()));
	o1.setAttribute(ATTR_ID, AttributeValue{ random.random_int(lights) });
	//add lights
	for (int i = 0; i < lights; i++) {
		Object& o2 = state.add(types.createObject(CLASS_LIGHT, state.getNextObjectId()));
		o2.setAttribute(ATTR_ID, AttributeValue{ i });
		o2.setAttribute(ATTR_ON, AttributeValue{ random.random_int(2) }); //each light randomly starts on or off
	}
	//
	return state;
}

StateDistribution DomainLights::act(const State& current, ActionId action, Random& random) const
{
	if (action == ACTION_NOOP) {
		return StateDistribution(current);
	}
	State next = current;
	//find the switch
	Object& the_switch = **(next.getObjectsOfClass(CLASS_SWITCH).begin());
	//
	if (action == ACTION_INCR) {
		the_switch.getAttribute(ATTR_ID)[0]++;
	}
	else if (action == ACTION_DECR) {
		the_switch.getAttribute(ATTR_ID)[0]--;
	}
	else if (action == ACTION_SWITCH) {
		int id = the_switch.getAttribute(ATTR_ID)[0];
		//find all lights with same id and toggle them
		for (auto& pair : next.getObjects()) {
			Object& o = pair.second;
			if (o.getTypeId() == CLASS_LIGHT && o.getAttribute(ATTR_ID)[0] == id) {
				int& val = o.getAttribute(ATTR_ON)[0];
				val = 1 - val;
			}
		}
	}
	return StateDistribution(next);
}

void DomainLights::print(const State& state) const
{
	const Object& the_switch = **(state.getObjectsOfClass(CLASS_SWITCH).begin());
	int id = the_switch.getAttribute(ATTR_ID)[0];
	//
	printf("Switch: %d\n", id);
	//
	printf(" ");
	int max_light = 0;
	std::map<int, bool> lights;
	for (const auto& pair : state.getObjects()) {
		const Object& o = pair.second;
		if (o.getTypeId() == CLASS_LIGHT) {
			int light_id = o.getAttribute(ATTR_ID)[0];
			int light_val = o.getAttribute(ATTR_ON)[0];
			lights[light_id] = light_val;
			max_light = std::max(max_light, light_id);
		}
	}
	for (int i = 0; i <= max_light; i++) {
		printf("%c", lights[i] ? '*' : '_');
	}
	printf("\n");
	//
	for (int i = -1; i < id; i++) {
		printf(" ");
	}
	printf("^\n");
}

State DomainPaths::act_move(const State& current, AttributeValue direction) const
{
	State new_state = current;
	Object* const ptr = *new_state.getObjectsOfClass(CLASS_PLAYER).begin();
	Object& player = *ptr;
	//
	AttributeValue player_pos = player.getAttribute(ATTR_POS);
	AttributeValue dst_pos = player_pos + direction;
	if (!is_empty(ATTR_POS, player_pos + direction, new_state)) {
		player.setAttribute(ATTR_POS, player_pos + direction);
	}
	//
	return new_state;
}

DomainPaths::DomainPaths(int n, int b, int a_) : Environment(1, 1), n(n), b(b), a(a_ > 0 ? std::min(a_, 2 * n) : (2 * n))
{
	int action_count = 0;
	//add move action in each direction
	for (int i = 0; i < n; i++) {
		//
		if (action_count++ < a) types.newAction("MOVE+" + std::to_string(i));
		AttributeValue v(n);
		v.set(i, 1);
		action_directions.push_back(v);
		//
		if (action_count++ < a) types.newAction("MOVE-" + std::to_string(i));
		AttributeValue v2(n);
		v2.set(i, -1);
		action_directions.push_back(v2);
	}
	//
	ATTR_POS = types.addAttributeType("position", n);
	CLASS_PLAYER = types.addObjectType("player");
	types.getObjectType(CLASS_PLAYER).attribute_types.insert(ATTR_POS);
	CLASS_PATH = types.addObjectType("path");
	types.getObjectType(CLASS_PATH).attribute_types.insert(ATTR_POS);
}

State DomainPaths::createRandomState(Random& random) const
{
	State state;

	//add player at (0, 0, ...)
	state.add(types.createObject(CLASS_PLAYER, state.getNextObjectId())).setAttribute(ATTR_POS, AttributeValue(n));

	//add some number of path blocks in random locations
	//like a randomized search of the space
	int n_blocks = b;

	std::set<AttributeValue> visited;
	std::vector<AttributeValue> frontier;
	//
	visited.insert(AttributeValue(n));
	frontier.push_back(AttributeValue(n));
	//
	while (n_blocks > 0 && frontier.size()) {
		//pop random pos from vector
		int index = random.random_int(frontier.size());
		AttributeValue pos = frontier.at(index);
		frontier.erase(frontier.begin() + index);
		//add path
		state.add(types.createObject(CLASS_PATH, state.getNextObjectId())).setAttribute(ATTR_POS, pos);
		//expand frontier
		for (auto& offset : action_directions) {
			AttributeValue pos2 = pos + offset;
			if (visited.insert(pos2).second) {
				frontier.push_back(pos2);
			}
		}
		//
		n_blocks--;
	}

	//
	return state;
}

StateDistribution DomainPaths::act(const State& current, ActionId action, Random& random) const
{
	return act_move(current, action_directions.at(action));
}

void DomainPaths::print(const State& state) const
{
	printf("Paths domain cannot be printed, since it is n-dimensional.\n");
}

DomainEnigma::DomainEnigma() : Environment(1, 1)
{

	ACTION_UP = types.newAction("UP");
	ACTION_DOWN = types.newAction("DOWN");
	ACTION_SIDE = types.newAction("SIDE");
	ACTION_SECRET = types.newAction("SECRET");
	//
	ATTR_X = types.addAttributeType("x", 1);
	ATTR_Y = types.addAttributeType("y", 1);
	ATTR_SECRET = types.addAttributeType("secret", 1);
	//
	CLASS_PLAYER = types.addObjectType("player");
	auto& attribs = types.getObjectType(CLASS_PLAYER).attribute_types;
	attribs.insert(ATTR_X);
	attribs.insert(ATTR_Y);
	attribs.insert(ATTR_SECRET);
}

State DomainEnigma::createRandomState(Random& random) const
{
	State s;
	s.add(types.createObject(CLASS_PLAYER, s.getNextObjectId())).setAttribute(ATTR_SECRET, AttributeValue{random.random_int(2)});
	return s;
}

StateDistribution DomainEnigma::act(const State& current, ActionId action, Random& random) const
{
	State next = current;
	//
	Object& player = **next.getObjectsOfClass(CLASS_PLAYER).begin();
	//
	if (action == ACTION_UP) {
		int& y = player.getAttribute(ATTR_Y).get(0);
		y = y + 1; //y++
	}
	else if (action == ACTION_DOWN) {

		player.getAttribute(ATTR_Y).set(0, 0); //reset y = 0
	}
	else if (action == ACTION_SIDE) {
		int secret = player.getAttribute(ATTR_SECRET).get(0);
		player.getAttribute(ATTR_X).get(0) += (secret ? 1 : -1);
	}
	else if (action == ACTION_SECRET) {
		int& secret = player.getAttribute(ATTR_SECRET).get(0);
		secret = (1 - secret);
	}
	return StateDistribution(next);
}

void DomainEnigma::print(const State& state) const
{
	const Object& player = **state.getObjectsOfClass(CLASS_PLAYER).begin();
	printf("Player: (%d, %d) secret: %d\n", player.getAttribute(ATTR_X).get(0), player.getAttribute(ATTR_Y).get(0), player.getAttribute(ATTR_SECRET).get(0));
}

State DomainEnigma::hideInformation(const State& state) const
{
	State next = state;
	Object& player = **next.getObjectsOfClass(CLASS_PLAYER).begin();
	player.getAttribute(ATTR_SECRET).set(0, 0); //set secret to 0 to hide it
	return next;
}

void DomainComplex::act_switches(State& next) const
{
	if (has_switches) {
		Object* const ptr = *next.getObjectsOfClass(CLASS_PLAYER).begin();
		Object& player = *ptr;
		AttributeValue player_pos = player.getAttribute(ATTR_POS);
		//
		for (auto& pair : next.getObjects()) {
			Object& obj = pair.second;
			if (obj.getTypeId() == CLASS_SWITCH && obj.getAttribute(ATTR_POS) == player_pos) {
				//switch is under player
				int& i = obj.getAttribute(ATTR_ON).get(0);
				i = (1 - i);
				//can't have more than one switch in a single position
				break;
			}
		}
	}
}

State DomainComplex::act_player(const State& current, const AttributeValue& delta) const
{
	State new_state = current;
	//check if switch must be toggled
	act_switches(new_state);
	//
	Object* const ptr = *new_state.getObjectsOfClass(CLASS_PLAYER).begin();
	Object& player = *ptr;
	AttributeValue player_pos = player.getAttribute(ATTR_POS);
	AttributeValue target_pos = player_pos + delta;
	//check if guard is adjacent to player
	//or if wall/gate is blocking player
	for (const auto& pair : new_state.getObjects()) {
		const Object& obj = pair.second;
		//if (obj.getTypeId() == CLASS_GUARD && (obj.getAttribute(ATTR_POS) - player_pos).length() == 1) {
		//	//guard is adjacent to player
		//	return new_state;
		//}
		if (obj.getTypeId() == CLASS_WALL || obj.getTypeId() == CLASS_GATE) {
			const AttributeValue& pos = obj.getAttribute(ATTR_POS);
			if (pos == target_pos) {
				//wall or gate blocking movement
				return new_state;
			}
		}
	}
	//move
	player.setAttribute(ATTR_POS, target_pos);
	//
	return new_state;
}

State DomainComplex::act_guard(const State& current, const AttributeValue& delta) const
{
	State new_state = current;
	//check if switch must be toggled
	act_switches(new_state);
	//
	Object* const ptr = *new_state.getObjectsOfClass(CLASS_GUARD).begin();
	Object& guard = *ptr;
	AttributeValue guard_pos = guard.getAttribute(ATTR_POS);
	AttributeValue target_pos = guard_pos + delta;
	//check if guard is adjacent to player
	//or if wall/gate is blocking player
	for (const auto& pair : new_state.getObjects()) {
		const Object& obj = pair.second;
		if (obj.getTypeId() == CLASS_WALL || obj.getTypeId() == CLASS_PLAYER) {
			const AttributeValue& pos = obj.getAttribute(ATTR_POS);
			if (pos == target_pos) {
				//wall or player blocking movement
				return new_state;
			}
		}
	}
	//move
	guard.setAttribute(ATTR_POS, target_pos);
	//
	return new_state;
}

State DomainComplex::act_jump(const State& current, const AttributeValue& delta) const
{
	State new_state = current;
	//check if switch must be toggled
	act_switches(new_state);
	//
	Object* const ptr = *new_state.getObjectsOfClass(CLASS_PLAYER).begin();
	Object& player = *ptr;
	AttributeValue player_pos = player.getAttribute(ATTR_POS);
	AttributeValue mid_pos = player_pos + delta;
	AttributeValue target_pos = mid_pos + delta;
	//check if wall is blocking player (mid_pos or target_pos)
	//and make sure there is a gate at mid_pos (and not at target_pos)
	bool is_gate_present = false;
	for (const auto& pair : new_state.getObjects()) {
		const Object& obj = pair.second;
		if (!obj.hasAttribute(ATTR_POS)) continue;
		//
		const AttributeValue& pos = obj.getAttribute(ATTR_POS);
		//
		if (obj.getTypeId() == CLASS_WALL) {
			if (pos == mid_pos || pos == target_pos) {
				//wall blocking movement
				return new_state;
			}
		}
		else if (obj.getTypeId() == CLASS_GATE) {
			if (pos == mid_pos) {
				//can jump over gate
				is_gate_present = true;
			}
			else if (pos == target_pos) {
				//gate blocking destination
				return new_state;
			}
		}
	}
	//move
	if(is_gate_present) player.setAttribute(ATTR_POS, target_pos);
	//
	return new_state;
}

DomainComplex::DomainComplex(int width, int height, int mode) :
	Environment(width, height),
	mode(mode), has_gates(mode > 0), has_guard(mode > 1), has_switches(mode > 2), has_jump(mode > 3)
	//bool has_gates, has_guard, has_switches, has_jump;
{
	assert(width >= 8);
	assert(height >= 8);
	assert(mode >= 0);
	assert(mode <= 5);
	//
	if (mode < 5) {
		PLAYER_UP = types.newAction("PLAYER_UP");
		PLAYER_DOWN = types.newAction("PLAYER_DOWN");
		PLAYER_LEFT = types.newAction("PLAYER_LEFT");
		PLAYER_RIGHT = types.newAction("PLAYER_RIGHT");
	}
	if (mode < 5 && has_guard) {
		GUARD_UP = types.newAction("GUARD_UP");
		GUARD_DOWN = types.newAction("GUARD_DOWN");
		GUARD_LEFT = types.newAction("GUARD_LEFT");
		GUARD_RIGHT = types.newAction("GUARD_RIGHT");
	}
	if (has_jump) {
		JUMP_UP = types.newAction("JUMP_UP");
		JUMP_DOWN = types.newAction("JUMP_DOWN");
		JUMP_LEFT = types.newAction("JUMP_LEFT");
		JUMP_RIGHT = types.newAction("JUMP_RIGHT");
	}
	//
	ATTR_POS = types.addAttributeType("position", 2);
	if(has_switches) ATTR_ON = types.addAttributeType("on", 1);
	//
	CLASS_PLAYER = types.addObjectType("player");
	types.getObjectType(CLASS_PLAYER).attribute_types.insert(ATTR_POS);
	CLASS_WALL = types.addObjectType("wall");
	types.getObjectType(CLASS_WALL).attribute_types.insert(ATTR_POS);
	if (has_gates) {
		CLASS_GATE = types.addObjectType("gate");
		types.getObjectType(CLASS_GATE).attribute_types.insert(ATTR_POS);
	}
	if (has_guard) {
		CLASS_GUARD = types.addObjectType("guard");
		types.getObjectType(CLASS_GUARD).attribute_types.insert(ATTR_POS);
	}
	if (has_switches) {
		CLASS_SWITCH = types.addObjectType("switch");
		types.getObjectType(CLASS_SWITCH).attribute_types.insert(ATTR_POS);
		types.getObjectType(CLASS_SWITCH).attribute_types.insert(ATTR_ON);
	}

}

State DomainComplex::createRandomState(Random& random) const
{
	State state;
	//create border around world
	create_border(CLASS_WALL, ATTR_POS, state);
	int a = ((width - 2) * (height - 2)); //grid area
	double n_w = 0.05 + random.random_uniform() * 0.35;
	double n_g = 0.05 + random.random_uniform() * (0.55 - n_w); //maybe lots of gates, maybe few
	//add some random walls
	int walls = n_w * a; //the borders take 2 blocks off each dimension
	for (int i = 0; i < walls; i++) {
		//keep testing random positions until one is found that makes the room still fully connected
		while (true) {
			AttributeValue pos = find_random_empty_position(ATTR_POS, state, random);
			Object& wall = state.add(types.createObject(CLASS_WALL, state.getNextObjectId()));
			wall.setAttribute(ATTR_POS, pos);
			if (is_connected(ATTR_POS, state, AttributeValue::DEFAULT_NEIGHBORS)) {
				break;
			}
			state.remove(wall.getObjectId());
		}
	}
	std::set<AttributeValue> gate_position_set;
	//if there are gates, add some
	if (has_gates) {
		int n_gates = std::max(int(n_g * a), 1);
		for (int i = 0; i < n_gates; i++) {
			AttributeValue pos = find_random_empty_position(ATTR_POS, state, random);
			Object& gate = state.add(types.createObject(CLASS_GATE, state.getNextObjectId()));
			gate.setAttribute(ATTR_POS, pos);
			//
			gate_position_set.insert(pos);
		}
	}
	std::vector<AttributeValue> gate_positions(gate_position_set.begin(), gate_position_set.end());
	//if there is a guard, add it
	if (has_guard && mode < 5) {
		AttributeValue guard_pos = find_random_empty_position(ATTR_POS, state, random);
		Object& guard = state.add(types.createObject(CLASS_GUARD, state.getNextObjectId()));
		guard.setAttribute(ATTR_POS, guard_pos);
	}
	//find random position for player
	//AttributeValue player_pos = find_random_empty_position(ATTR_POS, state, random);
	//next to a gate
	AttributeValue player_pos(2);

	int timeout = 0;
	if (gate_positions.size() > 0) {
		timeout = 30;
		while (timeout-- > 0) {
			AttributeValue gate_position = random.sample(gate_positions);
			player_pos = gate_position + random.sample(AttributeValue::DEFAULT_NEIGHBORS);
			if (is_empty(ATTR_POS, player_pos, state)) {
				break;
			}
		}
	}
	if (timeout <= 0) {
		//failed, give random empty pos
		player_pos = find_random_empty_position(ATTR_POS, state, random);
	}
	//if there are switches, add them
	if (has_switches && mode < 5) {
		int n_switches = random.random_uniform() * 0.2 * a;
		for (int i = 0; i < n_switches; i++) {
			AttributeValue pos = find_random_empty_position(ATTR_POS, state, random);
			Object& switch_ = state.add(types.createObject(CLASS_SWITCH, state.getNextObjectId()));
			switch_.setAttribute(ATTR_POS, pos);
			switch_.setAttribute(ATTR_ON, AttributeValue{ random.random_int(2) });
		}
	}
	//instantiate player
	Object& player = state.add(types.createObject(CLASS_PLAYER, state.getNextObjectId()));
	player.setAttribute(ATTR_POS, player_pos);
	//done
	return state;
}

StateDistribution DomainComplex::act(const State& current, ActionId action, Random& random) const
{
	if (action == PLAYER_UP) {
		return StateDistribution(act_player(current, AttributeValue::UP));
	}
	else if (action == PLAYER_DOWN) {
		return StateDistribution(act_player(current, AttributeValue::DOWN));
	}
	else if (action == PLAYER_LEFT) {
		return StateDistribution(act_player(current, AttributeValue::LEFT));
	}
	else if (action == PLAYER_RIGHT) {
		return StateDistribution(act_player(current, AttributeValue::RIGHT));
	}
	else if (action == GUARD_UP) {
		return StateDistribution(act_guard(current, AttributeValue::UP));
	}
	else if (action == GUARD_DOWN) {
		return StateDistribution(act_guard(current, AttributeValue::DOWN));
	}
	else if (action == GUARD_LEFT) {
		return StateDistribution(act_guard(current, AttributeValue::LEFT));
	}
	else if (action == GUARD_RIGHT) {
		return StateDistribution(act_guard(current, AttributeValue::RIGHT));
	}
	else if (action == JUMP_UP) {
		return StateDistribution(act_jump(current, AttributeValue::UP));
	}
	else if (action == JUMP_DOWN) {
		return StateDistribution(act_jump(current, AttributeValue::DOWN));
	}
	else if (action == JUMP_LEFT) {
		return StateDistribution(act_jump(current, AttributeValue::LEFT));
	}
	else if (action == JUMP_RIGHT) {
		return StateDistribution(act_jump(current, AttributeValue::RIGHT));
	}
	//?
	return StateDistribution(current);
}

void DomainComplex::print(const State& state) const
{
	std::map<AttributeValue, const Object*> objects_by_pos;
	//find all objects and put them in a nice table
	for (const auto& pair : state.getObjects()) {
		const Object& obj = pair.second;
		if (!obj.hasAttribute(ATTR_POS)) continue;
		AttributeValue pos = obj.getAttribute(ATTR_POS);
		const Object* ptr = objects_by_pos[pos];
		if (ptr == nullptr || obj.getObjectId() < ptr->getObjectId()) {
			objects_by_pos[pos] = &obj;
		}
	}
	//
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			char c = ' ';
			//
			AttributeValue pos{ x, y };
			auto it = objects_by_pos.find(pos);
			if (it != objects_by_pos.end()) {
				const Object* ptr = it->second;
				if (ptr->getTypeId() == CLASS_PLAYER) {
					c = 'P';
				}
				else if (ptr->getTypeId() == CLASS_WALL) {
					c = 219;
				}
				else if (ptr->getTypeId() == CLASS_GATE) {
					c = 'X';
				}
				else if (ptr->getTypeId() == CLASS_GUARD) {
					c = 'G';
				}
				else if (ptr->getTypeId() == CLASS_SWITCH) {
					c = ptr->getAttribute(ATTR_ON).get(0) ? '+' : '-';
				}
				else {
					c = '?';
				}
			}
			printf("%c", c);
		}
		printf("\n");
	}
}

State DomainPlayers::act_move(const State& current, int player_class, AttributeValue direction) const
{
	State new_state = current;
	Object* const ptr = *new_state.getObjectsOfClass(player_class).begin();
	Object& player = *ptr;
	//
	AttributeValue player_pos = player.getAttribute(ATTR_POS);
	AttributeValue target_pos = player_pos + direction;
	for (auto& pair : new_state.getObjects()) {
		const Object& o = pair.second;
		if (o.getTypeId() == CLASS_WALL && o.getAttribute(ATTR_POS) == target_pos) {
			return new_state; //can't move
		}
	}
	//move
	player.setAttribute(ATTR_POS, target_pos);
	//
	return new_state;
}

DomainPlayers::DomainPlayers(int width, int height, int n_players) : Environment(width, height), n_players(n_players)
{
	//ensure the world has a reasonable size
	assert(width >= 5);
	assert(height >= 5);
	//ye
	assert(n_players >= 1);
	//prints A-Z
	assert(n_players < 27);
	//
	ATTR_POS = types.addAttributeType("position", 2);
	//
	CLASS_WALL = types.addObjectType("wall");
	types.getObjectType(CLASS_WALL).attribute_types.insert(ATTR_POS);
	//
	std::pair<AttributeValue, std::string> dirs[] = {
		{AttributeValue::UP, "UP"},
		{AttributeValue::DOWN, "DOWN"},
		{AttributeValue::LEFT, "LEFT"},
		{AttributeValue::RIGHT, "RIGHT"}
	};
	//
	for (int i = 0; i < n_players; i++) {
		std::string name = "P" + std::to_string(i + 1);
		int class_player = types.addObjectType(name);
		types.getObjectType(class_player).attribute_types.insert(ATTR_POS);
		//
		player_classes.push_back(class_player);
		class_to_char[class_player] = 'A' + i;
		//
		for (auto& p : dirs) {
			int action_id = types.newAction(name + std::string("_") + p.second);
			action_to_class[action_id] = class_player;
			action_to_effect[action_id] = p.first;
		}
	}
	//
}

State DomainPlayers::createRandomState(Random& random) const
{
	State state;
	//create border around world
	create_border(CLASS_WALL, ATTR_POS, state);
	//add some random walls
	int walls = WALL_RATIO * ((width - 2) * (height - 2)); //the borders take 2 blocks off each dimension
	for (int i = 0; i < walls; i++) {
		//keep testing random positions until one is found that makes the room still fully connected
		while (true) {
			AttributeValue pos = find_random_empty_position(ATTR_POS, state, random);
			Object& wall = state.add(types.createObject(CLASS_WALL, state.getNextObjectId()));
			wall.setAttribute(ATTR_POS, pos);
			if (is_connected(ATTR_POS, state, AttributeValue::DEFAULT_NEIGHBORS)) {
				break;
			}
			state.remove(wall.getObjectId());
		}
	}
	//put players in random positions
	//choose all positions up front so players may end up overlapping
	std::vector<AttributeValue> player_positions;
	for (int i = 0; i < n_players; i++) player_positions.push_back(find_random_empty_position(ATTR_POS, state, random));
	for (int i = 0; i < n_players; i++) {
		Object& player = state.add(types.createObject(player_classes.at(i), state.getNextObjectId()));
		player.setAttribute(ATTR_POS, player_positions.at(i));
	}
	//done
	return state;
}

StateDistribution DomainPlayers::act(const State& current, ActionId action, Random& random) const
{
	return StateDistribution(act_move(current, action_to_class.at(action), action_to_effect.at(action)));
}

void DomainPlayers::print(const State& state) const
{
	std::map<AttributeValue, const Object*> objects_by_pos;
	std::map<AttributeValue, bool> overlaps_by_pos;
	//find all objects and put them in a nice table
	for (const auto& pair : state.getObjects()) {
		const Object& obj = pair.second;
		if (!obj.hasAttribute(ATTR_POS)) continue;
		AttributeValue pos = obj.getAttribute(ATTR_POS);
		const Object* ptr = objects_by_pos[pos];
		if (ptr == nullptr) {
			objects_by_pos[pos] = &obj;
		}
		else {
			overlaps_by_pos[pos] = true;
		}
	}
	//
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			char c = ' ';
			//
			AttributeValue pos{ x, y };
			auto it0 = overlaps_by_pos.find(pos);
			if(it0 != overlaps_by_pos.end()) {
				c = '*';
			}
			else {
				auto it = objects_by_pos.find(pos);
				if (it != objects_by_pos.end()) {
					int id = it->second->getTypeId();
					if (id == CLASS_WALL) {
						c = 219;
					}
					else {
						c = class_to_char.at(id);
					}
				}
			}
			printf("%c", c);
		}
		printf("\n");
	}
}

State DomainMoves::act_move(const State& current, AttributeValue direction) const
{
	State new_state = current;
	Object* const ptr = *new_state.getObjectsOfClass(CLASS_PLAYER).begin();
	Object& player = *ptr;
	//
	AttributeValue player_pos = player.getAttribute(ATTR_POS);
	AttributeValue target_pos = player_pos + direction;
	for (auto& pair : new_state.getObjects()) {
		const Object& o = pair.second;
		if (o.getTypeId() == CLASS_WALL && o.getAttribute(ATTR_POS) == target_pos) {
			return new_state; //can't move
		}
	}
	//move
	player.setAttribute(ATTR_POS, target_pos);
	//
	return new_state;
}

DomainMoves::DomainMoves(int width, int height, int n_copies) : Environment(width, height), n_copies(n_copies)
{
	//ensure the world has a reasonable size
	assert(width >= 5);
	assert(height >= 5);
	//ye
	assert(n_copies >= 1);
	//
	ATTR_POS = types.addAttributeType("position", 2);
	//
	CLASS_PLAYER = types.addObjectType("player");
	types.getObjectType(CLASS_PLAYER).attribute_types.insert(ATTR_POS);
	CLASS_WALL = types.addObjectType("wall");
	types.getObjectType(CLASS_WALL).attribute_types.insert(ATTR_POS);
	//
	std::pair<AttributeValue, std::string> dirs[] = {
		{AttributeValue::UP, "UP"},
		{AttributeValue::DOWN, "DOWN"},
		{AttributeValue::LEFT, "LEFT"},
		{AttributeValue::RIGHT, "RIGHT"}
	};
	//
	for (int i = 0; i < n_copies; i++) {
		for (auto& p : dirs) {
			int action_id = types.newAction(p.second + std::string("_") + std::to_string(i + 1));
			action_to_effect[action_id] = p.first;
		}
	}
	//
}

State DomainMoves::createRandomState(Random& random) const
{
	State state;
	//create border around world
	create_border(CLASS_WALL, ATTR_POS, state);
	//add some random walls
	int walls = WALL_RATIO * ((width - 2) * (height - 2)); //the borders take 2 blocks off each dimension
	for (int i = 0; i < walls; i++) {
		//keep testing random positions until one is found that makes the room still fully connected
		while (true) {
			AttributeValue pos = find_random_empty_position(ATTR_POS, state, random);
			Object& wall = state.add(types.createObject(CLASS_WALL, state.getNextObjectId()));
			wall.setAttribute(ATTR_POS, pos);
			if (is_connected(ATTR_POS, state, AttributeValue::DEFAULT_NEIGHBORS)) {
				break;
			}
			state.remove(wall.getObjectId());
		}
	}
	//put player in random position
	AttributeValue player_pos = find_random_empty_position(ATTR_POS, state, random);
	Object& player = state.add(types.createObject(CLASS_PLAYER, state.getNextObjectId()));
	player.setAttribute(ATTR_POS, player_pos);
	//done
	return state;
}

StateDistribution DomainMoves::act(const State& current, ActionId action, Random& random) const
{
	return StateDistribution(act_move(current, action_to_effect.at(action)));
}

void DomainMoves::print(const State& state) const
{
	std::map<AttributeValue, const Object*> objects_by_pos;
	//find all objects and put them in a nice table
	for (const auto& pair : state.getObjects()) {
		const Object& obj = pair.second;
		if (!obj.hasAttribute(ATTR_POS)) continue;
		AttributeValue pos = obj.getAttribute(ATTR_POS);
		const Object* ptr = objects_by_pos[pos];
		if (ptr == nullptr || obj.getObjectId() < ptr->getObjectId()) {
			objects_by_pos[pos] = &obj;
		}
	}
	//
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			char c = ' ';
			//
			AttributeValue pos{ x, y };
			auto it = objects_by_pos.find(pos);
			if (it != objects_by_pos.end()) {
				const Object* ptr = it->second;
				if (ptr->getTypeId() == CLASS_PLAYER) {
					c = 'P';
				}
				else if (ptr->getTypeId() == CLASS_WALL) {
					c = 219;
				}
			}
			printf("%c", c);
		}
		printf("\n");
	}
}