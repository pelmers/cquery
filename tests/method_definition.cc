class Foo {
  void foo() const;
};

void Foo::foo() const {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-3:2",
      "funcs": [0],
      "uses": ["1:7-1:10", "5:6-5:9"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Foo@F@foo#1",
      "short_name": "foo",
      "detailed_name": "void Foo::foo() const",
      "declarations": [{
          "spelling": "2:8-2:11",
          "extent": "2:3-2:19",
          "content": "void foo() const"
        }],
      "definition_spelling": "5:11-5:14",
      "definition_extent": "5:1-5:25",
      "declaring_type": 0
    }]
}
*/
