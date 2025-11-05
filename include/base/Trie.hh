#ifndef LIBBASE_TRIE_HH
#define LIBBASE_TRIE_HH

#include <base/Assert.hh>
#include <base/DSA.hh>
#include <base/Macros.hh>
#include <base/Types.hh>
#include <queue>
#include <ranges>
#include <vector>

namespace base {
template <typename CharType>
class basic_trie;

using trie = basic_trie<char>;
using u8trie = basic_trie<char8_t>;
using u16trie = basic_trie<char16_t>;
using u32trie = basic_trie<char32_t>;
using wtrie = basic_trie<wchar_t>;
}

/// Trie for performing string replacement.
///
/// This is intended to be used for matching multiple strings at
/// once and replacing them all in a single pass over the input;
/// if you’re only matching a single pattern or if all inputs and
/// replacements are exactly one character, use stream::replace()
/// or stream::replace_many() instead if possible.
template <typename CharType>
class base::basic_trie {
public:
    using char_type = CharType;
    using text_type = std::basic_string_view<char_type>;
    using string_type = std::basic_string<char_type>;

private:
    /// Type used to refer to a node.
    using Node = u32;

    /// A node for a single element in the trie.
    struct NodeData {
        /// Children of this node.
        Map<char_type, Node> children;

        /// Whether we have a replacement.
        Node has_replacement : 1;

        /// Depth of the node. The root node has depth 0.
        Node depth : 31 = 0;

        /// Index of the node that this node fails to.
        Node fail = 0;
    };

    /// All nodes in the trie.
    std::vector<NodeData> nodes{1};

    /// Map from nodes to replacements; these are stored externally because
    /// most nodes don’t have replacements, and storing and empty string in
    /// them would just be a waste of memory.
    Map<Node, string_type> replacements;

    /// Index of the root node.
    static constexpr Node Root = 0;

    /// Whether we need to recompute the failure links.
    bool dirty = false;

public:
    /// Construct a new trie.
    explicit basic_trie() = default;

    /// Construct a trie from pairs.
    basic_trie(std::initializer_list<std::pair<text_type, text_type>> pairs) {
        for (auto [from, to] : pairs) add(from, to);
        update();
    }

    /// Add a new pattern to the trie.
    ///
    /// If the pattern already exists, the output text
    /// is replaced with the new one.
    void add(text_type pattern, text_type replacement) {
        auto current = Root;

        // Insert the pattern into the trie.
        for (auto [i, el] : utils::enumerate(pattern)) {
            if (auto ch = child(current, el)) current = *ch;
            else current = nodes[current].children[el] = allocate();
            nodes[current].depth = usz(i + 1);
        }

        replacements[current] = string_type(replacement);
        nodes[current].has_replacement = true;
    }

    /// Check if the trie contains a pattern and return its replacement.
    auto get(text_type pattern) const -> std::optional<text_type> {
        auto current = Root;
        for (auto el : pattern) {
            if (auto ch = child(current, el)) current = *ch;
            else return std::nullopt;
        }

        if (nodes[current].has_replacement) return std::optional(text_type(replacements.at(current)));
        return std::nullopt;
    }

    /// Check if this trie contains a string that matches the start of
    /// the input text.
    bool is_prefix_of(text_type haystack) const {
        auto current = Root;
        for (auto el : haystack) {
            if (nodes[current].has_replacement) return true;
            if (auto ch = child(current, el)) current = *ch;
            else return false;
        }
        return nodes[current].has_replacement;
    }

    /// Replace all occurrences of the patterns in the trie in the input.
    ///
    /// This function always returns a copy because replacing elements means
    /// that we will most likely have to move and reallocate anyway, so it’s
    /// cheaper to just allocate one range instead of moving elements around
    /// constantly.
    auto replace(text_type input) -> string_type {
        if (dirty) update();
        const auto sz = u32(input.size());
        const auto end = input.end();
        auto it = input.begin();
        auto current = Root;
        string_type out;
        Node match_node = Root;
        out.reserve(sz); // Conservative estimate.

        // Iterate through the input and perform matching.
        //
        // This is a variation of the Aho-Corasick algorithm, with
        // two main differences:
        //
        //   1. We only match the longest pattern, e.g. a trie with
        //      the elements [fo, foo], will only match the input
        //      'foo' once.
        //
        //   2. We don’t allow overlapping matches since that makes
        //      no sense for replacement, e.g. [tree, reenact] with
        //      the input 'treenact' will only match 'tree'.
        //
        // The trick to make this work is to keep track of the longest
        // match we’ve found so far at any given starting point, and
        // only append when we fail; this ensures that there is no
        // longer match at that position.
        for (;;) {
            // Record whether this is a valid match.
            //
            // We need to check this here instead of after we advance below to
            // make sure we recompute this property when we fail and reexamine
            // the current character.
            if (nodes[current].has_replacement)
                match_node = current;

            // Go to the child node if there is one.
            //
            // It’s possible for us to be at the end of the string here since we
            // may end up having to backtrack even after reaching the end.
            if (it != end) {
                const auto c = *it;
                if (auto ch = child(current, c)) {
                    current = *ch;
                    ++it;
                    continue;
                }
            }

            // If we get here, we fail.
            //
            // Note that 'current' is the node corresponding to the *previous*
            // character, i.e. the character at 'it - 1'.
            const auto current_depth = nodes[current].depth;

            // We have a match.
            if (match_node != Root) {
                // Append the replacement text.
                out += replacements.at(match_node);

                // Backtrack to to the end of the current match.
                //
                // This is necessary to handle tries where an entry can contain
                // multiple non-overlapping submatches, e.g. ["football", "foo",
                // "tba"]. If the input is "footbalq", we need to emit "foo" and
                // backtrack to right after it so we can match the "tba" as well;
                // for this to work, backtracking is necessary, and we can’t use
                // failure links or anything like that for this...
                it = it - current_depth + nodes[match_node].depth;
                current = match_node = Root;
                continue;
            }

            // We don’t have a match; follow the failure link.
            const auto prev = current;
            const auto fail = current = nodes[current].fail;

            // We fail to the root.
            if (fail == Root) {
                // If we were already at the root, skip the current character; since
                // there will never be any backtracking from the root, this is also
                // where we detect if we’re done.
                if (prev == Root) {
                    if (it == end) return out;
                    out += *it++;
                    continue;
                }

                // Otherwise, the current character needs to be re-examined, but do
                // append everything before it since it won’t be useful anymore.
                out.append(it - current_depth, it);
                continue;
            }

            // Append any text that cannot match anymore.
            //
            // That is, if we’re failing to a node with depth 'N', after buffering
            // up M characters, we need to append 'M - N' characters, starting at
            // the index where we last began traversing the trie.
            out.append(it - current_depth, it - nodes[fail].depth);
        }
    }

private:
    /// Allocate a new node.
    auto allocate() -> Node {
        dirty = true;
        nodes.emplace_back();
        return Node(nodes.size() - 1);
    }

    /// Get the child of a node.
    auto child(std::same_as<Node> auto node, std::same_as<CharType> auto child) const -> std::optional<Node> {
        auto& children = nodes.at(node).children;
        auto it = children.find(child);
        if (it == children.end()) return std::nullopt;
        return it->second;
    }

    /// Get the root node.
    auto root() -> NodeData& { return nodes[0]; }

    /// Recompute all fail links in the trie.
    void update() {
        dirty = false;

        // The root node fails to itself, which is already handled
        // by the 'Node' constructor. Next, all the children of the
        // root fail to the root.
        Queue<Node> queue;
        for (auto n : root().children | vws::values) {
            nodes[n].fail = Root;
            queue.push(n);
        }

        // Then, process the tree in BFS order. The queue only ever
        // contains nodes whose fail links have already been computed,
        // so we can safely use them.
        while (not queue.empty()) {
            auto& parent = nodes[queue.dequeue()];

            // Compute the children’s fail links.
            for (auto [character, child_node] : parent.children) {
                // The algorithm for computing a fail node is as follows:
                //
                //   1. Let F be the parent’s fail node.
                //   2. If F has a child whose element matches c, fail to
                //      that child of F.
                //   3. If F is the root node, fail to the root.
                //   4. Let F be F’s fail node and go to step 2.
                //
                for (auto f = parent.fail; /* nothing */; f = nodes[f].fail) {
                    if (auto ch = child(f, character)) {
                        nodes[child_node].fail = *ch;
                        break;
                    }

                    if (f == Root) {
                        nodes[child_node].fail = Root;
                        break;
                    }
                }

                // And register the grandchildren for processing.
                queue.push(child_node);
            }
        }
    }
};

#endif // LIBBASE_TRIE_HH
