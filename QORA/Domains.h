#pragma once

#include "Environment.h"

//a very simple domain for testing the CLI
class DomainTest : public Environment {
	int ACTION_UP;
	int ACTION_DOWN;
	//int ATTR_POS;
	int ATTR_COUNT;
	int CLASS_PLAYER;
	int CLASS_WALL;
public:
	DomainTest();
	//create random starting state
	virtual State createRandomState(Random& random) const;
	//bool act(State, Action) -> return empty state if game is over and should restart
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const; //print to stdout
};

//the domain with player+walls for basic experimentation
class DomainWalls : public Environment {
	//int ATTR_POS;
	int CLASS_PLAYER;
	int CLASS_WALL;
	//
	State act_move(const State& current, AttributeValue direction) const;
public:
	DomainWalls(int width, int height);
	//create random starting state
	virtual State createRandomState(Random& random) const;
	//bool act(State, Action) -> return empty state if game is over and should restart
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const; //print to stdout
};

//domain with walls + an npc (the "fish") that moves around randomly
class DomainFish : public Environment {
	int n_fish;
	//
	int ACTION_NOOP;
	int ACTION_MOVE;
	//int ATTR_POS;
	int CLASS_WALL;
	int CLASS_FISH;
public:
	DomainFish(int width, int height, int n_fish = 1);
	//create random starting state
	virtual State createRandomState(Random& random) const;
	//bool act(State, Action) -> return empty state if game is over and should restart
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const; //print to stdout
};

//the domain with player+walls+colored doors
class DomainWallsDoors : public Environment {
	int n_doors; //number of doors to generate
	int n_colors;
	//
	int ACTION_CHANGE_COLOR;
	//int ATTR_POS;
	int ATTR_COLOR;
	int CLASS_PLAYER;
	int CLASS_WALL;
	int CLASS_DOOR;
	//
	State act_move(const State& current, AttributeValue direction) const;
public:
	DomainWallsDoors(int width, int height, int n_doors, int n_colors = 2);
	//create random starting state
	virtual State createRandomState(Random& random) const;
	//bool act(State, Action) -> return empty state if game is over and should restart
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const; //print to stdout
};

//a simple non-gridworld domain for testing causal/relational learning
class DomainLights : public Environment {
	int min_lights;
	int max_lights;
	//
	int ACTION_NOOP;
	int ACTION_INCR;
	int ACTION_DECR;
	int ACTION_SWITCH;
	//int ATTR_POS;
	int ATTR_ID;
	int ATTR_ON;
	int CLASS_SWITCH;
	int CLASS_LIGHT;
public:
	DomainLights(int min_lights, int max_lights = 0); //if max = 0, it'll go up to infinite
	//create random starting state
	virtual State createRandomState(Random& random) const;
	//bool act(State, Action) -> return empty state if game is over and should restart
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const; //print to stdout
};

//n-dimensional walls-like domain (but with path objects instead of walls)
class DomainPaths : public Environment {
	int n; //number of dimensions
	int b; //number of path blocks, default 100
	int a; //number of actions
	//
	std::vector<AttributeValue> action_directions;
	//int ATTR_POS;
	int CLASS_PLAYER;
	int CLASS_PATH;
	//
	State act_move(const State& current, AttributeValue direction) const;
public:
	DomainPaths(int n, int b, int a);
	//create random starting state
	virtual State createRandomState(Random& random) const;
	//bool act(State, Action) -> return empty state if game is over and should restart
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const; //print to stdout
};

//test of a non-linear non-Markovian domain
class DomainEnigma : public Environment {
	//
	int ACTION_UP; //y = y + 1
	int ACTION_DOWN; //y = 0
	int ACTION_SIDE; //move left or right, depending on
	int ACTION_SECRET; //toggle the player's "secret" state
	//
	int ATTR_X;
	int ATTR_Y;
	int ATTR_SECRET;
	//int ATTR_POS;
	int CLASS_PLAYER;
public:
	DomainEnigma();
	//create random starting state
	virtual State createRandomState(Random& random) const;
	//bool act(State, Action) -> return empty state if game is over and should restart
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const; //print to stdout

	virtual State hideInformation(const State& state) const;
};

//test of a set of incrementally-more-complex domains
class DomainComplex : public Environment {
	/*
	mode
	0 = walls domain
	1 = add "gates" that player can't move through
	2 = add "guard" with its own movement actions that can pass through gates
	3 = add switches on the ground that change when the player walks over them
	4 = add 4 additional actions for player that allow it to jump over gates
	5 = disable all actions except jumps
	*/
	int mode = 0;
	bool has_gates = false, has_guard = false, has_switches = false, has_jump = false;
	//
	int PLAYER_UP = -1, PLAYER_DOWN = -1, PLAYER_LEFT = -1, PLAYER_RIGHT = -1;
	int GUARD_UP = -1, GUARD_DOWN = -1, GUARD_LEFT = -1, GUARD_RIGHT = -1;
	int JUMP_UP = -1, JUMP_DOWN = -1, JUMP_LEFT = -1, JUMP_RIGHT = -1;
	//
	int ATTR_ON = -1; //switch, (0) or (1)
	//int ATTR_POS;
	int CLASS_PLAYER = -1;
	int CLASS_GUARD = -1;
	int CLASS_WALL = -1;
	int CLASS_GATE = -1;
	int CLASS_SWITCH = -1;
	//
	void act_switches(State& next) const; //check if a switch needs to be toggled
	State act_player(const State& current, const AttributeValue& delta) const;
	State act_guard(const State& current, const AttributeValue& delta) const;
	State act_jump(const State& current, const AttributeValue& delta) const; //delta is direction, not the 2-vector to destination
public:
	DomainComplex(int width, int height, int mode);
	//create random starting state
	virtual State createRandomState(Random& random) const;
	//bool act(State, Action) -> return empty state if game is over and should restart
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const; //print to stdout
};

//the domain with walls + n players, so # of actions scales
class DomainPlayers : public Environment {
	int n_players;

	//int ATTR_POS;
	int CLASS_WALL;
	std::vector<int> player_classes;
	std::map<int, char> class_to_char;
	std::map<int, int> action_to_class; //[action id] -> class of player it affects
	std::map<int, AttributeValue> action_to_effect; //[action id] -> effect e.g. (0, 1)
	//
	State act_move(const State& current, int player_class, AttributeValue direction) const;
public:
	DomainPlayers(int width, int height, int n_players);
	//create random starting state
	virtual State createRandomState(Random& random) const;
	//bool act(State, Action) -> return empty state if game is over and should restart
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const; //print to stdout
};

//called "actions" in the paper rebuttal, but that's a bit confusing
class DomainMoves : public Environment {
	int n_copies; //how many times to copy each action

	//int ATTR_POS;
	int CLASS_PLAYER;
	int CLASS_WALL;
	std::map<int, AttributeValue> action_to_effect; //[action id] -> effect e.g. (0, 1)
	//
	State act_move(const State& current, AttributeValue direction) const;
public:
	DomainMoves(int width, int height, int n_copies);
	//create random starting state
	virtual State createRandomState(Random& random) const;
	//bool act(State, Action) -> return empty state if game is over and should restart
	virtual StateDistribution act(const State& current, ActionId action, Random& random) const;
	//also some printing/rendering functions so the world can be viewed and interacted with
	virtual void print(const State& state) const; //print to stdout
};