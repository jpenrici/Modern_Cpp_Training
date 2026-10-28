local person = require("module_person")

print("Testing module_person (" .. _VERSION .. ") ...")

-- struct
local address = person.Place()
address.city = "Teresopolis"
address.country = "RJ"
address.number = 42
print("Struct : " .. address:summary())

-- class
local p1 = person.Person("Juan", 30)
p1:set_address(address)
p1:birthday()
print("Class  : " .. p1:greetings())

-- std::string
p1:set_name("Juan Pablo")
print(string.format("Nome   : %s (%d anos)", p1:name(), p1:age()))

-- internal reference
local ref = p1:address()
print("City : " .. ref.city)

-- verificacoes
assert(p1:age() == 31)
assert(p1:name() == "Juan Pablo")
assert(ref.city == "Teresopolis" and ref.number == 42)
print("OK: All checks passed.")
