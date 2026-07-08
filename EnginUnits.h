#ifndef EnginUnits_H
#define EnginUnits_H

#include <string>
#include <iostream>
#include <unordered_map>

#include "Utils.h"

inline constexpr double k_B = 1.380649e-23;
inline constexpr double q_charge = 1.60217663e-19;

inline std::unordered_map<char,double> engin_unit_table {
    {'m', 1e-3},
    {'u', 1e-6},
    {'n', 1e-9},
    {'p', 1e-12},
    {'f', 1e-15},
    {'k', 1e3},
    {'M', 1e6},
    {'G', 1e9},
    {'T', 1e12},
    {'P', 1e15}
};

inline double EnginUnits(std::string to_convert, bool silent = false) {
    size_t processed_char = 0;
    double value = 0.0;
    try {
        value = std::stod(to_convert,&processed_char);
    } catch(const std::exception& error) {
        if(!silent) {
            std::cout << "ERROR: " << error.what() << std::endl;
            std::cout << "Because of the encontered error, " << value << " was returned instead." << std::endl;
        }
        return value;
    }
    if(processed_char == to_convert.length()) return value;
    auto lookup_engin_unit = engin_unit_table.find(to_convert[processed_char]);
    if(lookup_engin_unit == engin_unit_table.end()) {
        if(!silent) std::cout << "ERROR: invalid enigneering unit suffix used, " << value << " was returned instead." << std::endl;
        return value;
    }
    double unit = lookup_engin_unit->second;
    return value*unit;
}

#endif