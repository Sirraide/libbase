class base::FileContents {
    friend File;

    std::vector<char> vec;

public:
    FileContents() = default;

    /// Get the data of this file.
    template <typename T = char>
    [[nodiscard]] auto data() const -> const T* { return reinterpret_cast<const T*>(vec.data()); }

    /// Get the size of this file.
    [[nodiscard]] auto size() const -> usz { return vec.size(); }

    /// Get the contents as a string view.
    [[nodiscard]] auto view() const -> std::string_view {
        return {vec.data(), vec.size()};
    }
};
