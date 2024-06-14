#include "pch.h"
#include "util.h"

INT64 QPC() {
	LARGE_INTEGER l;
	QueryPerformanceCounter(&l); //will never fail on Windows XP or later
	return l.QuadPart;
}

INT64 QPF() {
	LARGE_INTEGER l;
	QueryPerformanceFrequency(&l); //will never fail on Windows XP or later
	return l.QuadPart;
}

double QPC_TO_MS(INT64 delta)
{
	return (delta * 1000.0) / QPF();
}

double QPC_DELTA_MS(INT64 begin) {
	return QPC_TO_MS(QPC() - begin);
}

double QPC_TO_SEC(INT64 delta)
{
	return ((double)delta) / QPF();
}

double QPC_DELTA_SEC(INT64 begin) {
	return QPC_TO_SEC(QPC() - begin);
}

std::vector<std::string> str_split(const std::string& s, const std::string& splitter)
{
	std::vector<std::string> ss;
	std::size_t pos = 0;
	std::size_t len = splitter.length();
	while (true) {
		std::size_t idx = s.find(splitter, pos);
		if (idx == std::string::npos) {
			//no more matches
			break;
		}
		//match found
		ss.push_back(s.substr(pos, idx - pos));
		pos = idx + len;
	}
	//add the last bit
	ss.push_back(s.substr(pos));
	//
	return ss;
}

std::string str_extract(const std::string& s, const std::string& left, const std::string& right)
{
	std::size_t idx1 = s.find(left);
	if (idx1 == std::string::npos) return "";
	idx1 += left.size();
	std::size_t idx2 = s.find(right, idx1);
	if (idx2 == std::string::npos) return "";
	//
	return s.substr(idx1, idx2 - idx1);
}

std::pair<std::string, std::string> str_args(const std::string& s)
{
	std::string name = s;
	std::string args = "";
	std::size_t argpos = s.find("(");
	if (argpos != std::string::npos) {
		name = s.substr(0, argpos);
		args = str_extract(s, "(", ")");
	}
	return { name, args };
}

////////////////////////////////////////////////////////////////////////////////
//logging
////////////////////////////////////////////////////////////////////////////////

Logger* Logger::instance = nullptr;

bool Logger::init(const std::string& name)
{
	if (instance) return false;
	instance = new Logger(name);
	return instance->open(); //if this fails, we'll just let the program continue normally, I guess...
}

void Logger::quit()
{
	if (instance) delete instance;
	instance = nullptr;
}

//Logger& Logger::getLogger()
//{
//	return *instance;
//}

std::string Logger::formatString(const char* fmt, ...)
{
	int SZ_MAX = 1 << 20; //you really shouldn't exceed this with a single printable string
	int sz = 1024; //initial string size (doubles with every attempt); should be enough for any reasonable message
	while (sz <= SZ_MAX) {
		char* msg = new char[sz];
		va_list args;
		va_start(args, fmt);
		int result = vsprintf_s(msg, sz, fmt, args);
		va_end(args);
		if (result <= 0) {
			//error
			delete[] msg;
			sz *= 2;
			//try again
		}
		else {
			//yay it worked
			return std::string(msg);
		}
	}
	//oh noooo
	return "[[ERROR: EXCEEDED MAX STRING SIZE]]";
}

std::string Logger::timestampString(const char* fmt)
{
	//get time (this seems annoyingly convoluted...)
	time_t rawtime;
	struct tm timeinfo;
	time(&rawtime);
	if (localtime_s(&timeinfo, &rawtime)) {
		return "";
	}
	constexpr int n = 256; //surely this'll be enough, right?...
	char timestamp[n];
	strftime(timestamp, n, fmt, &timeinfo);
	return std::string(timestamp);
}

Logger::Logger(const std::string& name) :
	name(name), file("log_" + name + ".txt"), f(nullptr), indentation(0)
{
}

Logger::~Logger()
{
	if (f) {
		instance_log("LOG END", false);
		fclose(f);
	}
}

bool Logger::open()
{
	if (!fopen_s(&f, file.c_str(), "a")) {
		instance_log("LOG START", false);
		return true;
	}
	else {
		instance_log(formatString("Failed to open log file \"%s\" (error %d)", file.c_str(), errno), true);
		return false;
	}
}

void Logger::instance_log(const char* msg, bool error)
{
	std::string timestamp = timestampString("%T");
	if (error) {
		if (f) fprintf(f, "[%s ERROR] %s\n", timestamp.c_str(), msg);
		fprintf(stderr, "[%s ERROR] %s\n", timestamp.c_str(), msg);
	}
	else {
		std::string indent(indentation, ' ');
		if (f) fprintf(f, "[%s] %s%s\n", timestamp.c_str(), indent.c_str(), msg);
		fprintf(stderr, "[%s] %s%s\n", timestamp.c_str(), indent.c_str(), msg);
	}
}

void Logger::instance_log(const std::string& msg, bool error)
{
	instance_log(msg.c_str(), error);
}

int Logger::instance_indent_push()
{
	return indentation++;
}

void Logger::instance_indent_pop(int to)
{
	if (to < 0) {
		if (indentation > 0) indentation--;
	}
	else {
		indentation = to;
	}
}

void Logger::log(const char* msg, bool error)
{
	instance->instance_log(msg, error);
}

void Logger::log(const std::string& msg, bool error)
{
	log(msg.c_str(), error);
}

int Logger::indent_push()
{
	return instance->instance_indent_push();
}

void Logger::indent_pop(int to)
{
	return instance->instance_indent_pop(to);
}

////////////////////////////////////////////////////////////////////////////////
//progress
////////////////////////////////////////////////////////////////////////////////

const double Progress::MIN_INTERVAL = 2.0;

Progress::Progress(const std::string& name, int total) :
	name(name), total(total), begin(QPC()),
	lastCheckpoint(0), lastCheckpointData(0), currentCheckpointLength(MIN_INTERVAL) //initial: 1 sec per interval
{
}

void Progress::reset(int total)
{
	this->total = total;
	begin = QPC();
	lastCheckpoint = 0;
	lastCheckpointData = 0;
	currentCheckpointLength = MIN_INTERVAL;
}

void Progress::update(int data)
{
	double time = QPC_DELTA_SEC(begin);
	if (time >= lastCheckpoint + currentCheckpointLength) {
		double percent_complete = (100.0 * data) / total;
		double rate = data / time;
		double time_total = total / rate; //estimated total time
		//calc next checkpoint (so we don't print too much)
		double time_per_percent = (total / 100) / rate;
		currentCheckpointLength = std::min(currentCheckpointLength * 2, time_per_percent); //gradually approach a longer interval
		currentCheckpointLength = std::max(currentCheckpointLength, MIN_INTERVAL); //but never go below n sec
		//
		Logger::log(Logger::formatString(
			"[%s: %.1f%%, %.2f/%.2f sec (+ %.2f)] next checkpoint in %.2f seconds",
			name.c_str(), percent_complete, time, time_total, time - lastCheckpoint, currentCheckpointLength
		));
		lastCheckpoint = time;
	}
}

void Progress::end()
{
	Logger::log(Logger::formatString("[%s: done in %.2f sec]", name.c_str(), QPC_DELTA_SEC(begin)));
}

////////////////////////////////////////////////////////////////////////////////
//IntTensor
////////////////////////////////////////////////////////////////////////////////

IntTensor::IntTensor(int width, int height, int depth) :
	w(width), h(height), d(depth),
	values(width * height * depth, 0)
{
}

int IntTensor::getWidth() const
{
	return w;
}

int IntTensor::getHeight() const
{
	return h;
}

int IntTensor::getDepth() const
{
	return d;
}

std::vector<int>& IntTensor::getValues()
{
	return values;
}

const std::vector<int>& IntTensor::getValues() const
{
	return values;
}

int& IntTensor::at(int x, int y, int z)
{
	return values.at(x * (h * d) + y * d + z);
}

int IntTensor::at(int x, int y, int z) const
{
	return values.at(x * (h * d) + y * d + z);
}

int* IntTensor::block(int x, int y)
{
	return values.data() + (x * (h * d) + y * d);
}
