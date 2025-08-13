#pragma once
#include <string>
#include <vector>
namespace mp {
struct Item{ std::string name; double value; std::string unit; };
std::string toJson(const std::vector<Item>& items);
}
