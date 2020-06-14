//
// Created by nils on 28/05/2020.
//

#ifndef URL_ROUTER_PATH_MATCHER_HPP
#define URL_ROUTER_PATH_MATCHER_HPP

#include <vector>
#include <string>
#include <optional>
#include <cassert>
#include <iostream>

#include "parameters.hpp"

typedef std::string string_t;

template<typename Result>
class PathMatcher {

    struct Path {
        char character;
        std::vector<Path *> children;
        std::optional<Result> result;

        Path(char character, std::optional<Result> result) : character(character), result(result) {}

        ~Path() {
            for(size_t i = 0; i < children.size(); i++) {
                delete children[i];
            }
        }
    };

    Path *
    find_child(char character, std::vector<Path *> &parent, bool findWildcard = false, Path **wildcard = nullptr) {
        Path *child = nullptr;
        for (size_t i = 0; i < parent.size(); i++) {
            if (parent[i]->character == character) {
                child = parent[i];
                // If wildcards are not wanted, there is no need to continue
                if (!findWildcard) break;
            } else if (findWildcard && parent[i]->character == wildcard_open_) {
                (*wildcard) = parent[i];
                // If child has already been found, there is no need to continue
                if (child != nullptr) break;
            }
        }
        return child;
    }

    Path *create_path(std::vector<Path *> &parent, char character, std::optional<Result> value) {
        Path *route = new Path(character, value);
        parent.push_back(route);
        return route;
    }

    void insert_path(string_t &path, Result &result, Path *step = nullptr, int path_pos = 0) {
        /*
         * Recursively creates path. A path contains a vector with child paths.
         * These linked paths create a chain which can be walked down.
         * If we input /users/a and /users/b the following chain would be created.
         * / -> u -> s -> e -> r -> s -> / -> a
         *                                 -> b
         *
         * Now if you want to match against an input, this chain can be walked down until a path no longer contains the matching character.
         * If the input would equal /users/c, the chain would be walked until the last '/' and then failing because it only contains the children a & b, not c.
         *
         * The last two paths (a & b) will contains a value. This value states that the path has reached its end.
         */
        assert(path.size() > 1);
        if (path_pos == path.size() - 1) {
            // last insertion accompanied by ending value
            Path *child = find_child(path[path_pos], step->children);

            if (child != nullptr) {
                assert(!child->result); // Cant already have a value
                child->result = result;
            } else {
                create_path(step->children, path[path_pos], result);
            }
        } else {

            Path *child;
            if (path_pos == 0) {
                child = find_child(path[path_pos], paths_);
            } else {
                child = find_child(path[path_pos], step->children);
            }

            if (child == nullptr && path_pos == 0) {
                child = create_path(paths_, path[path_pos], std::nullopt);
            } else if (child == nullptr) {
                child = create_path(step->children, path[path_pos], std::nullopt);
            }

            return insert_path(path, result, child, path_pos + 1);
        }
    }

    void get_wildcard_name(Path **wildcard, string_t &wildcard_name) {
        /*
         * /users/{uuid} and users/{uuid}/friends is allowed
         * /users/{uuid} and users/{id}/friends is not allowed, because wildcards at the same positions must match
         *
         * This method walks down the chain until the wildcard_close_ character has been found. Everything between start and end is appended to the value.
         */
        assert((*wildcard)->children.size() == 1);
        if ((*wildcard)->children[0]->character != wildcard_close_) {
            wildcard_name.append(1, (*wildcard)->children[0]->character);
            *wildcard = (*wildcard)->children[0];
            get_wildcard_name(wildcard, wildcard_name);
        } else {
            *wildcard = (*wildcard)->children[0];
        }
    }

    string_t get_wildcard_value(string_t &path, size_t &pos) {
        // Walks down the input until the trailing_wildcard_ is found or the end is reached, everything between equals the wildcard value
        int begin = pos;
        for (; pos < path.size() - 1; pos++) {
            if (path[pos + 1] == trailing_wildcard_) {
                return path.substr(begin, pos - begin + 1);
            }
        }
        return path.substr(begin);
    }

    std::vector<Path *> paths_;
    Result default_;
    char wildcard_open_;
    char wildcard_close_;
    char trailing_wildcard_;

public:

    PathMatcher(Result default_, char wildcard_open='{', char wildcard_close='}', char trailing_wildcard='/')
        : default_(default_), wildcard_open_(wildcard_open), wildcard_close_(wildcard_close), trailing_wildcard_(
                                                                                                            trailing_wildcard) {}


    virtual ~PathMatcher() {
        for(size_t i = 0; i < paths_.size(); i++) {
            delete paths_[i];
        }
    }

    void add_path(string_t path, Result value) {
        insert_path(path, value);
    }

    Result match(string_t path, Parameters &variables) {
        /*
         * Starts at paths_ and continues trying to find children matching the next characters in input path.
         * If there is no child which matches the next character, but there was a wildcard_open_ as a child,
         * the code jumps back to it and sets a Parameters value for the wildcard with its value and then continues normally.
         */
        Path *step = find_child(path[0], paths_);
        if (step == nullptr) return default_;

        Path *lastWildcard = nullptr;
        size_t lastWildcardPos;
        size_t i = 1;
        for (; i < path.size() - 1 && step != nullptr; i++) {

            Path *nextWildcard = nullptr;
            step = find_child(path[i], step->children, true, &nextWildcard);

            if (nextWildcard != nullptr && nextWildcard != lastWildcard) {
                lastWildcardPos = i;
                lastWildcard = nextWildcard;
            }
            if (path[i] == trailing_wildcard_) {
                lastWildcard = nullptr;
            }

            if (step == nullptr && lastWildcard != nullptr) {
                i = lastWildcardPos;

                string_t wildcard_name;
                get_wildcard_name(&lastWildcard, wildcard_name);
                string_t wildcard_value = get_wildcard_value(path, i);
                variables.add(wildcard_name, wildcard_value);

                if (i == path.size() - 1) {
                    // Wildcard value reaches end
                    if (!lastWildcard->result) return default_;
                    return lastWildcard->result.value();
                } else {
                    step = lastWildcard;
                }
            }
        }

        if (step == nullptr) return default_;

        Path *wildcard = nullptr;
        Path *result = find_child(path[path.size() - 1], step->children, true, &wildcard);

        if(result != nullptr && result->result) return result->result.value();
        else if(wildcard != nullptr) {
            // find wildcard ending and check if it contains a value
            string_t wildcardName;
            get_wildcard_name(&wildcard, wildcardName);

            if(!wildcard->result) return default_;

            string_t value = path.substr(path.size() - 1);
            variables.add(wildcardName, value);
            return wildcard->result.value();
        }

        return default_;
    }
};

#endif //
