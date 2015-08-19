%module Ldimjc
%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"
%include "stdint.i"


 %{
using std::string;
#include "DimJobInterface.h"
 %}

%include "DimJobInterface.h"
