#ifndef LIBBASE_SQL_HH
#define LIBBASE_SQL_HH

#include <base/Macros.hh>
#include <base/Result.hh>
#include <base/Str.hh>
#include <string_view>

#ifdef LIBBASE_ENABLE_SQLITE
namespace base::sql {
class Database;

/// Blob of bytes.
using Blob = std::vector<std::byte>;

/// Reference to a blob.
using BlobRef = Span<std::byte>;

namespace internal {
#define LIBBASE_SQL_HANDLE(cls) \
        friend Handle;          \
        using Handle::Handle;   \
    private:                    \
        void Delete() noexcept; \
        auto ptr();

template <typename Derived>
class Handle {
protected:
    void* handle = nullptr;
    explicit Handle(void* handle) : handle{handle} {}

public:
    ~Handle() noexcept { static_cast<Derived*>(this)->Delete(); }
    Handle(Handle&& other) noexcept : handle(std::exchange(other.handle, nullptr)) {}
    Handle& operator=(Handle&& other) noexcept {
        if (this == std::addressof(other)) return *this;
        static_cast<Derived>(this)->Delete();
        handle = std::exchange(other.handle, nullptr);
        return *this;
    }
};

// NOT constexpr so we fail constant evaluation!
inline void ConstexprError(std::string_view) {}

template <typename ...Args>
struct QueryStrImpl {
    std::string_view value;

    // Validate a query string.
    template <typename T>
    requires std::convertible_to<const T&, std::string_view>
    consteval QueryStrImpl(const T& input) : value(std::string_view(input)) {
        enum struct ParameterKind {
            None,       ///< No query parameters so far.
            Positional, ///< '?' parameters.
            Numeric,    ///< '?NNN' parameters.
        };

        auto kind = ParameterKind::None;
        str s{value};
        u32 num_params{};
        while (not s.drop_until_any("?':@$").empty()) {
            // Named parameter.
            if (s.starts_with_any(":@$")) ConstexprError("Named query parameters are not supported");

            // String literal.
            else if (s.consume('\'')) {
                for (;;) {
                    s.drop_until('\'').drop();
                    if (not s.consume('\'')) break; // "''" is an escaped "'".
                }
            }

            // Query parameter.
            else {
                // Yeet '?'.
                Assert(s.consume('?'));

                // Numeric query parameter.
                if (s.starts_with(text::IsDigit)) {
                    if (kind == ParameterKind::Positional) ConstexprError("Mixing '?' and '?N' parameter is not supported");
                    kind = ParameterKind::Numeric;
                    auto num_str = s.take_while(text::IsDigit);
                    u32 num{};
                    auto res = std::from_chars(num_str.begin(), num_str.end(), num);
                    if (res.ptr != num_str.end() or res.ec != std::errc()) ConstexprError("Invalid query parameter number");
                    num_params = std::max(num, num_params);
                }

                // Positional parameter.
                else {
                    if (kind == ParameterKind::Numeric) ConstexprError("Mixing '?' and '?N' parameter is not supported");
                    kind = ParameterKind::Positional;
                    num_params++;
                }
            }
        }

        // Validate that we have *exactly* enough parameters.
        if (sizeof...(Args) != num_params) ConstexprError("Query parameter count mismatch");

        // Validate that each parameter type is valid.
        static_assert(
            ((std::integral<Args> or utils::convertible_to_any<Args, std::string_view, BlobRef>) and ...),
            "Query parameter must be a std::string_view, integer, or BlobRef"
        );
    }
};

// Prevent deduction.
template <typename ...Args>
using QueryStr = std::type_identity_t<QueryStrImpl<Args...>>;

class Query : public Handle<Query> {
    LIBBASE_SQL_HANDLE(Query);
    friend Database;
    u32 next_index = 1;
};
} // namespace internal

/// Database connexion.
///
/// This API is *not* thread-safe; if you want to access the same SQLite database
/// from multiple threads, create multiple database connexions.
class Database : public internal::Handle<Database> {
    LIBBASE_SQL_HANDLE(Database);
    using Query = internal::Query;

public:
    /// Create or open a database at 'path'.
    ///
    /// \throws std::runtime_error if the database could not be created.
    /// \see Create() for a non-throwing version.
    explicit Database(utils::zstring path);

    /// Create or open a new database at 'path'.
    /// \see OpenExisting()
    static auto Create(utils::zstring path) -> Result<Database>;

    /// Create an in-memory database.
    static auto CreateInMemory() -> Result<Database>;

    /// Open an existing database at 'path' in read-only mode.
    ///
    /// Notably, this also fails if the database does not exist.
    /// \see Create()
    static auto OpenExisting(utils::zstring path) -> Result<Database>;

    /// Execute SQL.
    template <typename ...Args>
    auto exec(internal::QueryStr<Args...> query_str, Args&& ...args) -> Result<> {
        Query query = Try(prepare(query_str.value, std::forward<Args>(args)...));
        Try(exec_impl(query));
        return {};
    }

    /// Execute a statement that evaluates to a single value.
    template <typename Value = i64, typename ...Args>
    auto query_value(internal::QueryStr<Args...> query_str, Args&& ...args) -> Result<Value> {
        Query query = Try(prepare(query_str.value, std::forward<Args>(args)...));
        if (not Try(exec_impl(query))) return Error("SQL error: No value returned for query_value()");
        return extract<Value>(query, 0);
    }

    /// Query rows as tuples.
    ///
    /// This returns a vector<tuple<Fields...>> or a vector<Fields...[0]> if there
    /// is exactly one field.
    template <typename ...Fields, typename ...Args>
    auto query(
        internal::QueryStr<Args...> query_str,
        Args&& ...args
    ) -> Result<std::vector<std::conditional_t<sizeof...(Fields) == 1, Fields...[0], std::tuple<Fields...>>>> {
        static_assert(sizeof...(Fields) != 0, "Must specify at least 1 field");
        using Row = std::conditional_t<sizeof...(Fields) == 1, Fields...[0], std::tuple<Fields...>>;
        std::vector<Row> rows;
        Query query = Try(prepare(query_str.value, std::forward<Args>(args)...));
        while (Try(exec_impl(query))) {
            if constexpr (sizeof...(Fields) == 1) {
                rows.push_back(Try(extract<Fields...[0]>(query, 0)));
            } else {
                auto GetRow = [&]<usz ...I>(std::index_sequence<I...>) -> Result<Row> {
                    Row row;
                    Result<> err;
                    (void) ([&] {
                        auto val = extract<std::tuple_element_t<I, Row>>(query, I);
                        if (not val) {
                            err = std::unexpected(std::move(val.error()));
                            return false;
                        }

                        std::get<I>(row) = std::move(val.value());
                        return true;
                    }() and ...);
                    Try(err);
                    return row;
                };

                rows.push_back(Try(GetRow(std::index_sequence_for<Fields...>())));
            }
        }
        return rows;
    }

private:
    auto bind(Query& query, std::string_view val, u32 param) -> Result<>;
    auto bind(Query& query, BlobRef val, u32 param) -> Result<>;
    auto bind(Query& query, i64 val, u32 param) -> Result<>;
    auto bind(Query& query, std::integral auto val, u32 param) -> Result<> {
        static_assert(sizeof(val) <= sizeof(i64), "Integer types with size >64 bits are not supported");
        return bind(query, i64(val), param);
    }

    template <typename T>
    auto extract(Query& query, u32 col) -> Result<T> {
        static_assert(
            (std::integral<T> and sizeof(T) <= sizeof(i64)) or utils::is_same<T, std::string, Blob>,
            "Query result must be an integer (<=64 bits), std::string, or Blob"
        );

        if constexpr (std::integral<T>) {
            auto v = Try(extract_int(query, col));
            return T(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return extract_string(query, col);
        } else {
            return extract_blob(query, col);
        }
    }

    auto extract_int(Query& query, u32 col) -> Result<i64>;
    auto extract_string(Query& query, u32 col) -> Result<std::string>;
    auto extract_blob(Query& query, u32 col) -> Result<Blob>;
    auto exec_impl(Query& query) -> Result<bool>;

    template <typename ...Args>
    auto prepare(std::string_view query_str, Args&& ...args) -> Result<Query> {
        Query query = Try(prepare_impl(query_str));
        Result<> err;
        [&]<usz ...I>(std::index_sequence<I...>){
            (void) ([&] {
                auto res = bind(query, std::forward<Args>(args), I + 1);
                if (not res) {
                    err = std::unexpected(std::move(res.error()));
                    return false;
                }
                return true;
            }() and ...);
        }(std::index_sequence_for<Args...>{});
        Try(err);
        return query;
    }

    auto prepare_impl(std::string_view query_str) -> Result<Query>;
    static auto CreateImpl(utils::zstring path, int flags) -> Result<Database>;
};
}

#undef LIBBASE_SQL_HANDLE
#endif // LIBBASE_ENABLE_SQLITE
#endif // LIBBASE_SQL_HH
