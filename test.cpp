
#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <iostream>

int main() {

    std::vector<int> nums = {1, 2, 3};

    for (auto it = nums.rbegin(); it != nums.rend(); ++it) {
        std::cout << *it << std::endl;
    }


    return 0;
}