%module module_person

%include <std_string.i>   // std::string <-> string (Perl)

%{
#include "person.hpp"
%}

%include "person.hpp"
