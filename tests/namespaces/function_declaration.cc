namespace hello {
void foo(int a, int b);
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@N@hello@F@foo#I#I#",
      "short_name": "foo",
      "detailed_name": "void hello::foo(int, int)",
      "declarations": [{
          "spelling": "2:6-2:9",
          "extent": "2:1-2:23",
          "content": "void foo(int a, int b)",
          "param_spellings": ["2:14-2:15", "2:21-2:22"]
        }]
    }]
}
*/
