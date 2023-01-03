//
// USB Power Delivery Sink Using FUSB302B
// Copyright (c) 2020 Manuel Bleichenbacher
//
// Licensed under MIT License
// https://opensource.org/licenses/MIT
//
// FIFO queue
//

#pragma once

#include <utility>

namespace usb_pd {

/**
 * Queue for elements of type T and a maximum of N items.
 *
 * The queue operates according to FIFO: first-in, first-out.
 *
 * The queue is multi-threading and interrupt safe if it is
 * used by a single reader and a single writer.
 */
template <class T, int N> struct queue {
  private:
    /// Allocation size
    static constexpr int BUF_SIZE = N + 1;

    // 0 <= head < BUF_SIZE
    // 0 <= tail < BUF_SIZE
    // head == tail: buffer is empty
    // Therefore, the buffer must never be filled completely.
    volatile int buf_head; // updated when adding data
    volatile int buf_tail; // updated when removing data

    T buffer[BUF_SIZE];

  public:
    /// Constructs a new queue instance
    queue();

    /// Returns the number of items that could be added to queue
    int avail_items();

    /// Returs the number of items in the queue
    int num_items();

    /// Adds item to queue
    void add_item(T& item);

    /// Adds item to queue
    void add_item(T&& item);

    /// Removes item from queue
    T pop_item();

    /// Removes all items
    void clear();
};

template <class T, int N> queue<T, N>::queue() : buf_head(0), buf_tail(0) {}

template <class T, int N> int queue<T, N>::avail_items() {
    int head = buf_head;
    int tail = buf_tail;

    if (head >= tail) {
        return BUF_SIZE - (head - tail) - 1;
    } else {
        return tail - head - 1;
    }
}

template <class T, int N> int queue<T, N>::num_items() {
    int head = buf_head;
    int tail = buf_tail;

    if (head >= tail) {
        return head - tail;
    } else {
        return BUF_SIZE - (tail - head);
    }
}

template <class T, int N> void queue<T, N>::add_item(T&& item) {
    int head = buf_head;

    int new_head = head + 1;
    if (new_head >= BUF_SIZE)
        new_head = 0;

    if (new_head == buf_tail)
        return; // queue full

    buffer[head] = item;
    buf_head = new_head;
}

template <class T, int N> void queue<T, N>::add_item(T& item) {
    int head = buf_head;

    int new_head = head + 1;
    if (new_head >= BUF_SIZE)
        new_head = 0;

    if (new_head == buf_tail)
        return; // queue is full

    buffer[head] = item;
    buf_head = new_head;
}

template <class T, int N> T queue<T, N>::pop_item() {
    int tail = buf_tail;
    if (tail == buf_head)
        return T(); // queue is empty

    int new_tail = tail + 1;
    if (new_tail >= BUF_SIZE)
        new_tail = 0;

    buf_tail = new_tail;
    return std::move(buffer[tail]);
}

template <class T, int N> void queue<T, N>::clear() {
    buf_head = 0;
    buf_tail = 0;
}

} // namespace usb_pd
