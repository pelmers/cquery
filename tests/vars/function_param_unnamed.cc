void foo(int, int) {}
/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@foo#I#I#",
      "short_name": "foo",
      "detailed_name": "void foo(int, int)",
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-1:22"
    }]
}
*/
