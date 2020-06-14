#include <cassert>

#include "path_matcher.hpp"
#include "parameters.hpp"

int main() {

    PathMatcher<int> matcher(-1);
    matcher.add_path("/users/{uuid}", 1);
    matcher.add_path("/users/{uuid}/friends/{friend_uuid}", 2);
    matcher.add_path("/posts", 3);
    matcher.add_path("/posts/{id}", 4);
    matcher.add_path("/posts/{id}/pictures", 5);

    Parameters parameters;
    assert(matcher.match("/users/9c4ceec8-f929-434e-8ff1-837dd54b7b56", parameters) == 1);
    assert(parameters.find("uuid", "") == "9c4ceec8-f929-434e-8ff1-837dd54b7b56");
    parameters.clear();

    assert(matcher.match("/users/9c4ceec8-f929-434e-8ff1-837dd54b7b56/friends/11dd8f68-ae6a-11ea-b3de-0242ac130004", parameters) == 2);
    assert(parameters.find("uuid", "") == "9c4ceec8-f929-434e-8ff1-837dd54b7b56");
    assert(parameters.find("friend_uuid", "") == "11dd8f68-ae6a-11ea-b3de-0242ac130004");
    parameters.clear();

    assert(matcher.match("/posts", parameters) == 3);
    assert(matcher.match("/posts/1", parameters) == 4);
    assert(parameters.find("id") == "1");
    parameters.clear();

    assert(matcher.match("/posts/5/pictures", parameters) == 5);
    assert(parameters.find("id") == "5");
}
