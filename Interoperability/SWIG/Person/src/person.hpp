#pragma once

#include <format>
#include <string>
#include <utility>

struct Place {
    std::string city;
    std::string country;
    int number { 0 };

    auto summary() const -> std::string
    {
        return std::format("{}/{}, n. {}", city, country, number);
    }
};

class Person {
public:
    Person(std::string name, int age)
        : name_ { std::move(name) }
        , age_ { age }
    {
    }

    // Getters
    auto name() const -> const std::string& { return name_; }
    auto age() const -> int { return age_; }
    auto address() const -> const Place& { return address_; }

    // Setters
    auto set_name(std::string name) -> void { name_ = std::move(name); }
    auto set_address(Place address) -> void { address_ = std::move(address); }

    // Behavior
    auto birthday() -> void { ++age_; }

    auto greetings() const -> std::string
    {
        return std::format("Hello, I’m {}, I’m {} years old, and I live in {}.",
            name_, age_, address_.summary());
    }

private:
    std::string name_;
    int age_ { 0 };
    Place address_ {};
};
