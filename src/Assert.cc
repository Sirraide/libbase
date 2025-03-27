#ifndef LIBBASE_USE_LIBASSERT
#    include <base/Assert.hh>
#    include <print>

void base::detail::AssertFail(
    std::string_view kind,
    std::string_view cond,
    std::string_view msg,
    std::source_location where
) {
    std::print(stderr, "{}:{}:{}: {}", where.file_name(), where.line(), where.column(), kind);
    if (not cond.empty()) std::print(stderr, ": '{}'", cond);
    if (not msg.empty()) std::print(stderr, ". {}", msg);
    std::println(stderr);
    std::exit(EXIT_FAILURE);
}
#endif
