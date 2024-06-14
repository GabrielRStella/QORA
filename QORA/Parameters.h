#pragma once

/*
basic cmdline parameter/argument helper with support for:
-string parameters
-enum parameters (string)
-int parameters (+ranged)
-float params (+ranged)
*/

enum class ParameterType {
    ENUM,
    STRING,
    INT,
    FLOAT
};

struct Parameter {
    std::string name = "?";
    ParameterType type = ParameterType::STRING;
    bool has_default = false;
    std::string default_value = "";
    //enum data
    std::vector<std::string> enum_values;
    //string data
    
    //int data
    int int_min = 0;
    bool has_int_min = false;
    int int_max = 0;
    bool has_int_max = false;
    //float data
    double float_min = 0.0;
    bool has_float_min = false;
    double float_max = 0.0;
    bool has_float_max = false;
    //
    static constexpr int NO_MIN_INT = std::numeric_limits<int>::min();
    static constexpr int NO_MAX_INT = std::numeric_limits<int>::max();
    static constexpr double NO_MIN_FLOAT = -std::numeric_limits<double>::infinity();
    static constexpr double NO_MAX_FLOAT = std::numeric_limits<double>::infinity();
    //
    static Parameter createEnumParam(const std::string& name, const std::initializer_list<std::string>& options, int default_index = -1);
    static Parameter createEnumParam(const std::string& name, const std::vector<std::string>& options, int default_index = -1);
    static Parameter createBoolParam(const std::string& name); //enum{true,false}
    static Parameter createBoolParam(const std::string& name, bool default_value); //enum{true,false}
    static Parameter createStringParam(const std::string& name);
    static Parameter createStringParam(const std::string& name, const std::string& default_value);
    static Parameter createIntParam(const std::string& name, int min_val = NO_MIN_INT, int max_val = NO_MAX_INT);
    static Parameter createIntParamDefault(const std::string& name, int default_value, int min_val = NO_MIN_INT, int max_val = NO_MAX_INT);
    static Parameter createFloatParam(const std::string& name, double min_val = NO_MIN_FLOAT, double max_val = NO_MAX_FLOAT);
    static Parameter createFloatParamDefault(const std::string& name, double default_value, double min_val = NO_MIN_FLOAT, double max_val = NO_MAX_FLOAT);
    //
    bool accept(const std::string& s) const;
    std::string to_string() const; //human-readable info about this parameter's arguments
};

class Parameters
{
    std::vector<Parameter> parameters;
    std::map<std::string, int> parameter_indices; //for lookup when parsing
    //
public:
    Parameters();
    Parameters& addParameter(const Parameter& parameter);
    int size() const;
    Parameter& getParameter(int index);
    const Parameter& getParameter(int index) const;
    bool parse(const std::string& args, std::map<std::string, std::string>& values); //parses comma-separated param:value pairs, stores them in a map, and returns true if parsing encountered no errors
};

