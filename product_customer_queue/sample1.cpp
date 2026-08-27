#include "sample1.h"

#include "PCQueue.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct Order {
    int id = 0;
    int terminal_id = 0;
    std::string dish;
};

} // namespace

bool run_sample1() {
    std::cout << "\n========== Sample 1: Restaurant FIFO Queue ==========\n";

    PCQueue<Order> order_queue(3);

    constexpr int terminal_count = 3;
    constexpr int cook_count = 2;
    constexpr int orders_per_terminal = 10;
    constexpr int expected_order_count =
        terminal_count * orders_per_terminal;

    std::atomic<int> produced_count{0};
    std::atomic<int> consumed_count{0};
    std::atomic<int> blocked_push_count{0};

    std::mutex output_mutex;
    const auto print = [&output_mutex](const std::string& message) {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << message << std::endl;
    };

    const char* dishes[] = {
        "Rice",
        "Noodles",
        "Soup",
        "Hamburger"
    };

    // Consumers: cooks take orders from the shared queue.
    std::vector<std::thread> cooks;
    for (int cook_id = 0; cook_id < cook_count; ++cook_id) {
        auto cook_task = [&, cook_id]() {
            Order order;
            while (order_queue.pop(order)) {
                print("Cook " + std::to_string(cook_id)
                    + " started order " + std::to_string(order.id)
                    + " (" + order.dish + ")");

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                ++consumed_count;

                print("Cook " + std::to_string(cook_id)
                    + " finished order " + std::to_string(order.id));
            }

            print("Cook " + std::to_string(cook_id) + " exited");
        };

        cooks.emplace_back(std::move(cook_task));
    }

    // Producers: ordering terminals create and submit orders.
    std::vector<std::thread> terminals;
    for (int terminal_id = 0;
         terminal_id < terminal_count;
         ++terminal_id) {
        auto producer_task = [&, terminal_id]() {
            for (int index = 0; index < orders_per_terminal; ++index) {
                const int order_id =
                    terminal_id * orders_per_terminal + index;

                Order order{
                    order_id,
                    terminal_id,
                    dishes[order_id % 4]
                };

                const auto push_start = std::chrono::steady_clock::now();
                const bool success = order_queue.push(std::move(order));
                const auto push_end = std::chrono::steady_clock::now();
                const auto wait_time =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        push_end - push_start);

                if (!success) {
                    print("Terminal " + std::to_string(terminal_id)
                        + " stopped because queue was closed");
                    return;
                }

                if (wait_time.count() >= 20) {
                    ++blocked_push_count;
                }

                ++produced_count;
                print("Terminal " + std::to_string(terminal_id)
                    + " submitted order " + std::to_string(order_id)
                    + ", push waited " + std::to_string(wait_time.count())
                    + " ms");

                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        };

        terminals.emplace_back(std::move(producer_task));
    }

    for (std::thread& terminal : terminals) {
        terminal.join();
    }

    order_queue.shut_down();

    for (std::thread& cook : cooks) {
        cook.join();
    }

    const bool passed =
        produced_count == expected_order_count
        && consumed_count == expected_order_count
        && order_queue.empty()
        && order_queue.is_shutdown();

    std::cout << "\n---------- Sample 1 Result ----------\n";
    std::cout << "Expected orders: " << expected_order_count << '\n';
    std::cout << "Produced orders: " << produced_count.load() << '\n';
    std::cout << "Consumed orders: " << consumed_count.load() << '\n';
    std::cout << "Blocked pushes : " << blocked_push_count.load() << '\n';
    std::cout << "Queue empty    : " << std::boolalpha
              << order_queue.empty() << '\n';
    std::cout << "Test result    : "
              << (passed ? "PASSED" : "FAILED") << '\n';

    return passed;
}
