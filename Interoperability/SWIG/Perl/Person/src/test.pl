#!/usr/bin/perl

use strict;
use warnings;

use FindBin;
use lib "$FindBin::Bin/../lib";
use module_person;

print "Testing module_person ...\n";

# --- struct: fields accessible like hash keys (or via swig_<field>_get/set)
my $address = module_person::Place->new();
$address->{city}    = "Teresopolis";
$address->{country} = "RJ";
$address->{number}  = 42;
print "Struct : ", $address->summary(), "\n";

# --- class: constructor, setters, and methods
my $p1 = module_person::Person->new( "Juan", 30 );
$p1->set_address($address);
$p1->birthday();
print "Class  : ", $p1->greetings(), "\n";

# --- std::string
$p1->set_name("Juan Pablo");
print "Name   : ", $p1->name(), " (", $p1->age(), " ages)\n";

# --- internal reference: address() returns a pointer to the member of $p1
my $ref = $p1->address();
print "City : ", $ref->{city}, "\n";
