#include <vector>
#include <algorithm>
#include <cassert>

template <typename Vertex>
struct Graph {
	std::vector<std::vector<int>> adjacency_list;
	//for each vertex remember list of adjacent vertices
	//this vertices must be in ascending order
	std::vector<Vertex> V;
	//vector of vertices
	int N;
	//number of vertices

	Graph(int NN) : N(NN), V(NN), adjacency_list(NN) {
		//create default vertexes and graph without edges
		assert(NN > 0);
	}
	Graph(std::vector<Vertex>& VV, int NN) : N(NN), V(VV), adjacency_list(NN) {
		//copy vertexes and create graph without edges
		assert(NN > 0);
		assert(NN == VV.size());
	}
	~Graph() {}

	void push_edge_quick_without_correct_order(int begin, int end) {
		//put in adjacency_list of begin vertix other vertix
		assert(0 <= begin && begin < N);
                assert(0 <= end && end < N);
                assert(begin != end);
                adjacency_list[begin].push_back(end);
	}

	void push_edge_quick(int begin, int end) {
		//put in adjacency_list of begin vertix other vertix with maximal number
		assert(0 <= begin && begin < N);
		assert(0 <= end && end < N);
		assert(begin != end);
		assert(adjacency_list[begin].empty() ||
				end > adjacency_list[begin][adjacency_list[begin].size()-1]);
		adjacency_list[begin].push_back(end);
	}

	void sort_all_adjacency_lists() {
		//create correct order in all adjacency lists
		for (int i = 0; i < N; ++i) {
			std::sort(adjacency_list[i].begin(), adjacency_list[i].end());
			std::unique(adjacency_list[i].begin(), adjacency_list[i].end());
		}
	}
};

template <typename Vertex>
void makeReverseEdges(Graph<Vertex>& graph, Graph<Vertex>& reverse_graph) {
	//delete all edges of second graph
	//copy all edges of first graph as reverse
	//adjacency_list order in second graph will correct
	assert(graph.N == reverse_graph.N);
	for (int i = 0; i < graph.N; ++i) {
		reverse_graph.adjacency_list[i].clear();
	}
	for (int i = 0; i < graph.N; ++i) {
		for (int j = 0; j < graph.adjacency_list[i].size(); ++j) {
			reverse_graph.push_edge_quick(graph.adjacency_list[i][j], i);
		}
	}
}

template <typename Vertex>
Graph<Vertex>& makeReverseGraph(Graph<Vertex>& graph) {
	Graph<Vertex> reverse_graph(graph.N);
	makeReverseEdges<Vertex>(graph, reverse_graph);
	return reverse_graph;
}

template <typename Vertex>
Graph<Vertex>& makeReverseGraphWithCopyV(Graph<Vertex>& graph) {
	Graph<Vertex> reverse_graph(graph.V, graph.N);
	makeReverseEdges<Vertex>(graph, reverse_graph);
	return reverse_graph;
}

enum color_type {white, grey, black};

struct DFSVertex {
	int parent = -1;
	int tin = -1;
	int tout = -1;
	color_type color = white;
};

int timer = 0;

void DFS(Graph<DFSVertex>& graph, int v, int p = -1) {
	graph.V[v].parent = p;
	graph.V[v].tin = timer++;
	graph.V[v].color = grey;
	for (int to: graph.adjacency_list[v]) {
		if (graph.V[to].color != white)
			continue;
		DFS(graph, to, v);
	}
	graph.V[v].tout = timer++;
	graph.V[v].color = black;
}
