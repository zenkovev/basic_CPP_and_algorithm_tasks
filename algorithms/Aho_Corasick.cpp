#include <deque>
#include <iostream>
#include <string>
#include <vector>

namespace constants {
static const int kSigma = 26;
static const int kLinkNil = -1;
}  // namespace constants

class Trie {
 private:
  struct Node {
    std::vector<int> to;
    int depth;
    std::vector<int> terms;

    Node(int depth_tmp) : to(constants::kSigma, 0), depth(depth_tmp) {}
  };

  std::vector<Node> tree_;
  std::vector<int> link_;
  std::vector<int> compressed_link_;
  std::vector<std::vector<int>> go_;

 public:
  void Add(const std::string& str, int str_number) {
    if (tree_.empty()) {
      tree_.push_back(Node(0));
    }
    int vertex = 0;  // root
    for (size_t i = 0; i < str.size(); ++i) {
      if (tree_[vertex].to[str[i] - 'a'] == 0) {
        tree_[vertex].to[str[i] - 'a'] = tree_.size();
        tree_.push_back(Node(tree_[vertex].depth + 1));
      }
      vertex = tree_[vertex].to[str[i] - 'a'];
    }
    tree_[vertex].terms.push_back(str_number);
  }

  void AhoCorasick() {
    link_ = std::vector<int>(tree_.size(), constants::kLinkNil);
    compressed_link_ = std::vector<int>(tree_.size(), constants::kLinkNil);
    go_ = std::vector<std::vector<int>>(
        tree_.size(), std::vector<int>(constants::kSigma, constants::kLinkNil));

    for (int symbol = 0; symbol < constants::kSigma; ++symbol) {
      if (tree_[0].to[symbol] != 0) {
        go_[0][symbol] = tree_[0].to[symbol];
      } else {
        go_[0][symbol] = 0;
      }
    }

    std::deque<int> queue;
    queue.push_back(0);
    while (!queue.empty()) {
      int vertex = queue[0];
      queue.pop_front();

      for (int symbol = 0; symbol < constants::kSigma; ++symbol) {
        int vertex_u = tree_[vertex].to[symbol];
        if (vertex_u == 0) {
          continue;
        }

        link_[vertex_u] = (vertex == 0) ? 0 : go_[link_[vertex]][symbol];
        compressed_link_[vertex_u] = (tree_[link_[vertex_u]].terms.empty())
                                         ? compressed_link_[link_[vertex_u]]
                                         : link_[vertex_u];

        for (int symbol_d = 0; symbol_d < constants::kSigma; ++symbol_d) {
          if (tree_[vertex_u].to[symbol_d] != 0) {
            go_[vertex_u][symbol_d] = tree_[vertex_u].to[symbol_d];
          } else {
            go_[vertex_u][symbol_d] = go_[link_[vertex_u]][symbol_d];
          }
        }

        queue.push_back(vertex_u);
      }
    }
  }

  auto CalculateOccurrences(const std::string& str, int dictionary_size) const {
    std::vector<std::vector<int>> answer(dictionary_size);
    int vertex = 0;
    for (size_t i = 0; i < str.size(); ++i) {
      vertex = go_[vertex][str[i] - 'a'];
      int vertex_u = vertex;
      if (tree_[vertex_u].terms.empty()) {
        vertex_u = compressed_link_[vertex_u];
      }
      while (vertex_u != constants::kLinkNil) {
        for (size_t j = 0; j < tree_[vertex_u].terms.size(); ++j) {
          answer[tree_[vertex_u].terms[j]].push_back(i - tree_[vertex_u].depth +
                                                     2);
        }
        vertex_u = compressed_link_[vertex_u];
      }
    }
    return answer;
  }
};

int main() {
  std::string str;
  std::cin >> str;

  Trie forest;
  int dictionary_size;
  std::cin >> dictionary_size;
  for (int i = 0; i < dictionary_size; ++i) {
    std::string tmp;
    std::cin >> tmp;
    forest.Add(tmp, i);
  }
  forest.AhoCorasick();

  auto answer = std::move(forest.CalculateOccurrences(str, dictionary_size));
  for (size_t i = 0; i < answer.size(); ++i) {
    if (answer[i].empty()) {
      std::cout << 0 << '\n';
    } else {
      std::cout << answer[i].size();
      for (size_t j = 0; j < answer[i].size(); ++j) {
        std::cout << ' ' << answer[i][j];
      }
      std::cout << '\n';
    }
  }
}
