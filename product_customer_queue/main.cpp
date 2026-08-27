#include "sample1.h"
#include "sample2.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc == 1) {
        const bool sample1_passed = run_sample1();
        const bool sample2_passed = run_sample2();
        return sample1_passed && sample2_passed ? 0 : 1;
    }

    const std::string sample_name = argv[1];
    if (sample_name == "sample1") {
        return run_sample1() ? 0 : 1;
    }

    if (sample_name == "sample2") {
        return run_sample2() ? 0 : 1;
    }

    std::cerr << "Usage: product_customer_queue [sample1|sample2]\n";
    return 2;
}
