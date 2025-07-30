#include "TestCommon.hh"

#include <base/Threading.hh>
#include <thread>
#include <print>

using namespace base;

TEST_CASE("ThreadSafe: Basic usage") {
    ThreadSafe<std::string> s;
    s.with([](std::string& s) { CHECK(s.empty()); });
    s.with([](std::string& s) { s = "foobar"; });
    s.with([](std::string& s) { CHECK(s == "foobar"); });
}

TEST_CASE("ThreadSafe: initialisation") {
    struct S {
        int x, y;
        S(int x, int y) : x{x}, y{y} {}
    };

    ThreadSafe<S> s1{4, 5};
    ThreadSafe<S> s2{S{6, 7}};
    s1.with([](S& s) {
        CHECK(s.x == 4);
        CHECK(s.y == 5);
    });

    s2.with([](S& s) {
        CHECK(s.x == 6);
        CHECK(s.y == 7);
    });
}

TEST_CASE("ThreadSafe is actually thread-safe") {
    ThreadSafe<int> s;

    {
        std::jthread t1{[&] {
            for (int i = 0; i < 500'000; i++) s.with([](int& i) { i++; });
        }};

        std::jthread t2{[&] {
            for (int i = 0; i < 500'000; i++) s.with([](int& i) { i++; });
        }};
    }

    s.with([] (int i) { CHECK(i == 1'000'000); });
}

TEST_CASE("Notifiable") {
    Notifiable<Queue<int>> n;
    int last = -1;

    {
        std::jthread t2{[&] {
            while (last != 9999) {
                n.wait(
                    [](Queue<int>& q){ return not q.empty(); },
                    [&](Queue<int>& q) {
                        while (not q.empty()) {
                            int next = q.dequeue();
                            CHECK(last + 1 == next);
                            last = next;
                        }
                    }
                );
            }
        }};

        std::jthread t1{[&] {
            for (int i = 0; i < 10'000; i++)
                n.update_one([&](Queue<int>& q) { q.push(i); });
        }};
    }

    CHECK(last == 9999);
    CHECK(n.val.empty());
}

TEST_CASE("ThreadSafeQueue") {
    ThreadSafeQueue<int> q;
    int acc{};

    {
        std::jthread t2{[&] {
            for (auto v : q.stream()) {
                acc += v;
                if (v == 9999) break;
            }
        }};

        std::jthread t1{[&] {
            for (int i = 0; i < 10'000; i++)
                q.enqueue(i);
        }};
    }

    CHECK(acc == 49'995'000);
    CHECK(q.val.empty());
}

TEST_CASE("ThreadSafeQueue: Force-close") {
    ThreadSafeQueue<int> q;
    std::atomic<int> acc{};

    {
        std::jthread t2{[&] {
            for (auto v : q.stream()) acc += v;
        }};

        std::jthread t1{[&] {
            for (int i = 0; i < 10'000; i++) q.enqueue(i);

            // Wait until t2 has processed a few elements.
            do std::this_thread::sleep_for(50ms);
            while (acc.load() < 4950);
            q.close();
        }};
    }

    // Process the rest.
    for (auto v : q.val.stream()) acc += v;
    CHECK(acc == 49'995'000);
    CHECK(q.val.empty());
}
