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
        usz match_start = 0;
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
        for (usz i = 0; i < sz;) {
            const auto& c = input[i];

            // Record whether this is a valid match.
            if (nodes[current].replacement.has_value())
                match_node = current;

            // Go to the child node if there is one.
            if (auto child = nodes[current].children.get(c)) {
                current = *child;
                i++;
                continue;
            }

            // If we get here, we fail. This means we need to do several
            // things. First, append the replacement text.
            if (match_node != Root) {
                auto& rep = nodes[match_node].replacement.value();
                out.insert(out.end(), rep.begin(), rep.end());

                // Move past everything we just replaced and start matching
                // anew. We can’t use the fail link here as Aho-Corasick
                // normally would because those are only valid if we allow
                // overlapping matches, which we don’t if we perform a
                // replacement.
                i = match_start += nodes[match_node].depth;
                current = match_node = Root;
                continue;
            }

            // We fail; follow the failure link.
            auto consumed = nodes[match_node].depth;
            auto prev = current;
            current = nodes[current].fail;

            // If we fail to the root, append everything that was matched
            // so far, since it won’t be useful anymore, but keep the current
            // character—unless we fail from root to root, in which case
            // discard it.
            if (current == Root) {
                i += prev == Root;
                out.insert(
                    out.end(),
                    input.begin() + isz(match_start + consumed),
                    input.begin() + isz(i)
                );
                match_start = i;
                continue;
            }

            // Even if we don’t fail to the root, not all the text that
            // still remains unmatched needs to be preserved: if we fail
            // to a node with depth N, only preserve the last N characters
            // of the text matched so far (however, since that includes the
            // current character, we actually need to subtract 1 from the
            // depth), and append everything before that to the output since
            // it can’t match anymore.
            match_start += consumed;
            auto rest = i - match_start;
            auto to_append = rest - (nodes[current].depth - 1);
            out.insert(
                out.end(),
                input.begin() + isz(match_start),
                input.begin() + isz(match_start + to_append)
            );

            // Finally, skip past matching the rest.
            match_start += to_append;
        }

        // Check if the last node actually had a match.
        if (nodes[current].replacement.has_value())
            match_node = current;

        // We may reach the end of the string while something is still
        // matched, so append any remaining match.
        if (match_node != Root) {
            auto& rep = nodes[match_node].replacement.value();
            out.insert(out.end(), rep.begin(), rep.end());
            match_start += nodes[match_node].depth;
        }

        // As well as any trailing text.
        out.insert(out.end(), input.begin() + isz(match_start), input.end());
        return out;
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
        LIBBASE_DEBUG(dirty = false);

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
