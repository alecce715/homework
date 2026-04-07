#include <iostream>
#include <map>
using namespace std;

int main() {
    map<int, int> macTable; // MAC地址 -> 端口
    int n;
    cout << "输入帧的数量: ";
    cin >> n;
    
    for (int i = 0; i < n; i++) {
        int src, srcPort, dst;
        cout << "输入第" << i+1 << "帧的源地址、源端口、目的地址: ";
        cin >> src >> srcPort >> dst;
        
        // 自学习：记录源地址和端口
        macTable[src] = srcPort;
                
        // 转发处理
        if (dst == 0xF) { // 广播地址
            int otherPort = (srcPort == 1) ? 2 : 1;
            cout << "帧" << i+1 << ": 广播，转发到端口" << otherPort << endl;
        } else {
            auto it = macTable.find(dst);
            if (it != macTable.end()) { // 地址表中有记录
                int dstPort = it->second;
                if (dstPort == srcPort) {
                    cout << "帧" << i+1 << ": 目的地址在同一端口，丢弃" << endl;
                } else {
                    cout << "帧" << i+1 << ": 转发到端口" << dstPort << endl;
                }
            } else { // 地址表中无记录
                int otherPort = (srcPort == 1) ? 2 : 1;
                cout << "帧" << i+1 << ": 未知目的地址，泛洪到端口" << otherPort << endl;
            }
        }
    }
    
    // 输出MAC地址表
    cout << "\nMAC地址表:" << endl;
    for (auto& entry : macTable) {
        cout << "MAC: " << hex << entry.first << " -> 端口" << dec << entry.second << endl;
    }
    
    return 0;
}
