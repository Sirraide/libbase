#ifndef LIBBASE_TRIE_HH
#define LIBBASE_TRIE_HH

#include <base/Assert.hh>
#include <base/DSA.hh>
#include <base/Macros.hh>
#include <base/Types.hh>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <vector>

#ifdef __cpp_lib_generator

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
    /// A node for a single element in the trie.
    struct Node {
        /// Children of this node.
        HashMap<char_type, usz> children;

        /// The replacement text for this node, if any.
        std::optional<string_type> replacement;

        /// Index of the node that this node fails to.
        usz fail = 0;

        /// Depth of the node. The root node has depth 0.
        usz depth = 0;
    };

    /// All nodes in the trie.
    std::vector<Node> nodes{1};

    /// Index of the root node.
    static constexpr usz Root = 0;

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
            if (auto child = nodes[current].children.get(el)) current = *child;
            else current = nodes[current].children[el] = allocate();
            nodes[current].depth = usz(i + 1);
        }

        nodes[current].replacement = string_type(replacement);
    }

    /// Replace all occurrences of patterns in the input.
    ///
    /// This function always returns a copy because replacing
    /// elements means that we will most likely have to move
    /// and reallocate anyway, so it’s cheaper to just allocate
    /// one range instead of moving elements around constantly.
    auto replace(text_type input) -> string_type {
        if (dirty) update();
        const auto sz = usz(input.size());
        auto current = Root;
        string_type out;
        usz match_node = Root;
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
        usz i = 0;
        for (;;) {
            // Record whether this is a valid match.
            //
            // We need to check this here instead of after we advance below to
            // make sure we recompute this property when we fail and reexamine
            // the current character.
            if (nodes[current].replacement.has_value())
                match_node = current;

            // Go to the child node if there is one.
            //
            // It’s possible for us to be at the end of the string here since we
            // may end up having to backtrack even after reaching the end.
            if (i < sz) {
                const auto c = input[i];
                if (auto child = nodes[current].children.get(c)) {
                    current = *child;
                    i++;
                    continue;
                }
            }

            // If we get here, we fail.
            //
            // Note that 'current' is the node corresponding to the *previous*
            // character, i.e. the character at index 'i - 1'.
            auto current_depth = nodes[current].depth;

            // We have a match.
            if (match_node != Root) {
                // Append the replacement text.
                out += nodes[match_node].replacement.value();

                // Backtrack to to the end of the current match.
                //
                // This is necessary to handle tries where an entry can contain
                // multiple non-overlapping submatches, e.g. ["football", "foo",
                // "tba"]. If the input is "footbalq", we need to emit "foo" and
                // backtrack to right after it so we can match the "tba" as well;
                // for this to work, backtracking is necessary, and we can’t use
                // failure links or anything like that for this..
                i = i - current_depth + nodes[match_node].depth;
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
                    if (i == sz) return out;
                    out += input[i];
                    i++;
                    continue;
                }

                // Otherwise, the current character needs to be re-examined, but do
                // append everything before it since it won’t be useful anymore.
                out.append(
                    input.begin() + isz(i - current_depth),
                    input.begin() + isz(i)
                );
                continue;
            }

            // Append any text that cannot match anymore.
            //
            // That is, if we’re failing to a node with depth 'N', after buffering
            // up M characters, we need to append 'M - N' characters, starting at
            // the index where we last began traversing the trie.
            out.append(
                input.begin() + isz(i - current_depth),
                input.begin() + isz(i - nodes[fail].depth)
            );
        }
    }

private:
    /// Allocate a new node.
    auto allocate() -> usz {
        dirty = true;
        nodes.emplace_back();
        return nodes.size() - 1;
    }

    /// Get the root node.
    auto root() -> Node& { return nodes[0]; }

    /// Recompute all fail links in the trie.
    void update() {
        dirty = false;

        // The root node fails to itself, which is already handled
        // by the 'Node' constructor. Next, all the children of the
        // root fail to the root.
        Queue<usz> queue;
        for (auto n : root().children | vws::values) {
            nodes[n].fail = Root;
            queue.push(n);
        }

        // Then, process the tree in BFS order. The queue only ever
        // contains nodes whose fail links have already been computed,
        // so we can safely use them.
        for (auto i : queue.stream()) {
            auto& parent = nodes[i];

            // Compute the children’s fail links.
            for (auto& c : parent.children) {
                // The algorithm for computing a fail node is as follows:
                //
                //   1. Let F be the parent’s fail node.
                //   2. If F has a child whose element matches c, fail to
                //      that child of F.
                //   3. If F is the root node, fail to the root.
                //   4. Let F be F’s fail node and go to step 2.
                //
                for (auto f = parent.fail; /* nothing */; f = nodes[f].fail) {
                    if (auto child = nodes[f].children.get(c.first)) {
                        nodes[c.second].fail = *child;
                        break;
                    }

                    if (f == Root) {
                        nodes[c.second].fail = Root;
                        break;
                    }
                }

                // And register the grandchildren for processing.
                queue.push(c.second);
            }
        }
    }
};

#endif // __cpp_lib_generator

#endif // LIBBASE_TRIE_HH
