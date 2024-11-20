class base::fs::FileContents {
    friend File;

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

    /// Get the data of this file.
    template <typename T = char>
    [[nodiscard]] auto data() const -> const T* { return static_cast<const T*>(ptr); }

    /// Get the size of this file.
    [[nodiscard]] auto size() const -> usz { return sz; }

    /// Get the contents as a string view.
    [[nodiscard]] auto view() const -> std::string_view {
        return {static_cast<const char*>(ptr), sz};
    }

private:
    void Delete();
};
