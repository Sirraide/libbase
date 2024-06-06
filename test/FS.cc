#include "TestCommon.hh"

#include <base/FS.hh>
#include <base/Base.hh>
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

TEST_CASE("File::Open") {
    CHECK(File::Open(TPath, OpenFlags::Create).has_value());
    CHECK(File::Open(TPath, OpenFlags::Read).has_value());
    CHECK(File::Open(TPath, OpenFlags::Write).has_value());
    CHECK(File::Open(TPath, OpenFlags::ReadWrite).has_value());
    CHECK(File::Open(TPath, OpenFlags::Append).has_value());
    CHECK(not File::Open("this-file-does-not-exist", OpenFlags::Read));
    CHECK(File::Open(TPath, OpenFlags(0)).error() == "No access mode specified");
    CHECK(File::Open(TPath, OpenFlags::Append | OpenFlags::Write).error() == "'Append' and 'Write' are exclusive");
    CHECK(File::Open(TPath, OpenFlags(134'124'134'134)).error() == "Invalid flags specified");
}

TEST_CASE("File::flags()") {
    CHECK(File::Open(TPath, OpenFlags::Create).value().flags() == OpenFlags::Create);
    CHECK(File::Open(TPath, OpenFlags::ReadWrite).value().flags() == OpenFlags::ReadWrite);
}

TEST_CASE("File::read()") {
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

TEST_CASE("File::rewind()") {
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

TEST_CASE("File::size()") {
    CHECK(File::Open(__FILE__).value().size() == ThisFile().size());
}

TEST_CASE("File::write()") {
    auto f = File::Open(TPath, OpenFlags::ReadWrite).value();
    auto s1 = "foobarbaz\n"sv;
    auto s2 = "quxquux\n"sv;
    f.write({reinterpret_cast<const std::byte*>(s1.data()), s1.size()}).value();
    f.write({reinterpret_cast<const std::byte*>(s2.data()), s2.size()}).value();
    f.rewind();
    CHECK(File::Read(TPath).value() == "foobarbaz\nquxquux\n");
    CHECK(f.size() == 18);
}

TEST_CASE("File::Read") {
    CHECK(File::Read(__FILE__).value() == ThisFile());
}
