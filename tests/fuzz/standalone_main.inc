// Self-driving main() used when CAIDE_FUZZ_STANDALONE is defined (e.g. on
// macOS Apple Clang where libFuzzer isn't shipped). Feeds a small set of
// deterministic + pseudo-random inputs through LLVMFuzzerTestOneInput so
// the harness logic is at least smoke-validated.

#ifdef CAIDE_FUZZ_STANDALONE

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

int main(int argc, char** argv) {
    // Seed: 1st arg if numeric, otherwise 1.
    unsigned seed = 1;
    if (argc > 1) seed = static_cast<unsigned>(std::atoi(argv[1]));
    std::srand(seed);

    constexpr int kIterations = 200;
    constexpr size_t kMaxInput = 256;

    // Empty input + a few hand-picked corner cases.
    {
        const uint8_t empty[1] = {0};
        LLVMFuzzerTestOneInput(empty, 0);
        LLVMFuzzerTestOneInput(empty, 1);
    }

    std::vector<uint8_t> buf;
    buf.reserve(kMaxInput);
    for (int i = 0; i < kIterations; ++i) {
        size_t n = static_cast<size_t>(std::rand()) % kMaxInput;
        buf.assign(n, 0);
        for (size_t j = 0; j < n; ++j) buf[j] = static_cast<uint8_t>(std::rand());
        LLVMFuzzerTestOneInput(buf.data(), n);
    }

    std::printf("[standalone] %d iterations completed without crashing\n", kIterations);
    return 0;
}

#endif  // CAIDE_FUZZ_STANDALONE
