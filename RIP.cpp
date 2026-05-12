#include <iostream>
#include <map>
#include <string>
#include <algorithm>
using namespace std;

struct RouteEntry {
    int hops;           // 跳数
    string nextHop;     // 下一跳
    bool fromR2;        // 是否来自R2（用于判断是否直接来自R2）
};

int main() {
    map<string, RouteEntry> R1_table;  // R1的路由表
    map<string, RouteEntry> R2_advert;  // R2通告的路由表
    
    int n;
    cout << "输入R1的路由表项数: ";
    cin >> n;
    
    cout << "输入R1的路由表（目的网络 跳数 下一跳）:" << endl;
    for (int i = 0; i < n; i++) {
        string dest, nextHop;
        int hops;
        cin >> dest >> hops >> nextHop;
        R1_table[dest] = {hops, nextHop, false};
    }
    
    int m;
    cout << "输入R2发来的路由表项数: ";
    cin >> m;
    
    cout << "输入R2的路由表（目的网络 跳数）:" << endl;
    for (int i = 0; i < m; i++) {
        string dest;
        int hops;
        cin >> dest >> hops;
        R2_advert[dest] = {hops, "R2", true};  // 下一跳是R2
    }
    
    // RIP更新算法
    const int INF = 16;  // RIP中16跳表示不可达
    
    // 1. 处理R2通告的路由
    for (auto &entry : R2_advert) {
        string dest = entry.first;
        int hops_from_R2 = entry.second.hops;
        int new_hops = hops_from_R2 + 1;  // 经过R2需要+1跳
        
        if (new_hops > INF) new_hops = INF;  // 超过16跳设为不可达
        
        if (R1_table.find(dest) == R1_table.end()) {
            // R1中没有该路由，直接添加
            R1_table[dest] = {new_hops, "R2", true};
        } else {
            // R1中已有该路由
            RouteEntry &existing = R1_table[dest];
            
            if (existing.nextHop == "R2") {
                // 下一跳是R2，无论跳数变大变小都更新
                existing.hops = new_hops;
            } else {
                // 下一跳不是R2，只有新路径更优时才更新
                if (new_hops < existing.hops) {
                    existing.hops = new_hops;
                    existing.nextHop = "R2";
                }
            }
        }
    }
    
    // 2. 处理R1中下一跳是R2的路由（R2不再通告的路由）
    // 如果R2不再通告某个网络，R1中该路由的跳数应增加1（毒性逆转）
    for (auto it = R1_table.begin(); it != R1_table.end(); ) {
        string dest = it->first;
        RouteEntry &entry = it->second;
        
        if (entry.nextHop == "R2" && R2_advert.find(dest) == R2_advert.end()) {
            // R2不再通告该网络，增加跳数
            entry.hops += 1;
            if (entry.hops > INF) {
                it = R1_table.erase(it);  // 超过16跳删除该路由
                continue;
            }
        }
        ++it;
    }
    
    // 输出更新后的路由表
    cout << "\nR1更新后的路由表:" << endl;
    cout << "目的网络\t跳数\t下一跳" << endl;
    for (auto &entry : R1_table) {
        cout << entry.first << "\t\t" << entry.second.hops << "\t" << entry.second.nextHop << endl;
    }
    
    return 0;
}
