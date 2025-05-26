
// Copyright (c) 2018 brinkqiang (brink.qiang@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// This file (dmtask_impl.tpp) is included by dmtask_impl.h
// It contains the definitions of template members and functions.

#ifndef DMTASK_IMPL_TPP
#define DMTASK_IMPL_TPP

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
    using CurrentResultType = T; // Should not be void here due to SFINAE on the other overload
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
    using NextResultType = invoke_result_t<Func>;

    std::promise<NextResultType> prom;
    std::future<NextResultType> next_std_fut = prom.get_future();

    std::async(std::launch::async,
        [current_std_fut_moved = std::move(std_fut), p = std::move(prom), cont = std::forward<Func>(continuation)]() mutable {
        try {
            current_std_fut_moved.get(); 
            if constexpr (std::is_void_v<NextResultType>) {
                cont();
                p.set_value();
            } else {
                p.set_value(cont());
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

template<typename T>
T DmTaskImpl<T>::get() { // 'override' keyword is in the declaration in .h
    ensure_valid();
    T value = std_fut.get(); 
    valid_ = false; 
    return value;
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

#endif // DMTASK_IMPL_TPP