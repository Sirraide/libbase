#ifndef LIBBASE_FS_HH
#   error Include <base/FS.hh> instead.
#endif

class base::fs::FileContents : public FileContentsBase {
    friend File;
    friend FileContentsBase;

    std::vector<char> vec;

public:
    FileContents() = default;

private:
    auto _m_ptr() const -> const void* { return vec.data(); }
    auto _m_size() const -> usz { return vec.size(); }
};
