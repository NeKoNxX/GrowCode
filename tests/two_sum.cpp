#include <iostream>
#include <vector>
#include <unordered_map>
using namespace std;

int main() {
    int n;
    cin >> n;

    vector<int> nums(n);
    for (int i = 0; i < n; i++) {
        cin >> nums[i];
    }

    int target;
    cin >> target;

    unordered_map<int, int> mp;

    for (int i = 0; i < n; i++) {
        int complement = target - nums[i];

        auto it = mp.find(complement);
        if (it != mp.end()) {
            cout << it->second << " " << i << '\n';
            return 0;
        }

        mp[nums[i]] = i;
    }

    return 0;
}