#pragma once

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>

namespace csim {
class KnobBase {
public:
    KnobBase(const std::string& name, const std::string& description)
        : name(name), description(description)
    {
    }

    virtual ~KnobBase() = default;

    auto get_name() const -> const std::string& { return name; }
    auto get_description() const -> const std::string& { return description; }

    virtual auto set_from_string(const std::string& value) -> void = 0;

    virtual auto to_string() const -> std::string = 0;

    virtual auto type_name() const -> std::string = 0;

    virtual auto reset_to_default() -> void = 0;

protected:
    std::string name;
    std::string description;
};

// Template class for specific knob types
template <typename T> class Knob : public KnobBase {
public:
    Knob(const std::string& name, const std::string& description, const T& default_value)
        : KnobBase(name, description), value(default_value), default_value(default_value)
    {
    }

    // Conversion operator for easy access
    operator T() const { return value; }

    // Assignment operator
    auto operator=(const T& val) -> Knob&
    {
        value = val;
        return *this;
    }

    auto get() const -> T { return value; }

    auto set(const T& val) -> void { value = val; }

    auto get_default() const -> T { return default_value; }

    auto reset_to_default() -> void override { value = default_value; }

    auto set_from_string(const std::string& str) -> void override
    {
        std::istringstream iss(str);
        T                  temp;
        if (!(iss >> temp)) {
            throw std::runtime_error("Failed to parse value '" + str + "' for knob '" + name + "'");
        }
        value = temp;
    }

    auto to_string() const -> std::string override
    {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    auto type_name() const -> std::string override
    {
        if (typeid(T) == typeid(int))
            return "int";
        if (typeid(T) == typeid(double))
            return "double";
        if (typeid(T) == typeid(float))
            return "float";
        if (typeid(T) == typeid(bool))
            return "bool";
        if (typeid(T) == typeid(std::string))
            return "string";
        return "unknown";
    }

private:
    T value;
    T default_value;
};

template <>
inline auto
Knob<bool>::set_from_string(const std::string& str) -> void
{
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
        value = true;
    }
    else if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
        value = false;
    }
    else {
        throw std::runtime_error("Failed to parse boolean value '" + str + "' for knob '" + name +
                                 "'");
    }
}

template <>
inline auto
Knob<std::string>::set_from_string(const std::string& str) -> void
{
    value = str;
    // Remove quotes if present
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
}

} // namespace csim