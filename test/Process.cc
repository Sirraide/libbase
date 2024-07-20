#include "TestCommon.hh"

#include <base/Process.hh>

using namespace base;
using namespace std::literals;

TEST_CASE("Can spawn processes") {
    CHECK(Process::SpawnBlocking("echo", {"echo", "hello"}).success());
    CHECK(Process::SpawnBlocking("echo", {"echo", "hello"}).success());
}

TEST_CASE("Can spawn processes asynchronously") {
    std::array args = {"echo", "hello"};
    CHECK(Process::SpawnAsync("echo", args).value().wait().success());
    CHECK(Process::SpawnAsync("echo", args).value().wait().success());
}

TEST_CASE("Can redirect streams") {
    auto res1 = Process::Builder("echo", {"echo", "hello"})
        .capture(Process::Stream::Stdout)
        .spawn_blocking();

    CHECK(res1.success());
    CHECK(res1.output_stream() == "hello\n");

    auto res2 = Process::Builder("echo", {"echo", "hello"})
        .redirect(Process::Stream::Stdout, Process::Stream::Stderr)
        .capture(Process::Stream::Stderr)
        .spawn_blocking();

    CHECK(res2.success());
    CHECK(res2.error_stream() == "hello\n");
}