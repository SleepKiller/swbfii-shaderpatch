#pragma once

#include <algorithm>
#include <atomic>
#include <exception>
#include <execution>
#include <functional>
#include <iterator>
#include <type_traits>

namespace sp {

template<class ExecutionPolicy, class ForwardIterable, class Function>
void for_each(ExecutionPolicy&& policy, ForwardIterable& iterable, Function&& func)
{
   std::for_each(std::forward<ExecutionPolicy>(policy), std::begin(iterable),
                 std::end(iterable), std::forward<Function>(func));
}

template<class ExecutionPolicy, class ForwardIterable, class Function>
void for_each_exception_capable(ExecutionPolicy&& policy,
                                ForwardIterable&& iterable, Function&& func)
{
   std::atomic_bool exception_occured{false};
   std::exception_ptr exception_ptr;

   for_each(std::forward<ExecutionPolicy>(policy),
            std::forward<ForwardIterable>(iterable),
            [&func, &exception_occured, &exception_ptr](auto&& v) {
               try {
                  if (exception_occured.load()) return;

                  std::invoke(std::forward<Function>(func),
                              std::forward<decltype(v)>(v));
               }
               catch (...) {
                  if (exception_occured.exchange(true)) return;

                  exception_ptr = std::current_exception();
               }
            });

   if (exception_ptr) std::rethrow_exception(exception_ptr);
}
}
