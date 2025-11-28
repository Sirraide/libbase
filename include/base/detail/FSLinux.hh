class base::fs::FileContentsBase {
    friend File;

    void* ptr = nullptr;
    usz sz = 0;

    explicit FileContentsBase(void* ptr, usz sz) : ptr(ptr), sz(sz) {}

public:
    FileContentsBase() = default;
    FileContentsBase(const FileContentsBase&) = delete;
    FileContentsBase& operator=(const FileContentsBase&) = delete;
    FileContentsBase(FileContentsBase&& other) noexcept
        : ptr(std::exchange(other.ptr, nullptr)),
          sz(std::exchange(other.sz, 0)) {}

    FileContentsBase& operator=(FileContentsBase&& other) noexcept {
        if (this != &other) {
            Delete();
            ptr = std::exchange(other.ptr, nullptr);
            sz = std::exchange(other.sz, 0);
        }

        return *this;
    }

    ~FileContentsBase() { Delete(); }

    /// Get the data of this file.
    template <utils::ByteSizedPointee T = char>
    [[nodiscard]] auto data() const -> const T* { return static_cast<const T*>(ptr); }

    /// Get the size of this file.
    [[nodiscard]] auto size() const -> usz { return sz; }

private:
    void Delete();
};
