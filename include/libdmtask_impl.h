
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

#ifndef DMTASK_IMPL_H
#define DMTASK_IMPL_H

#include "dmtask.h"
#include <future>
#include <functional>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <utility> // For std::forward, std::move, std::decay_t
#include <memory>  // For std::unique_ptr

// Helper to deduce result type of a callable
template<typename Func, typename... Args>
using invoke_result_t = typename std::invoke_result<Func, Args...>::type;

// Forward declaration of the concrete implementation
template<typename T>
class DmTaskImpl;

// Factory function declaration
template<typename T, typename Func, typename... Args>
std::unique_ptr<DmTaskImpl<T>> make_async_call(Func&& func, Args&&... args);

// Concrete Implementation DmTaskImpl Declaration
template<typename T>
class DmTaskImpl : public IDmTask<T> {
private:
    std::future<T> std_fut;
    bool valid_ = true; 

    void ensure_valid() const;

public:
    explicit DmTaskImpl(std::future<T>&& sf);

    DmTaskImpl(DmTaskImpl&& other) noexcept;
    DmTaskImpl& operator=(DmTaskImpl&& other) noexcept;
    DmTaskImpl(const DmTaskImpl&) = delete;
    DmTaskImpl& operator=(const DmTaskImpl&) = delete;

    // .then for DmTaskImpl<NonVoid>
    template<typename Func>
    auto then(Func&& continuation) -> std::unique_ptr<DmTaskImpl<invoke_result_t<Func, T>>>;

    // .then for DmTaskImpl<Void>
    template<typename Func, typename CurrentTaskType = T, std::enable_if_t<std::is_void_v<CurrentTaskType>, int> = 0>
    auto then(Func&& continuation) -> std::unique_ptr<DmTaskImpl<invoke_result_t<Func>>>;

    T get() override;
};

// Include the template implementations file
#include "libdmtask_impl.tpp"

#endif // DMTASK_IMPL_H