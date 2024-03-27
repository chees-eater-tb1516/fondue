#include <iostream>
#include <chrono>
#include <thread>
#include <random>

int main() {
  // Setup of random number generation
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> distribution(100, 100000000);;

  // Loop duration
  double frequency {2.0};
  std::chrono::duration<double> loop_duration(1.0 / frequency);
  
  // Number of iterations
  const unsigned int n {100};
  unsigned int counter {0};
  
  // Variable to accumulate durations
  std::chrono::duration<double> total {0};
  
  // Main loop 
  while (counter<n) {
    // Get the start time
    auto start_time = std::chrono::steady_clock::now();

    // Simulate variable time computation
    unsigned int random_number = distribution(gen);	
    for (unsigned int i=0; i<random_number; i++)
      ;

    std::cout << "Iteration: " << counter << " of " << n << std::endl;  
    counter++;

    // Compute elapsed time and wait for the remaning duration    
    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_time = end_time - start_time;
    std::this_thread::sleep_for(loop_duration - elapsed_time);

    // Accumulate loop duration   
    auto end_time_loop = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_time_loop = end_time_loop - start_time;
    total += elapsed_time_loop;
  }

  // Compute average iteration duration (should be the same of the loop duration)
  auto avg = std::chrono::duration_cast<std::chrono::nanoseconds>(total).count()/n;
  std::cout << "Avg: " << avg << " ns" << std::endl;
  
  return 0;
}

