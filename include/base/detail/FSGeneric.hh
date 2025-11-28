class base::fs::FileContentsBase {
    friend File;

    std::vector<char> vec;

public:
    FileContentsBase() = default;

    /// Get the data of this file.
    template <utils::ByteSizedPointee T = char>
    [[nodiscard]] auto data() const -> const T* {
        return reinterpret_cast<const T*>(vec.data());
    }

    /// Get the size of this file.
    [[nodiscard]] auto size() const -> usz { return vec.size(); }
};
