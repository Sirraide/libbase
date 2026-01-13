#ifdef LIBBASE_ENABLE_SQLITE

#include "TestCommon.hh"
#include <base/SQL.hh>

using namespace base;
using namespace base::sql;

TEST_CASE("SQL: Basic example") {
    auto db = Database::CreateInMemory().value();
    db.exec("CREATE TABLE foo (bar INTEGER) STRICT").check();
    db.exec("INSERT INTO foo (bar) VALUES (?)", 42).check();
    CHECK(db.query_value<i64>("SELECT bar FROM foo LIMIT 1").value() == 42);
}

TEST_CASE("SQL: exec() propagates errors") {
    auto db = Database::CreateInMemory().value();
    CHECK_THROWS(db.exec("CREATE TABLE foo").check());
    CHECK_THROWS(db.exec("q3epotjolikjwoierjgopiwrajgowerijgn").check());
    CHECK_THROWS(db.query_value("q3epotjolikjwoierjgopiwrajgowerijgn").check());
    CHECK_THROWS(db.query<i64>("q3epotjolikjwoierjgopiwrajgowerijgn").check());
}

TEST_CASE("SQL: query()") {
    auto db = Database::CreateInMemory().value();
    db.exec(R"sql(
        CREATE TABLE foo (
            bool INTEGER,
            int INTEGER,
            text TEXT,
            blob BLOB
        ) STRICT
    )sql").check();

    std::vector blob{std::byte(1), std::byte(2), std::byte(3)};
    db.exec(
        "INSERT INTO foo (bool, int, text, blob) VALUES (?, ?, ?, ?)",
        true,
        42,
        "foobarbaz",
        blob
    ).check();

    auto rows = db.query<bool, int, std::string, Blob>("SELECT * FROM foo").value();
    REQUIRE(rows.size() == 1);
    const auto& [bool_val, int_val, str_val, blob_val] = rows[0];
    CHECK(bool_val == true);
    CHECK(int_val == 42);
    CHECK(str_val == "foobarbaz");
    CHECK(blob_val == blob);
}

TEST_CASE("SQL: query() many rows") {
    auto db = Database::CreateInMemory().value();
    db.exec("CREATE TABLE foo (int INTEGER) STRICT").check();
    db.exec("INSERT INTO foo (int) VALUES (?), (?), (?), (?)", 1, 2, 3, 4).check();
    CHECK(db.query<int>("SELECT int FROM foo ORDER BY int ASC").value() == std::vector{1, 2, 3, 4});
    CHECK(db.query<int>("SELECT int FROM foo ORDER BY int DESC").value() == std::vector{4, 3, 2, 1});
}

TEST_CASE("SQL: numbered parameters") {
    auto db = Database::CreateInMemory().value();
    db.exec("CREATE TABLE foo (a INTEGER, b INTEGER) STRICT").check();
    db.exec("INSERT INTO foo (a, b) VALUES (?1, ?2), (?1, ?3), (?1, ?2), (?3, ?2)", 7, 8, 9).check();
    CHECK(db.query<int>("SELECT a FROM foo ORDER BY a ASC").value() == std::vector{7, 7, 7, 9});
    CHECK(db.query<int>("SELECT b FROM foo ORDER BY b ASC").value() == std::vector{8, 8, 8, 9});
}

#endif // LIBBASE_ENABLE_SQLITE
