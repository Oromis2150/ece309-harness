#include "core/conversation.h"
#include <stdexcept>
#include <utility>

// Default Constructor and Destructor
Conversation::Conversation() {}

Conversation::~Conversation()
{
    delete[] data_;
}

// Copy Constructor and Copy Assignment Operator
Conversation::Conversation(const Conversation &other)
{
    if (other.data_ != nullptr)
    {
        data_ = new Message[other.capacity_];
        try {
            for (std::size_t i = 0; i < other.size_; ++i) {
                data_[i] = other.data_[i];
            }
        }
        catch (...) {
            delete[] data_;
            data_ = nullptr;
            throw;
        }
    }
    else
    {
        data_ = nullptr;
    }
    size_ = other.size_;
    capacity_ = other.capacity_;
}
Conversation &Conversation::operator=(const Conversation &other)
{
    Conversation temp(other);

    swap(temp);

    return *this;
}

// Move Constructor and Move Assignment Operator
Conversation::Conversation(Conversation &&other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_)
{
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}
Conversation &Conversation::operator=(Conversation &&other) noexcept
{
    if (this != &other)
    {
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;

        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

// Append Message to Conversation
void Conversation::append(Message m) {
    std::size_t new_capacity;

    if (size_ == capacity_) {
        (new_capacity = capacity_ == 0 ? 1 : capacity_ * 2);
        Message *new_data = new Message[new_capacity];

        for (std::size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move(data_[i]);
        }

        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
    }
    data_[size_] = std::move(m);
    ++size_;
}

// Number of messages currently stored.
std::size_t Conversation::size() const noexcept { return size_; }
// Bounds-checked access. Decide what happens on i >= size() (throw,
// assert, whatever you pick) and test that behavior explicitly.
const Message& Conversation::at(std::size_t i) const {
    if (i >= size_) {
        throw std::out_of_range("Conversation::at: index out of range");
    }
    return data_[i];
}
// Range-for iteration, oldest message first. begin() == end() when
// size() == 0.
const Message *Conversation::begin() const noexcept { return data_; }
const Message *Conversation::end() const noexcept { return data_ + size_; }

void Conversation::swap(Conversation& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
}