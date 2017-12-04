void foo(int a) {
  a += 10;
}
/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@foo#I#",
      "short_name": "foo",
      "detailed_name": "void foo(int)",
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-3:2"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:var_usage_func_parameter.cc@9@F@foo#I#@a",
      "short_name": "a",
      "detailed_name": "int a",
      "definition_spelling": "1:14-1:15",
      "definition_extent": "1:10-1:15",
      "is_local": true,
      "is_macro": false,
      "uses": ["1:14-1:15", "2:3-2:4"]
    }]
}
*/
