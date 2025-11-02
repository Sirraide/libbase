#ifndef LIBBASE_TRIEMAP_HH
#define LIBBASE_TRIEMAP_HH

#include <base/DSA.hh>

namespace base {
template <typename CharType, typename ValueType>
class BasicTrieMap;

template <typename ValueType>
using TrieMap = BasicTrieMap<char, ValueType>;

template <typename ValueType>
using Trie8Map = BasicTrieMap<char8_t, ValueType>;

template <typename ValueType>
using Trie16Map = BasicTrieMap<char16_t, ValueType>;

template <typename ValueType>
using Trie32Map = BasicTrieMap<char32_t, ValueType>;

template <typename ValueType>
using TrieWMap = BasicTrieMap<wchar_t, ValueType>;
}

/// Map-like container from strings to values using a trie.
///
/// Use this with str::match_prefix().
template <typename CharType, typename ValueType>
class base::BasicTrieMap {
    using TextType = std::basic_string_view<CharType>;

    /// A node for a single element in the trie.
    struct Node {
        /// Children of this node.
        HashMap<CharType, usz> children;

        /// The replacement text for this node, if any.
        std::optional<ValueType> replacement;

        /// Depth of the node. The root node has depth 0.
        usz depth = 0;
    };

    /// All nodes in the trie.
    std::vector<Node> nodes{1};

    /// Index of the root node.
    static constexpr usz Root = 0;

public:
    /// Construct a new trie.
    explicit BasicTrieMap() = default;

    /// Construct a trie from pairs.
    BasicTrieMap(std::initializer_list<std::pair<TextType, ValueType>> pairs) {
        for (auto [from, to] : pairs) add(from, to);
    }

    /// Add a new pattern to the trie.
    ///
    /// If the pattern already exists, the output text
    /// is replaced with the new one.
    void add(TextType pattern, ValueType replacement) {
        auto current = Root;
        for (auto [i, el] : utils::enumerate(pattern)) {
            if (auto child = nodes[current].children.get(el)) current = *child;
            else current = nodes[current].children[el] = allocate();
            nodes[current].depth = usz(i + 1);
        }
        nodes[current].replacement = std::move(replacement);
    }

    /// Check if the trie contains a pattern and return its replacement and depth.
    ///
    /// \see str::match_prefix().
    auto match_prefix(TextType pattern) const -> std::optional<std::pair<u32, ValueType>> {
        auto current = Root;
        auto last_match = Root;
        for (auto el : pattern) {
            if (nodes[current].replacement.has_value()) last_match = current;
            if (auto child = nodes[current].children.get(el)) current = *child;
            else break;
        }

        if (nodes[current].replacement.has_value()) last_match = current;
        if (not nodes[last_match].replacement.has_value()) return std::nullopt;
        return std::pair{nodes[last_match].depth, *nodes[last_match].replacement};
    }

private:
    /// Allocate a new node.
    auto allocate() -> usz {
        nodes.emplace_back();
        return nodes.size() - 1;
    }

    /// Get the root node.
    auto root() -> Node& { return nodes[0]; }
};

#endif // LIBBASE_TRIEMAP_HH
