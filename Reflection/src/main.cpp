// ==================================================================================
// C++26 Static Reflection (P2996) — Demo
//
// This program shows how to walk over the non-static data members of an
// arbitrary struct at compile time, and print each member's name and value
// at run time — without writing a single line of boilerplate reflection
// code by hand (no macros, no manual field lists).
//
// Build (GCC 16+):
//   g++ -std=c++26 -freflection main.cpp -o main
//
// Key building blocks from <meta>:
//   ^^T                                -> produces a std::meta::info reflection of T
//   std::meta::nonstatic_data_members_of(refl, access_context)
//                                      -> returns a vector<info> describing the
//                                         members visible under that access context
//   std::define_static_array(vec)      -> "materializes" that vector into a real
//                                         static array, which is required because
//                                         `template for` needs a compile-time range
//                                         with static storage, not a std::vector
//   template for (constexpr auto m : ...) -> a compile-time loop: the compiler
//                                         unrolls it, instantiating code per member
//   std::meta::identifier_of(m)        -> the member's name, as std::string_view
//   entity.[:m:]                       -> "splice": turns the reflection `m` back
//                                         into an actual member access expression
// ==================================================================================

#include <array>
#include <meta>
#include <print>
#include <span>
#include <string_view>

// A small aggregate to be reflected upon.
struct Vector3 {
    float x { 0.0f };
    float y { 0.0f };
    float z { 0.0f };
};

// The "main" entity we want to inspect. Note the mix of member kinds:
// primitives (int, bool), a nested struct (Vector3), and a raw array (stats).
// Our printing function below handles all three cases generically.
struct Enemy {
    int id { 101 };
    bool isActive { true };
    Vector3 position { 10.5f, 0.0f, -5.2f };
    float stats[3] { 100.0f, 50.0f, 15.0f }; // Health, Mana, Attack
};

// -----------------------------------------------------------------------------
// printEntityDetails
//
// Generic pretty-printer for any aggregate type T. Instead of hand-writing
// "std::println(entity.id); std::println(entity.isActive); ..." for every
// struct we care about, we ask the compiler to enumerate T's members for us
// and generate the printing code automatically.
// -----------------------------------------------------------------------------
template <typename T>
void printEntityDetails(const T& entity, std::string_view indent = "")
{
    // Step 1: reflect on the type T itself.
    // ^^T is a compile-time value (a std::meta::info) that *describes* T;
    // it carries no runtime cost.
    constexpr auto type_info = ^^T;

    // Step 2: ask for T's non-static data members.
    //
    // access_context::current() means "give me the members visible from
    // this exact point in the code" — i.e. respect normal access rules
    // (public/private/protected) as if we were writing member-access code
    // right here. Use access_context::unchecked() instead if you need to
    // bypass access control entirely (e.g. for a generic serializer).
    //
    // nonstatic_data_members_of() returns a std::vector<std::meta::info>,
    // which is perfect for compile-time computation but can't be iterated
    // by `template for` directly (that needs static storage). So we wrap it
    // in std::define_static_array to get a real compile-time array back.
    //
    // NOTE: we spell out the type (std::span<const std::meta::info>) instead
    // of using `auto`. As of GCC 16, `auto` deduces a span&& referring to a
    // lifetime-extended temporary here, which currently trips up GCC's
    // constant-expression evaluator inside `template for` (a known compiler
    // limitation, not a bug in this code). Naming the type explicitly as a
    // plain const span sidesteps that reference deduction entirely.
    static constexpr const std::span<const std::meta::info> members = std::define_static_array(
        std::meta::nonstatic_data_members_of(
            type_info,
            std::meta::access_context::current()));

    // Step 3: iterate over the members *at compile time*.
    // The compiler unrolls this loop, generating a separate block of code
    // for each member — similar to manually writing one branch per field,
    // but automatically and kept in sync if T ever changes.
    template for (constexpr auto member : members)
    {
        // The member's name, extracted as a compile-time string_view.
        constexpr std::string_view member_name = std::meta::identifier_of(member);

        // The splice operator `.[:member:]` turns the reflection `member`
        // back into a genuine member-access expression, i.e. this behaves
        // exactly like writing `entity.id`, `entity.position`, etc.,
        // just chosen for us by the compiler at compile time.
        const auto& member_value = entity.[:member:];

        using MemberType = std::remove_cvref_t<decltype(member_value)>;

        if constexpr (std::is_array_v<MemberType>) {
            // Case 1: raw C-style array (e.g. float stats[3]).
            std::print("{}{}: [", indent, member_name);
            for (const auto& elem : member_value) {
                std::print("{} ", elem);
            }
            std::println("]");
        } else if constexpr (std::is_class_v<MemberType>) {
            // Case 2: nested struct/class (e.g. Vector3 position).
            // Recurse into it, increasing the indentation for readability.
            std::println("{}{}:", indent, member_name);
            printEntityDetails(member_value, std::string(indent) + "  ");
        } else {
            // Case 3: primitive type (int, bool, float, ...).
            std::println("{}{}: {}", indent, member_name, member_value);
        }
    }
}

int main()
{
    Enemy goblin {
        .id = 42,
        .isActive = true,
        .position = { 12.0f, 1.5f, -80.0f },
        .stats = { 80.0f, 20.0f, 12.5f }
    };

    std::println("=== Inspecting Enemy Struct ===\n");
    printEntityDetails(goblin);

    return 0;
}
