#include "static_function_in_type.h"

namespace ns {
// static
void Foo::Register(Manager* m) {
}
}

/*
OUTPUT: static_function_in_type.h
{
  "types": [{
      "id": 0,
      "usr": "c:@N@ns@S@Manager",
      "uses": ["3:7-3:14", "6:24-6:31"]
    }, {
      "id": 1,
      "usr": "c:@N@ns@S@Foo",
      "short_name": "Foo",
      "detailed_name": "ns::Foo",
      "definition_spelling": "5:8-5:11",
      "definition_extent": "5:1-7:2",
      "funcs": [0],
      "uses": ["5:8-5:11"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@N@ns@S@Foo@F@Register#*$@N@ns@S@Manager#S",
      "short_name": "Register",
      "detailed_name": "void ns::Foo::Register(ns::Manager *)",
      "declarations": [{
          "spelling": "6:15-6:23",
          "extent": "6:3-6:33",
          "content": "static void Register(Manager*)",
          "param_spellings": ["6:32-6:32"]
        }],
      "declaring_type": 1
    }]
}
OUTPUT: static_function_in_type.cc
{
  "includes": [{
      "line": 1,
      "resolved_path": "&static_function_in_type.h"
    }],
  "types": [{
      "id": 0,
      "usr": "c:@N@ns@S@Foo",
      "funcs": [0],
      "uses": ["5:6-5:9"]
    }, {
      "id": 1,
      "usr": "c:@N@ns@S@Manager",
      "instances": [0],
      "uses": ["5:20-5:27"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@N@ns@S@Foo@F@Register#*$@N@ns@S@Manager#S",
      "short_name": "Register",
      "detailed_name": "void ns::Foo::Register(ns::Manager *)",
      "definition_spelling": "5:11-5:19",
      "definition_extent": "5:1-6:2",
      "declaring_type": 0
    }],
  "vars": [{
      "id": 0,
      "usr": "c:static_function_in_type.cc@86@N@ns@S@Foo@F@Register#*$@N@ns@S@Manager#S@m",
      "short_name": "m",
      "detailed_name": "ns::Manager * m",
      "definition_spelling": "5:29-5:30",
      "definition_extent": "5:20-5:30",
      "variable_type": 1,
      "is_local": true,
      "is_macro": false,
      "uses": ["5:29-5:30"]
    }]
}
*/