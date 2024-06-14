#include "pch.h"
#include "Parameters.h"

#include "util.h"

Parameter Parameter::createEnumParam(const std::string& name, const std::initializer_list<std::string>& options, int default_index)
{
    Parameter p;
    p.name = name;
    p.type = ParameterType::ENUM;
    //
    p.enum_values.insert(p.enum_values.begin(), options.begin(), options.end());
    if (default_index >= 0) {
        p.has_default = true;
        p.default_value = p.enum_values[default_index];
    }
    //
    return p;
}

Parameter Parameter::createEnumParam(const std::string& name, const std::vector<std::string>& options, int default_index)
{
    Parameter p;
    p.name = name;
    p.type = ParameterType::ENUM;
    //
    p.enum_values.insert(p.enum_values.begin(), options.begin(), options.end());
    if (default_index >= 0) {
        p.has_default = true;
        p.default_value = p.enum_values[default_index];
    }
    //
    return p;
}

Parameter Parameter::createBoolParam(const std::string& name)
{
    return createEnumParam(name, {"true","false"});
}

Parameter Parameter::createBoolParam(const std::string& name, bool default_value)
{
    return createEnumParam(name, { "true","false" }, 1 - default_value);
}

Parameter Parameter::createStringParam(const std::string& name)
{
    Parameter p;
    p.name = name;
    p.type = ParameterType::STRING;
    return p;
}

Parameter Parameter::createStringParam(const std::string& name, const std::string& default_value)
{
    Parameter p;
    p.name = name;
    p.type = ParameterType::STRING;
    p.has_default = true;
    p.default_value = default_value;
    return p;
}

Parameter Parameter::createIntParam(const std::string& name, int min_val, int max_val)
{
    Parameter p;
    p.name = name;
    p.type = ParameterType::INT;
    if (min_val != NO_MIN_INT) {
        p.has_int_min = true;
        p.int_min = min_val;
    }
    if (max_val != NO_MAX_INT) {
        p.has_int_max = true;
        p.int_max = max_val;
    }
    return p;
}

Parameter Parameter::createIntParamDefault(const std::string& name, int default_value, int min_val, int max_val)
{
    Parameter p;
    p.name = name;
    p.type = ParameterType::INT;
    p.has_default = true;
    p.default_value = std::to_string(default_value);
    if (min_val != NO_MIN_INT) {
        p.has_int_min = true;
        p.int_min = min_val;
    }
    if (max_val != NO_MAX_INT) {
        p.has_int_max = true;
        p.int_max = max_val;
    }
    return p;
}

Parameter Parameter::createFloatParam(const std::string& name, double min_val, double max_val)
{
    Parameter p;
    p.name = name;
    p.type = ParameterType::FLOAT;
    if (min_val != NO_MIN_FLOAT) {
        p.has_float_min = true;
        p.float_min = min_val;
    }
    if (max_val != NO_MAX_FLOAT) {
        p.has_float_max = true;
        p.float_max = max_val;
    }
    return p;
}

Parameter Parameter::createFloatParamDefault(const std::string& name, double default_value, double min_val, double max_val)
{
    Parameter p;
    p.name = name;
    p.type = ParameterType::FLOAT;
    p.has_default = true;
    p.default_value = std::to_string(default_value);
    if (min_val != NO_MIN_FLOAT) {
        p.has_float_min = true;
        p.float_min = min_val;
    }
    if (max_val != NO_MAX_FLOAT) {
        p.has_float_max = true;
        p.float_max = max_val;
    }
    return p;
}

bool Parameter::accept(const std::string& s) const
{
    switch (type) {
    case ParameterType::ENUM:
        //accept any string in the enum list
        return std::find(enum_values.begin(), enum_values.end(), s) != enum_values.end();
    case ParameterType::STRING:
        //accept any string
        return true;
    case ParameterType::INT:
        {
        //convert to int
        int val = std::stoi(s);
        //range checks
        if (has_int_min && val < int_min) return false;
        if (has_int_max && val > int_max) return false;
        //
        return true;
        }
    case ParameterType::FLOAT:
        {
        //convert to double
        double val = std::stod(s);
        //range checks
        if (has_float_min && val < float_min) return false;
        if (has_float_max && val > float_max) return false;
        //
        return true;
        }
    }
    return false;
}

std::string Parameter::to_string() const
{
    std::ostringstream oss;
    //name:stuff
    //name(default):stuff
    oss << name;
    if (has_default) {
        oss << '(' << default_value << ')';
    }
    oss << ':';
    //
    switch (type) {
    case ParameterType::ENUM:
        //name:enum{val1,val2,...}
        oss << "enum{";
        for (std::size_t i = 0; i < enum_values.size(); i++) {
            oss << enum_values[i];
            if (i < enum_values.size() - 1) oss << ',';
        }
        oss << "}";
        break;
    case ParameterType::STRING:
        //name:string
        oss << "string";
        break;
    case ParameterType::INT:
        //name:int
        //name:int{min..}
        //name:int{..max}
        //name:int{min..max}
        oss << "int";
        if (has_int_min || has_int_max) {
            oss << '{';
            if (has_int_min) oss << int_min;
            oss << "..";
            if (has_int_max) oss << int_max;
            oss << '}';
        }
        break;
    case ParameterType::FLOAT:
        //name:float
        //name:float[min,)
        //name:float(,max]
        //name:float[min,max]
        oss << "float";
        if (has_float_min || has_float_max) {
            if (has_float_min) oss << '[' << float_min;
            else oss << '(';
            oss << ",";
            if (has_float_max) oss << float_max << ']';
            else oss << ')';
        }
        break;
    }
    return oss.str();
}

Parameters::Parameters()
{
}

Parameters& Parameters::addParameter(const Parameter& parameter)
{
    parameter_indices[parameter.name] = parameters.size();
    parameters.push_back(parameter);
    //for stacking
    return *this;
}

int Parameters::size() const
{
    return parameters.size();
}

Parameter& Parameters::getParameter(int index)
{
    return parameters.at(index);
}

const Parameter& Parameters::getParameter(int index) const
{
    return parameters.at(index);
}

bool Parameters::parse(const std::string& args, std::map<std::string, std::string>& values)
{
    //pre-populate with any default values
    for (const Parameter& param : parameters) {
        if (param.has_default) values[param.name] = param.default_value;
    }
    //parse given args
    if(args.size() > 0) {
        std::vector<std::string> argpairs = str_split(args, ",");
        for (const std::string& argpair : argpairs) {
            std::vector<std::string> namevalue = str_split(argpair, ":");
            if (namevalue.size() != 2) {
                //??? error - no value
                Logger::log(Logger::formatString("Parameter parse error: no value separator found in \"%s\"\n", argpair.c_str()), true);
                return false;
            }
            const std::string& name = namevalue[0];
            const std::string& value = namevalue[1];
            auto it = parameter_indices.find(name);
            if (it == parameter_indices.end()) {
                //??? error - invalid parameter
                Logger::log(Logger::formatString("Parameter parse error: invalid parameter name \"%s\"\n", name.c_str()), true);
                return false;
            }
            Parameter& param = parameters[it->second];
            if (!param.accept(value)) {
                //??? error - invalid value
                Logger::log(Logger::formatString("Parameter parse error: invalid parameter value \"%s\" for parameter \"%s\"\n", value.c_str(), name.c_str()), true);
                return false;
            }
            //success!
            values[name] = value;
        }
    }
    //make sure all non-default params have values
    for (const Parameter& param : parameters) {
        if (values.find(param.name) == values.end()) {
            //??? error - no value given
            Logger::log(Logger::formatString("Parameter parse error: no value given for parameter \"%s\"\n", param.name.c_str()), true);
            return false;
        }
    }
    //
    return true;
}
