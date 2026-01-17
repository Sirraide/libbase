#ifdef LIBBASE_ENABLE_SQLITE
#include <base/SQL.hh>
#include <sqlite3.h>

using namespace base;
using namespace base::sql;

/// ====================================================================
///  Handle API
/// ====================================================================
auto Database::ptr() { return static_cast<sqlite3*>(handle); }
auto internal::Query::ptr() { return static_cast<sqlite3_stmt*>(handle); }

void Database::Delete() noexcept {
    int code = sqlite3_close_v2(ptr());
    Assert(code != SQLITE_BUSY, "Database closed while connexions were still open");
}

void internal::Query::Delete() noexcept {
    // The return value of this indicates whether there were any errors during
    // evaluation; we don’t care about that here.
    sqlite3_finalize(ptr());
}

/// ====================================================================
///  Database
/// ====================================================================
Database::Database(utils::zstring path) : Database(Create(path).value()) {}


auto Database::Create(utils::zstring path) -> Result<Database> {
    return CreateImpl(path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
}

auto Database::CreateImpl(utils::zstring path, int flags) -> Result<Database> {
    sqlite3* handle = nullptr;
    int code = sqlite3_open_v2(
        path.c_str(),
        &handle,
        flags | SQLITE_OPEN_NOMUTEX,
        nullptr
    );

    if (code != SQLITE_OK) {
        auto err = Error(
            "Failed to open database at '{}': {}",
            path,
            handle ? sqlite3_errmsg(handle) : sqlite3_errstr(code)
        );

        // SQLite usually creates the handle even if opening fails.
        sqlite3_close(handle);
        return err;
    }

    return Database(handle);
}

auto Database::CreateInMemory() -> Result<Database> {
    return CreateImpl(
        ":memory:",
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY
    );
}

auto Database::OpenExisting(utils::zstring path) -> Result<Database> {
    return CreateImpl(path, SQLITE_OPEN_READONLY);
}

auto Database::bind(Query& query, BlobRef val, u32 param) -> Result<> {
    int code = sqlite3_bind_blob64(
        query.ptr(),
        int(param),
        val.data(),
        sqlite3_uint64(val.size()),
        SQLITE_TRANSIENT
    );

    if (code != SQLITE_OK) return Error("Failed to bind parameter: {}", sqlite3_errstr(code));
    return {};
}

auto Database::bind(Query& query, std::string_view val, u32 param) -> Result<> {
    int code = sqlite3_bind_text64(
        query.ptr(),
        int(param),
        val.data(),
        sqlite3_uint64(val.size()),
        SQLITE_TRANSIENT,
        SQLITE_UTF8
    );

    if (code != SQLITE_OK) return Error("Failed to bind parameter: {}", sqlite3_errstr(code));
    return {};
}

auto Database::bind(Query& query, i64 val, u32 param) -> Result<> {
    int code = sqlite3_bind_int64(query.ptr(), int(param), sqlite_int64(val));
    if (code != SQLITE_OK) return Error("Failed to bind parameter: {}", sqlite3_errstr(code));
    return {};
}

auto Database::exec_impl(Query& query) -> Result<bool> {
    switch (int code = sqlite3_step(query.ptr())) {
        default: return Error("SQL error: sqlite3_step() returned unexpected code {}. Message: {}", code, sqlite3_errstr(code));
        case SQLITE_MISUSE: return Error("SQL error: {}. This is a bug in libbase.", sqlite3_errstr(code));
        case SQLITE_ERROR: return Error("SQL error: {}", sqlite3_errstr(code));
        case SQLITE_BUSY: return Error("SQL error: Database is busy; try again later");
        case SQLITE_ROW: return true;
        case SQLITE_DONE: return false;
    }
}

auto Database::extract_blob(Query& query, u32 col) -> Result<Blob> {
    if (sqlite3_column_type(query.ptr(), int(col)) != SQLITE_BLOB)
        return Error("Column {} is not a blob", col);

    auto size = sqlite3_column_bytes(query.ptr(), int(col));
    auto data = static_cast<const std::byte*>(sqlite3_column_blob(query.ptr(), int(col)));
    return Blob(data, data + size);
}

auto Database::extract_int(Query& query, u32 col) -> Result<i64> {
    if (sqlite3_column_type(query.ptr(), int(col)) != SQLITE_INTEGER)
        return Error("Column {} is not an integer", col);

    return i64(sqlite3_column_int64(query.ptr(), int(col)));
}

auto Database::extract_string(Query& query, u32 col) -> Result<std::string> {
    if (sqlite3_column_type(query.ptr(), int(col)) != SQLITE_TEXT)
        return Error("Column {} is not a string", col);

    auto size = sqlite3_column_bytes(query.ptr(), int(col));
    auto data = sqlite3_column_text(query.ptr(), int(col));
    return std::string(reinterpret_cast<const char*>(data), usz(size));
}

auto Database::prepare_impl(std::string_view query_str) -> Result<Query> {
    sqlite3_stmt* stmt{};
    int code = sqlite3_prepare_v2(
        ptr(),
        query_str.data(),
        int(query_str.size()),
        &stmt,
        nullptr
    );

    if (code != SQLITE_OK) return Error(
        "Failed to compile statement: {}. Statement was:\n{}",
        sqlite3_errstr(code),
        query_str
    );

    return Query(stmt);
}

#endif // LIBBASE_ENABLE_SQLITE
