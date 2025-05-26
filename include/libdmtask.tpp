
#ifndef __LIBDMTASK_TPP__
#define __LIBDMTASK_TPP__

// Definitions for DmTaskImpl<T> methods
template<typename T>
void DmTaskImpl<T>::ensure_valid() const {
    if (!valid_) {
        throw std::future_error(std::future_errc::no_state);
    }
}

template<typename T>
DmTaskImpl<T>::DmTaskImpl(std::future<T>&& sf) : std_fut(std::move(sf)) {}

template<typename T>
DmTaskImpl<T>::DmTaskImpl(DmTaskImpl&& other) noexcept : std_fut(std::move(other.std_fut)), valid_(other.valid_) {
    other.valid_ = false;
}

template<typename T>
DmTaskImpl<T>& DmTaskImpl<T>::operator=(DmTaskImpl&& other) noexcept {
    if (this != &other) {
        std_fut = std::move(other.std_fut);
        valid_ = other.valid_;
        other.valid_ = false;
    }
    return *this;
}

template<typename T>
template<typename Func>
auto DmTaskImpl<T>::then(Func&& continuation) -> std::unique_ptr<DmTaskImpl<invoke_result_t<Func, T>>> {
    ensure_valid();
    // This 'then' overload is for when T is NOT void.
    // The SFINAE on the other 'then' overload handles the T = void case.
    using CurrentResultType = T; 
    using NextResultType = invoke_result_t<Func, CurrentResultType>;

    std::promise<NextResultType> prom;
    std::future<NextResultType> next_std_fut = prom.get_future();
    
    std::async(std::launch::async,
        [current_std_fut_moved = std::move(std_fut), p = std::move(prom), cont = std::forward<Func>(continuation)]() mutable {
        try {
            CurrentResultType value = current_std_fut_moved.get();
            if constexpr (std::is_void_v<NextResultType>) {
                cont(value);
                p.set_value();
            } else {
                p.set_value(cont(value));
            }
        } catch (...) {
            try {
                p.set_exception(std::current_exception());
            } catch (...) { /* Should not happen with std::promise */ }
        }
    });
    
    valid_ = false; 
    return std::make_unique<DmTaskImpl<NextResultType>>(std::move(next_std_fut));
}

template<typename T>
template<typename Func, typename CurrentTaskType, std::enable_if_t<std::is_void_v<CurrentTaskType>, int>>
auto DmTaskImpl<T>::then(Func&& continuation) -> std::unique_ptr<DmTaskImpl<invoke_result_t<Func>>> {
    ensure_valid();
    using NextResultType = invoke_result_t<Func>; // Func takes no arguments if CurrentTaskType is void

    std::promise<NextResultType> prom;
    std::future<NextResultType> next_std_fut = prom.get_future();

    std::async(std::launch::async,
        [current_std_fut_moved = std::move(std_fut), p = std::move(prom), cont = std::forward<Func>(continuation)]() mutable {
        try {
            current_std_fut_moved.get(); // Wait for void future, call its get()
            if constexpr (std::is_void_v<NextResultType>) {
                cont(); // Call continuation that takes void
                p.set_value();
            } else {
                p.set_value(cont()); // Call continuation that takes void and returns a value
            }
        } catch (...) {
            try {
                p.set_exception(std::current_exception());
            } catch (...) { /* Should not happen */ }
        }
    });
    valid_ = false; 
    return std::make_unique<DmTaskImpl<NextResultType>>(std::move(next_std_fut));
}

// MODIFIED get() method
template<typename T>
T DmTaskImpl<T>::get() { // 'override' keyword is in the declaration in .h
    ensure_valid();
    if constexpr (std::is_void_v<T>) {
        std_fut.get(); // For std::future<void>, just call get() to wait/rethrow, it returns void
        valid_ = false; 
        return; // Explicitly return for void
    } else {
        T value = std_fut.get(); // For non-void T, get value
        valid_ = false; 
        return value;
    }
}

// Factory function definition
template<typename T, typename Func, typename... Args>
std::unique_ptr<DmTaskImpl<T>> make_async_call(Func&& func, Args&&... args) {
    std::promise<T> prom;
    std::future<T> std_fut = prom.get_future();

    std::async(std::launch::async,
        [p = std::move(prom), f = std::decay_t<Func>(std::forward<Func>(func)), ...largs = std::decay_t<Args>(std::forward<Args>(args))]() mutable {
            try {
                if constexpr (std::is_void_v<T>) {
                    f(largs...);
                    p.set_value();
                } else {
                    p.set_value(f(largs...));
                }
            } catch (...) {
                try {
                    p.set_exception(std::current_exception());
                } catch (...) { /* Should not happen */ }
            }
        });

    return std::make_unique<DmTaskImpl<T>>(std::move(std_fut));
}

#endif // __LIBDMTASK_TPP__