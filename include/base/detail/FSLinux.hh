#ifndef LIBBASE_FS_HH
#   error Include <base/FS.hh> instead.
#endif

class base::fs::FileContents : public FileContentsBase {
    friend File;
    friend FileContentsBase;

    void* ptr = nullptr;
    usz sz = 0;

    explicit FileContents(void* ptr, usz sz) : ptr(ptr), sz(sz) {}

public:
    FileContents() = default;
    FileContents(const FileContents&) = delete;
    FileContents& operator=(const FileContents&) = delete;
    FileContents(FileContents&& other) noexcept
        : ptr(std::exchange(other.ptr, nullptr)),
          sz(std::exchange(other.sz, 0)) {}

    FileContents& operator=(FileContents&& other) noexcept {
        if (this != &other) {
            Delete();
            ptr = std::exchange(other.ptr, nullptr);
            sz = std::exchange(other.sz, 0);
        }

        return *this;
    }

    ~FileContents() { Delete(); }

private:
    auto _m_ptr() const -> const void* { return ptr; }
    auto _m_size() const -> usz { return sz; }
    void Delete();
};
