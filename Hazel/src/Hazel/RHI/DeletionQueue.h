//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <set>
#include <type_traits>
#include <utility>

namespace Hazel
{
    class DeletionQueue
    {
      public:
        struct Operation
        {
            std::function<void()> func;
            uint64_t seq = 0;
            uint64_t time = 0;

            Operation() = default;

            Operation(std::function<void()> func)
                : func(std::move(func))
            {}

            template <typename F>
                requires(!std::same_as<std::remove_cvref_t<F>, Operation>
                         && !std::same_as<std::remove_cvref_t<F>, std::function<void()>>
                         && std::is_invocable_r_v<void, F>)
            Operation(F&& f)
                : func(std::forward<F>(f))
            {}

            void operator()() const
            {
                if (func) { func(); }
            }
        };

        struct OperationCompare
        {
            bool operator()(const Operation& a, const Operation& b) const
            {
                if (a.seq != b.seq) return a.seq > b.seq;
                return a.time < b.time;
            }
        };

        using OperationSet = std::priority_queue<Operation, std::vector<Operation>, OperationCompare>;

        DeletionQueue() = default;
        DeletionQueue(const DeletionQueue&) = delete;
        DeletionQueue& operator=(const DeletionQueue&) = delete;

        DeletionQueue& operator=(DeletionQueue&& queue) noexcept
        {
            if (this != &queue)
            {
                std::scoped_lock lock(m_Mutex, queue.m_Mutex);
                m_Operations = std::move(queue.m_Operations);
                m_CurrentTime = queue.m_CurrentTime;
            }
            return *this;
        }

        void Enqueue(Operation operation)
        {
            std::scoped_lock lock(m_Mutex);
            operation.time = ++m_CurrentTime;
            m_Operations.push(std::move(operation));
        }

        void Enqueue(Operation operation, uint64_t seq)
        {
            std::scoped_lock lock(m_Mutex);
            operation.time = ++m_CurrentTime;
            operation.seq = seq;
            m_Operations.push(std::move(operation));
        }

        void Flush() { Execute(ExtractAll()); }

        OperationSet ExtractAll()
        {
            std::scoped_lock lock(m_Mutex);
            OperationSet operations;
            operations.swap(m_Operations);
            m_CurrentTime = 0;
            return operations;
        }

        OperationSet ExtractSeqLessThan(uint64_t seq)
        {
            std::scoped_lock lock(m_Mutex);
            OperationSet operations;
            while (!m_Operations.empty() && m_Operations.top().seq < seq)
            {
                operations.push(m_Operations.top());
                m_Operations.pop();
            }
            return operations;
        }

        static void Execute(OperationSet operations)
        {
            while (!operations.empty())
            {
                const auto& operation = operations.top();
                operation();
                operations.pop();
            }
        }

      private:
        std::mutex m_Mutex;
        OperationSet m_Operations;
        uint64_t m_CurrentTime = 0;
    };
} // namespace Hazel