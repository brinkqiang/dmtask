#include "libdmtask.h" // This will bring in IDmTask, DmTaskImpl, and make_async_call
#include <iostream>
#include <string>
#include <vector>   // Not strictly needed by this main, but often useful
#include <stdexcept> // For std::runtime_error
#include <chrono>    // For std::chrono::milliseconds, std::this_thread::sleep_for


// Example asynchronous functions (can be in their own file or here for simplicity)
int initial_int_task(int id, const std::string& name) {
    std::cout << "Thread " << std::this_thread::get_id() << ": initial_int_task(" << id << ", \"" << name << "\") started." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (id == 0) {
        std::cout << "Thread " << std::this_thread::get_id() << ": initial_int_task throwing exception." << std::endl;
        throw std::runtime_error("ID cannot be zero in initial_int_task");
    }
    int result = id * 10;
    std::cout << "Thread " << std::this_thread::get_id() << ": initial_int_task finished, returning " << result << "." << std::endl;
    return result;
}

void initial_void_task(const std::string& message) {
    std::cout << "Thread " << std::this_thread::get_id() << ": initial_void_task(\"" << message << "\") started." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "Thread " << std::this_thread::get_id() << ": initial_void_task finished." << std::endl;
}

int main() {
    std::cout << "Main thread: " << std::this_thread::get_id() << std::endl;

    std::cout << "\n--- Scenario 1: Successful chain (int -> int -> string) ---" << std::endl;
    auto successful_future_impl = make_async_call<int>(initial_int_task, 5, "Task_A");

    auto final_string_future_impl = successful_future_impl
        ->then([](int value) {
            std::cout << "Thread " << std::this_thread::get_id() << ": First .then() processing: " << value << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return value * 2;
        })
        ->then([](int value) {
            std::cout << "Thread " << std::this_thread::get_id() << ": Second .then() processing: " << value << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return std::to_string(value) + " as string";
        });
    
    std::unique_ptr<IDmTask<std::string>> final_string_task_interface = std::move(final_string_future_impl);

    try {
        std::string result = final_string_task_interface->get();
        std::cout << "Main thread: Final result from Scenario 1: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Main thread: Exception in Scenario 1: " << e.what() << std::endl;
    } catch (const std::future_error& fe) {
        std::cout << "Main thread: Future error in Scenario 1: " << fe.what() << " code: " << fe.code() << std::endl;
    }

    std::cout << "\n--- Scenario 2: Chain with exception in initial task ---" << std::endl;
    auto exceptional_future_impl = make_async_call<int>(initial_int_task, 0, "Task_B_Fails"); 

    auto future_after_exception_impl = exceptional_future_impl
        ->then([](int value) {
            std::cout << "Thread " << std::this_thread::get_id() << ": (Scenario 2) First .then() - UNREACHABLE, received: " << value << std::endl;
            return value * 2;
        })
        ->then([](int value) {
            std::cout << "Thread " << std::this_thread::get_id() << ": (Scenario 2) Second .then() - UNREACHABLE, received: " << value << std::endl;
            return std::to_string(value);
        });
    
    std::unique_ptr<IDmTask<std::string>> future_after_exception_interface = std::move(future_after_exception_impl);

    try {
        std::string result = future_after_exception_interface->get(); 
        std::cout << "Main thread: Final result from Scenario 2 (UNREACHABLE): " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Main thread: Caught expected exception in Scenario 2: " << e.what() << std::endl;
    } catch (const std::future_error& fe) {
        std::cout << "Main thread: Future error in Scenario 2: " << fe.what() << " code: " << fe.code() << std::endl;
    }

    std::cout << "\n--- Scenario 3: Chain starting with void, then returning value ---" << std::endl;
    auto void_future_impl = make_async_call<void>(initial_void_task, "Hello Void Task");
    
    auto future_from_void_impl = void_future_impl
        ->then([]() { 
            std::cout << "Thread " << std::this_thread::get_id() << ": (Scenario 3) .then() after void_future." << std::endl;
            return 123; 
        })
        ->then([](double value) {
            std::cout << "Thread " << std::this_thread::get_id() << ": (Scenario 3) .then() received " << value << " from void chain." << std::endl;
            return "Final: " + std::to_string(value);
        });
    std::unique_ptr<IDmTask<std::string>> future_from_void_interface = std::move(future_from_void_impl);
    
    try {
        std::string result = future_from_void_interface->get();
        std::cout << "Main thread: Final result from Scenario 3: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Main thread: Exception in Scenario 3: " << e.what() << std::endl;
    } catch (const std::future_error& fe) {
        std::cout << "Main thread: Future error in Scenario 3: " << fe.what() << " code: " << fe.code() << std::endl;
    }

    std::cout << "\n--- Scenario 4: Chain with a .then() that returns void ---" << std::endl;
    auto int_future_for_void_then_impl = make_async_call<int>(initial_int_task, 7, "Task_C");

    auto future_after_void_then_impl = int_future_for_void_then_impl
        ->then([](int value) { 
            std::cout << "Thread " << std::this_thread::get_id() << ": (Scenario 4) .then() received " << value << ", performing void action." << std::endl;
        })
        ->then([]() { 
            std::cout << "Thread " << std::this_thread::get_id() << ": (Scenario 4) .then() after void continuation." << std::endl;
            return std::string("Completed after void middle step");
        });
    std::unique_ptr<IDmTask<std::string>> future_after_void_then_interface = std::move(future_after_void_then_impl);
    
    try {
        std::string result = future_after_void_then_interface->get();
        std::cout << "Main thread: Final result from Scenario 4: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Main thread: Exception in Scenario 4: " << e.what() << std::endl;
    } catch (const std::future_error& fe) {
        std::cout << "Main thread: Future error in Scenario 4: " << fe.what() << " code: " << fe.code() << std::endl;
    }

    std::cout << "\nMain thread: All scenarios complete." << std::endl;
    return 0;
}