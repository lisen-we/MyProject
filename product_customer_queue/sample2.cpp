#include "sample2.h"

#include "PriorityPCQueue.h"

#include <atomic>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct ServiceRequest {
    int id = 0;
    int source_id = 0;
    int priority = 0;
    std::size_t sequence = 0;
    std::string description;
};

struct HigherPriorityFirst {
    bool operator()(
        const ServiceRequest& left,
        const ServiceRequest& right) const {
        if (left.priority != right.priority) {
            return left.priority < right.priority;
        }

        // For equal priorities, the earlier submitted request comes first.
        return left.sequence > right.sequence;
    }
};

} // namespace

bool run_sample2() {
    std::cout << "\n========== Sample 2: Priority Queue ==========\n";

    constexpr int producer_count = 3;
    constexpr int requests_per_producer = 4;
    constexpr int expected_request_count =
        producer_count * requests_per_producer;

    // This sample first gathers a batch, then starts the consumer so the
    // priority order can be observed and verified deterministically.
    PriorityPCQueue<ServiceRequest, HigherPriorityFirst> request_queue(
        expected_request_count);

    std::atomic<int> produced_count{0};
    std::atomic<std::size_t> next_sequence{0};
    std::mutex output_mutex;

    const auto print = [&output_mutex](const std::string& message) {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << message << std::endl;
    };

    std::vector<std::thread> producers;
    for (int producer_id = 0;
         producer_id < producer_count;
         ++producer_id) {
        producers.emplace_back([&, producer_id]() {
            for (int index = 0;
                 index < requests_per_producer;
                 ++index) {
                const int request_id =
                    producer_id * requests_per_producer + index;
                const int priority = request_id % 3 + 1;

                ServiceRequest request{
                    request_id,
                    producer_id,
                    priority,
                    next_sequence.fetch_add(1),
                    "Request-" + std::to_string(request_id)
                };

                if (!request_queue.push(std::move(request))) {
                    return;
                }

                ++produced_count;
                print("Producer " + std::to_string(producer_id)
                    + " submitted request " + std::to_string(request_id)
                    + " with priority " + std::to_string(priority));
            }
        });
    }

    for (std::thread& producer : producers) {
        producer.join();
    }

    request_queue.shut_down();

    std::vector<ServiceRequest> processed_requests;
    std::thread consumer([&]() {
        ServiceRequest request;
        while (request_queue.pop(request)) {
            print("Worker processed request " + std::to_string(request.id)
                + " with priority " + std::to_string(request.priority));
            processed_requests.push_back(std::move(request));
        }
    });
    consumer.join();

    bool priority_order_is_correct = true;
    for (std::size_t index = 1;
         index < processed_requests.size();
         ++index) {
        const ServiceRequest& previous = processed_requests[index - 1];
        const ServiceRequest& current = processed_requests[index];

        if (previous.priority < current.priority
            || (previous.priority == current.priority
                && previous.sequence > current.sequence)) {
            priority_order_is_correct = false;
            break;
        }
    }

    const bool passed =
        produced_count == expected_request_count
        && processed_requests.size() == expected_request_count
        && priority_order_is_correct
        && request_queue.empty()
        && request_queue.is_shutdown();

    std::cout << "\n---------- Sample 2 Result ----------\n";
    std::cout << "Produced requests : " << produced_count.load() << '\n';
    std::cout << "Processed requests: " << processed_requests.size() << '\n';
    std::cout << "Priority order    : " << std::boolalpha
              << priority_order_is_correct << '\n';
    std::cout << "Test result       : "
              << (passed ? "PASSED" : "FAILED") << '\n';

    return passed;
}
