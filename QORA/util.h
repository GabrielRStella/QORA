#pragma once

//timing
INT64 QPC();
INT64 QPF();
double QPC_TO_MS(INT64 delta);
double QPC_DELTA_MS(INT64 begin);
double QPC_TO_SEC(INT64 delta);
double QPC_DELTA_SEC(INT64 begin);

//string manipulation
std::vector<std::string> str_split(const std::string& s, const std::string& splitter); //split a string on exact matches of the splitter string
std::string str_extract(const std::string& s, const std::string& left, const std::string& right); //if the left and right substrings are present in the source string (in correct order), return the text between them
std::pair<std::string, std::string> str_args(const std::string& s); //parses strings of the form "name" to {"name", ""} or "name(args)" to {"name", "args"}

//logging, adapted from old project

class Logger
{
public:
	//
	static bool init(const std::string& name);
	static void quit();
	//static Logger& getLogger(); //get instance
	//utility func (auto-resizes buffer and returns as cppstring)
	static std::string formatString(const char* fmt, ...);
	static std::string timestampString(const char* fmt = "%c");
private:
	static Logger* instance;
	Logger(const std::string& name);
	~Logger();
	bool open();
	//
	std::string name; //<program name>
	std::string file; //"log_<name>.txt"
	FILE* f; //opened for append
	//
	int indentation;
public:
	void instance_log(const char* msg, bool error = false);
	void instance_log(const std::string& msg, bool error = false);
	int instance_indent_push(); //return the prev indentation level
	void instance_indent_pop(int to = -1); //pop once (if -1) or to a specific level (if >= 0)
	//basic logging
	static void log(const char* msg, bool error = false);
	static void log(const std::string& msg, bool error = false);
	//indentation stack (for info logging)
	static int indent_push(); //return the prev indentation level
	static void indent_pop(int to = -1); //pop once (if -1) or to a specific level (if >= 0)
};

class Progress
{
	static const double MIN_INTERVAL;

	std::string name; 
	//total data
	int total;
	//creation time
	INT64 begin;
	//
	double lastCheckpoint;
	UINT64 lastCheckpointData;
	double currentCheckpointLength;

public:
	Progress(const std::string& name, int total);
	void reset(int total);
	void update(int data);
	void end();
};

//3d stack of integers, for convenience
//stores them linearly, in the same format as numpy:
//so for [x][y][z], z values are stored contiguously, then z groups are stored contiguously in y, then x
class IntTensor {
	//stored in the same order as torch/numpy so they can be reshaped directly into the python program
	int w, h, d;
	std::vector<int> values;
public:
	IntTensor(int width, int height, int depth);
	//
	int getWidth() const;
	int getHeight() const;
	int getDepth() const;
	//
	std::vector<int>& getValues();
	const std::vector<int>& getValues() const;
	//
	int& at(int x, int y, int z);
	int at(int x, int y, int z) const;
	//get a pointer to a single z block, starting at z=0
	int* block(int x, int y);
};