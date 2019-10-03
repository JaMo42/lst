#pragma once

#include <Windows.h>
#include <tchar.h>

#include <string>
#include <vector>
#include <queue>

#include <iostream>
#include <algorithm>

// @TODO Remove
// For debugging purposes
#include <bitset>
#include <iostream>
template<typename T, const std::size_t N = sizeof(T) * 8>
constexpr void PrintBits(const T &Value) {
	std::cout << std::bitset<N>(Value).to_string() << std::endl;
}
