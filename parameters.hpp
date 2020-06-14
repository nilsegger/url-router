//
// Created by nils on 14/06/2020.
//

#ifndef URL_ROUTER_PARAMETERS_HPP
#define URL_ROUTER_PARAMETERS_HPP


#include <vector>
#include <string>

typedef std::string string_t;

class Parameters {
    std::vector<std::pair<string_t, string_t>> parameters_;

public:

    void add(string_t& name, string_t& value) {
        parameters_.emplace_back(name, value);
    }

    string_t find(string_t name, string_t default_="") {
        for(auto & parameter : parameters_) {
            if(parameter.first == name) return parameter.second;
        }
        return default_;
    }

    void clear() {
        parameters_.clear();
    }
};

#endif //URL_ROUTER_PARAMETERS_HPP
