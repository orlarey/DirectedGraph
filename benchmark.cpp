/*******************************************************************************
********************************************************************************

    benchmark : Performance benchmarking for DirectedGraph library

    Created for performance analysis and optimization validation.
    Copyright © 2023 DirectedGraph Library. All rights reserved.

 *******************************************************************************
 ******************************************************************************/

#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "DirectedGraph/DirectedGraph.hh"
#include "DirectedGraph/DirectedGraphAlgorythm.hh"
#include "DirectedGraph/Schedule.hh"

//===========================================================
// Timing utilities
//===========================================================

class Timer {
   private:
    std::chrono::high_resolution_clock::time_point start_time;

   public:
    void start() { start_time = std::chrono::high_resolution_clock::now(); }

    [[nodiscard]] double elapsed_ms() const
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        return static_cast<double>(duration.count()) / 1000.0;  // Return milliseconds
    }
};

//===========================================================
// Graph generators for benchmarking
//===========================================================

class GraphGenerator {
   public:
    // Generate a linear chain graph: 0->1->2->...->n-1
    static digraph<int> generateChain(int n)
    {
        digraph<int> g;
        for (int i = 0; i < n; ++i) {
            g.add(i);
            if (i > 0) {
                g.add(i - 1, i, 0);
            }
        }
        return g;
    }

    // Generate a complete DAG: every node connects to all nodes with higher index - OPTIMIZED
    static digraph<int> generateCompleteDAG(int n)
    {
        digraph<int> g;

        // Add all nodes first (single batch)
        for (int i = 0; i < n; ++i) {
            g.add(i);
        }

        // Add all connections (separated from node creation)
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                g.add(i, j, 0);
            }
        }
        return g;
    }

    // Generate a random DAG with specified density - OPTIMIZED
    static digraph<int> generateRandomDAG(int n, double density, int seed = 42)
    {
        std::mt19937                     gen(static_cast<unsigned int>(seed));
        std::uniform_real_distribution<> dis(0.0, 1.0);
        std::uniform_int_distribution<>  weight_dis(0, 3);

        digraph<int> g;

        // Pre-compute connections to avoid repeated graph modifications
        std::vector<std::tuple<int, int, int>> connections;

        // Estimate capacity: max possible edges * density
        int max_edges = (n * (n - 1)) / 2;
        connections.reserve(static_cast<size_t>(max_edges * density * 1.2));  // 20% buffer

        // Generate all connections first
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                if (dis(gen) < density) {
                    connections.emplace_back(i, j, weight_dis(gen));
                }
            }
        }

        // Add all nodes first (batch operation)
        for (int i = 0; i < n; ++i) {
            g.add(i);
        }

        // Add all connections (batch operation)
        for (const auto& [from, to, weight] : connections) {
            g.add(from, to, weight);
        }

        return g;
    }

    // Generate a graph with cycles
    static digraph<int> generateCyclic(int n, int cycles)
    {
        digraph<int> g = generateRandomDAG(n, 0.3);

        std::mt19937                    gen(42);
        std::uniform_int_distribution<> node_dis(0, n - 1);

        // Add back edges to create cycles
        for (int c = 0; c < cycles; ++c) {
            int from = node_dis(gen);
            int to   = node_dis(gen);
            if (from != to) {
                g.add(std::max(from, to), std::min(from, to), 1);
            }
        }
        return g;
    }

    // Generate a tree-like structure
    static digraph<int> generateTree(int depth, int branching)
    {
        digraph<int> g;
        int          node_id = 0;

        // Pre-compute total nodes for better memory planning
        int total_nodes = 0;
        for (int d = 0; d <= depth; ++d) {
            int nodes_at_depth = 1;
            for (int i = 0; i < d; ++i) {
                nodes_at_depth *= branching;
            }
            total_nodes += nodes_at_depth;
        }

        // Collect all connections first
        std::vector<std::pair<int, int>> connections;
        connections.reserve(total_nodes - 1);  // tree has n-1 edges

        std::function<void(int, int, int)> build_tree = [&](int current, int current_depth,
                                                            int max_depth) {
            if (current_depth < max_depth) {
                for (int i = 0; i < branching; ++i) {
                    int child = ++node_id;
                    connections.emplace_back(current, child);
                    build_tree(child, current_depth + 1, max_depth);
                }
            }
        };

        build_tree(0, 0, depth);

        // Add all nodes first
        for (int i = 0; i <= node_id; ++i) {
            g.add(i);
        }

        // Add all connections
        for (const auto& [parent, child] : connections) {
            g.add(parent, child, 0);
        }

        return g;
    }
};

//===========================================================
// Benchmark suite
//===========================================================

struct BenchmarkResult {
    std::string test_name;
    std::string graph_type;
    int         graph_size;
    double      time_ms;
    std::string additional_info;
};

class BenchmarkSuite {
   private:
    std::vector<BenchmarkResult> results;
    Timer                        timer;
    std::string                  output_filename;

    template <typename Func>
    double measureTime(const std::string& test_desc, Func&& func)
    {
        std::cout << "  Running: " << test_desc << "... " << std::flush;
        timer.start();
        std::forward<Func>(func)();
        double elapsed = timer.elapsed_ms();
        std::cout << elapsed << " ms\n";
        return elapsed;
    }

    void addResult(const std::string& test, const std::string& graph_type, int size, double time,
                   const std::string& info = "")
    {
        results.push_back({test, graph_type, size, time, info});
    }

    [[nodiscard]] std::string generateTimestampFilename() const
    {
        auto now    = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm     = *std::localtime(&time_t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d%H%M%S") << "-benchmark-results.txt";
        return oss.str();
    }

   public:
    BenchmarkSuite() : output_filename(generateTimestampFilename())
    {
        std::cout << "Benchmark results will be saved to: " << output_filename << "\n\n";
    }

    // Benchmark graph construction
    void benchmarkConstruction()
    {
        std::cout << "=== Graph Construction Benchmarks ===\n";

        std::vector<int> sizes = {50, 100, 200, 500, 1000};

        for (int n : sizes) {
            std::cout << "Testing with " << n << " nodes:\n";

            // Chain construction
            double time = measureTime("Chain(" + std::to_string(n) + ")",
                                      [&]() { auto g = GraphGenerator::generateChain(n); });
            addResult("Construction", "Chain", n, time);

            // Complete DAG construction
            if (n <= 200) {  // Complete DAG grows O(n²), so limit size
                time = measureTime("CompleteDAG(" + std::to_string(n) + ")",
                                   [&]() { auto g = GraphGenerator::generateCompleteDAG(n); });
                addResult("Construction", "CompleteDAG", n, time);
            }

            // Random DAG construction
            time = measureTime("RandomDAG(" + std::to_string(n) + ")",
                               [&]() { auto g = GraphGenerator::generateRandomDAG(n, 0.1); });
            addResult("Construction", "RandomDAG", n, time);
        }
    }

    // Benchmark Tarjan algorithm (strongly connected components)
    void benchmarkTarjan()
    {
        std::cout << "=== Tarjan Algorithm Benchmarks ===\n";

        std::vector<int> sizes = {50, 100, 200, 500};

        for (int n : sizes) {
            std::cout << "Testing Tarjan with " << n << " nodes:\n";

            // Test on cyclic graphs
            auto g = GraphGenerator::generateCyclic(n, n / 10);

            int    cycle_count = 0;
            double time        = measureTime("Tarjan-Cyclic(" + std::to_string(n) + ")", [&]() {
                Tarjan<int> t(g);
                cycle_count = t.cycles();
            });

            addResult("Tarjan", "Cyclic", n, time, "cycles=" + std::to_string(cycle_count));
        }
    }

    // Benchmark scheduling algorithms
    void benchmarkScheduling()
    {
        std::cout << "=== Scheduling Algorithms Benchmarks ===\n";

        std::vector<int> sizes = {50, 100, 200};  // Note: SP-Schedule has exponential complexity

        for (int n : sizes) {
            std::cout << "Testing Scheduling with " << n << " nodes:\n";

            auto g = GraphGenerator::generateRandomDAG(n, 0.15);  // Lower density

            // DFS Schedule
            double time = measureTime("DFS-Schedule(" + std::to_string(n) + ")",
                                      [&]() { auto s = dfschedule(g); });
            addResult("DFS Schedule", "RandomDAG", n, time);

            // BFS Schedule
            time = measureTime("BFS-Schedule(" + std::to_string(n) + ")",
                               [&]() { auto s = bfschedule(g); });
            addResult("BFS Schedule", "RandomDAG", n, time);

            // Special Schedule - VERY slow O(exponential), limit to small sizes
            if (n <= 100) {  // Only test small sizes for SP-Schedule
                time = measureTime("SP-Schedule(" + std::to_string(n) + ") [WARNING: SLOW]",
                                   [&]() { auto s = spschedule(g); });
                addResult("SP Schedule", "RandomDAG", n, time);
            } else {
                std::cout << "  Skipping: SP-Schedule(" << n << ") - too slow for large graphs\n";
            }
        }
    }

    // Benchmark graph transformations
    void benchmarkTransformations()
    {
        std::cout << "=== Graph Transformations Benchmarks ===\n";

        std::vector<int> sizes = {50, 100, 200, 500};

        for (int n : sizes) {
            std::cout << "Testing Transformations with " << n << " nodes:\n";

            auto g = GraphGenerator::generateRandomDAG(n, 0.15);

            // Reverse transformation
            double time =
                measureTime("Reverse(" + std::to_string(n) + ")", [&]() { auto r = reverse(g); });
            addResult("Reverse", "RandomDAG", n, time);

            // Parallelize transformation
            time = measureTime("Parallelize(" + std::to_string(n) + ")",
                               [&]() { auto p = parallelize(g); });
            addResult("Parallelize", "RandomDAG", n, time);

            // Serialize transformation
            time = measureTime("Serialize(" + std::to_string(n) + ")",
                               [&]() { auto s = serialize(g); });
            addResult("Serialize", "RandomDAG", n, time);
        }
    }

    // Benchmark memory usage patterns
    void benchmarkMemoryUsage()
    {
        std::cout << "=== Memory Usage Benchmarks ===\n";

        std::vector<int> sizes = {100, 200, 500, 1000};

        for (int n : sizes) {
            std::cout << "Testing Memory Operations with " << n << " nodes:\n";

            // Test multiple graph copies - FIXED: separate generation from measurement
            auto g1 = GraphGenerator::generateRandomDAG(n, 0.1);  // Pre-generate outside timing

            double time = measureTime("Memory-Ops(" + std::to_string(n) + ")", [&]() {
                const auto& g2 = g1;  // Reference (no cost)
                auto        g3 = g1;  // Copy graph (shared_ptr copy - fast!)
                g3.add(g2);           // Graph merge (this is the real work)
            });
            addResult("Memory Ops", "RandomDAG", n, time);
        }
    }

    // Run all benchmarks
    void runAll()
    {
        std::cout << "Starting DirectedGraph Library Benchmarks...\n";
        std::cout << "Note: SP-Schedule (spschedule) has exponential complexity due to\n";
        std::cout << "      recursive algorithm without memoization. Limited to small graphs.\n\n";

        benchmarkConstruction();
        benchmarkTarjan();
        benchmarkScheduling();
        benchmarkTransformations();
        benchmarkMemoryUsage();

        printResults();
        saveResultsToFile();
    }

    // Save results to file
    void saveResultsToFile()
    {
        std::ofstream file(output_filename);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open " << output_filename << " for writing\n";
            return;
        }

        // Write header with metadata
        auto now    = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        file << "# DirectedGraph Library Benchmark Results\n";
        file << "# Generated: " << std::ctime(&time_t);
        file << "# Total tests: " << results.size() << "\n";
        file << "#\n";
        file << "# Format: Test_Name,Graph_Type,Size,Time_ms,Additional_Info\n";
        file << "\n";

        // Write CSV header
        file << "Test,Graph_Type,Size,Time_ms,Info\n";

        // Write all results
        for (const auto& result : results) {
            file << result.test_name << "," << result.graph_type << "," << result.graph_size << ","
                 << std::fixed << std::setprecision(2) << result.time_ms << ","
                 << result.additional_info << "\n";
        }

        file.close();
        std::cout << "\nResults saved to: " << output_filename << "\n";
    }

    // Print benchmark results
    void printResults()
    {
        std::cout << "\n=== BENCHMARK RESULTS ===\n\n";

        // Group by test type
        std::map<std::string, std::vector<BenchmarkResult*>> grouped;
        for (auto& result : results) {
            grouped[result.test_name].push_back(&result);
        }

        for (const auto& [test_name, test_results] : grouped) {
            std::cout << "--- " << test_name << " ---\n";
            std::cout << std::setw(12) << "Graph Type" << std::setw(8) << "Size" << std::setw(12)
                      << "Time (ms)" << std::setw(15) << "Info"
                      << "\n";
            std::cout << std::string(50, '-') << "\n";

            for (const auto* result : test_results) {
                std::cout << std::setw(12) << result->graph_type << std::setw(8)
                          << result->graph_size << std::setw(12) << std::fixed
                          << std::setprecision(2) << result->time_ms << std::setw(15)
                          << result->additional_info << "\n";
            }
            std::cout << "\n";
        }

        // Summary statistics
        std::cout << "=== SUMMARY ===\n";
        std::cout << "Total tests run: " << results.size() << "\n";

        double total_time = 0;
        for (const auto& result : results) {
            total_time += result.time_ms;
        }
        std::cout << "Total benchmark time: " << std::fixed << std::setprecision(2) << total_time
                  << " ms\n\n";
    }
};

//===========================================================
// Main benchmark application
//===========================================================

int main(int /* argc */, const char** /* argv */)
{
    std::cout << "DirectedGraph Library Performance Benchmark\n";
    std::cout << "===========================================\n\n";

    try {
        BenchmarkSuite suite;
        suite.runAll();

        std::cout << "\nBenchmark completed successfully!\n";
        std::cout << "Check the generated file for detailed CSV results.\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed with exception: " << e.what() << "\n";
        return 1;
    }
}