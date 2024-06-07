#include "TestCommon.hh"

#include <base/Base.hh>
#include <base/FS.hh>
#include <fstream>

using namespace base;

auto TPath = std::filesystem::temp_directory_path() / "foo";

auto ThisFile() -> std::string_view {
    static std::string file_contents = [] {
        std::fstream file(__FILE__);
        return std::string{
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{},
        };
    }();
    return file_contents;
}

TEST_CASE("File::Delete, File::Exists") {
    CHECK(File::Exists(__FILE__));
    File::Open(TPath, OpenMode::ReadWrite).value();
    CHECK(File::Exists(TPath));
    CHECK(File::Delete(TPath, false).value());
    CHECK(not File::Exists(TPath));
    CHECK(not File::Delete(TPath, false).value());
    CHECK(not File::Delete("this-file-does-not-exist", false).value());
    CHECK(not File::Exists("this-file-does-not-exist"));
}

TEST_CASE("File::Delete (Recursive)") {
    File::Delete(TPath, false).value();
    std::filesystem::create_directories(TPath / "a" / "b" / "c");

    CHECK(File::Exists(TPath / "a" / "b" / "c"));
    CHECK(File::Delete(TPath, false).error() == std::format("Could not delete path '{}': Directory not empty", TPath.string()));
    CHECK(File::Delete(TPath, true).value());
    CHECK(not File::Exists(TPath));
}

TEST_CASE("File::Open") {
    File::Open(TPath, OpenMode::Write).value();
    File::Open(TPath, OpenMode::Read).value();
    File::Open(TPath, OpenMode::ReadWrite).value();
    File::Open(TPath, OpenMode::Append).value();
    CHECK(not File::Open("this-file-does-not-exist", OpenMode::Read));
    CHECK(File::Open(TPath, OpenMode(0)).error() == "Invalid open mode '0'");
    CHECK(File::Open(TPath, OpenMode(134)).error() == "Invalid open mode '134'");
}

TEST_CASE("File::ReadInto") {
    std::string a;
    File::ReadInto(__FILE__, a).value();
    CHECK(a == ThisFile());
    File::ReadInto(__FILE__, a).value();
    CHECK(a == std::string{ThisFile()} + std::string{ThisFile()});

    std::vector<char> b;
    File::ReadInto(__FILE__, b).value();
    CHECK(std::string_view{b.data(), b.size()} == ThisFile());
}

TEST_CASE("File::Read") {
    CHECK(File::Read(__FILE__).value() == ThisFile());
}

TEST_CASE("File::Write") {
    File::Write(TPath, "foobarbaz\n"sv).value();
    CHECK(File::Read(TPath).value() == "foobarbaz\n");

    File::Write(TPath, ThisFile()).value();
    CHECK(File::Read(TPath).value() == ThisFile());
}

TEST_CASE("File::mode") {
    CHECK(File::Open(TPath, OpenMode::Read).value().mode() == OpenMode::Read);
    CHECK(File::Open(TPath, OpenMode::ReadWrite).value().mode() == OpenMode::ReadWrite);
}

TEST_CASE("File::print") {
    auto f = File::Open(TPath, OpenMode::ReadWrite).value();
    f.print("foobarbaz\n").value();
    f.print("{}\n", "quxquux").value();
    f.print("{}:{}\n", 47, 74).value();
    f.rewind();
    CHECK(File::Read(TPath).value() == "foobarbaz\nquxquux\n47:74\n");
}

TEST_CASE("File::read") {
    auto f = File::Open(__FILE__).value();
    std::vector<char> into;
    f.read(into).value();

    // We haven’t allocated any space, so nothing should be read.
    CHECK(into.empty());

    // Read the first 20 chars.
    into.resize(20);
    f.read(into).value();
    CHECK(std::string_view{into.data(), into.size()} == ThisFile().substr(0, 20));

    // Read the next 20 chars.
    f.read(into).value();
    CHECK(std::string_view{into.data(), into.size()} == ThisFile().substr(20, 20));

    // Read the rest.
    into.resize(ThisFile().size() - 40);
    f.read(into).value();
    CHECK(std::string_view{into.data(), into.size()} == ThisFile().substr(40));
}

TEST_CASE("File::rewind") {
    auto f = File::Open(__FILE__).value();
    std::vector<char> into;
    into.resize(20);
    f.read(into).value();
    into.resize(40);

    rgs::fill(into, 'X');
    f.rewind();
    f.read(into).value();
    CHECK(std::string_view{into.data(), into.size()} == ThisFile().substr(0, 40));

    rgs::fill(into, 'X');
    f.rewind();
    f.read(into).value();
    CHECK(std::string_view{into.data(), into.size()} == ThisFile().substr(0, 40));
}

TEST_CASE("File::size") {
    CHECK(File::Open(__FILE__).value().size() == ThisFile().size());
}

TEST_CASE("File::resize, File::write") {
    auto f = File::Open(TPath, OpenMode::ReadWrite).value();

    f.resize(0).value();
    CHECK(f.size() == 0);

    f.write("foobarbaz\n").value();
    f.write("quxquux\n"sv).value();
    f.rewind();
    CHECK(File::Read(TPath).value() == "foobarbaz\nquxquux\n");
    CHECK(f.size() == 18);

    f.resize(10).value();
    CHECK(f.size() == 10);

    f.resize(0).value();
    CHECK(f.size() == 0);
}

TEST_CASE("File::writev") {
    auto f = File::Open(TPath, OpenMode::ReadWrite).value();
    InputView input[]{"foobarbaz\n"sv, "quxquux\n"};

    f.resize(0).value();
    CHECK(f.size() == 0);
    f.writev(input).value();
    f.rewind();
    CHECK(File::Read(TPath).value() == "foobarbaz\nquxquux\n");
    CHECK(f.size() == 18);
}

TEST_CASE("Opening in Write mode should truncate") {
    File::Write(TPath, "foo").value();
    CHECK(File::Open(TPath, OpenMode::Write).value().size() == 0);

    File::Write(TPath, "foo").value();
    CHECK(File::Open(TPath, OpenMode::ReadWrite).value().size() == 0);
}