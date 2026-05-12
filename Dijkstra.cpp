#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <stack>
using namespace std;

// 邻接表节点结构
struct Node {
    int vertex;
    int weight;
    Node(int v, int w) : vertex(v), weight(w) {}
};

// 优先队列比较函数
struct CompareNode {
    bool operator()(pair<int, int> a, pair<int, int> b) {
        return a.second > b.second;  // 按距离从小到大排序
    }
};

class Graph {
private:
    int V;  // 顶点数
    vector<vector<Node>> adjList;
    
public:
    Graph(int vertices) {
        V = vertices;
        adjList.resize(V);
    }
    
    void addEdge(int u, int v, int weight, bool directed = false) {
        adjList[u].push_back(Node(v, weight));
        if (!directed) {
            adjList[v].push_back(Node(u, weight));
        }
    }
    
    // Dijkstra算法，返回从源点到所有顶点的最短距离和前驱节点
    pair<vector<int>, vector<int>> dijkstra(int source) {
        vector<int> dist(V, INT_MAX);
        vector<int> prev(V, -1);
        vector<bool> visited(V, false);
        
        dist[source] = 0;
        priority_queue<pair<int, int>, vector<pair<int, int>>, CompareNode> pq;
        pq.push({source, 0});
        
        while (!pq.empty()) {
            int u = pq.top().first;
            pq.pop();
            
            if (visited[u]) continue;
            visited[u] = true;
            
            for (const Node &neighbor : adjList[u]) {
                int v = neighbor.vertex;
                int weight = neighbor.weight;
                
                if (!visited[v] && dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                    prev[v] = u;
                    pq.push({v, dist[v]});
                }
            }
        }
        
        return {dist, prev};
    }
    
    // 打印从源点到目标点的路径
    void printPath(int source, int target, vector<int> &prev) {
        stack<int> path;
        int current = target;
        
        while (current != -1) {
            path.push(current);
            current = prev[current];
        }
        
        cout << "最短路径: ";
        while (!path.empty()) {
            cout << path.top();
            path.pop();
            if (!path.empty()) cout << " -> ";
        }
        cout << endl;
    }
};

int main() {
    int V, E;
    cout << "输入顶点数和边数: ";
    cin >> V >> E;
    
    Graph g(V);
    
    cout << "输入边（起点 终点 权重）:" << endl;
    for (int i = 0; i < E; i++) {
        int u, v, w;
        cin >> u >> v >> w;
        g.addEdge(u, v, w);  // 默认为无向图
    }
    
    int source;
    cout << "输入源点: ";
    cin >> source;
    
    auto result = g.dijkstra(source);
    vector<int> dist = result.first;
    vector<int> prev = result.second;
    
    cout << "\n从源点 " << source << " 出发的最短路径:" << endl;
    cout << "顶点\t距离\t路径" << endl;
    
    for (int i = 0; i < V; i++) {
        if (dist[i] == INT_MAX) {
            cout << i << "\t不可达\t-" << endl;
        } else {
            cout << i << "\t" << dist[i] << "\t";
            g.printPath(source, i, prev);
        }
    }
    
    return 0;
}
