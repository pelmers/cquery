struct Type;

Type* foo();
Type* foo();
Type* foo() {}

class Foo {
  Type* Get(int);
  void Empty();
};

Type* Foo::Get(int) {}
void Foo::Empty() {}

extern const Type& external();

static Type* bar();
static Type* bar() {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Type",
      "uses": ["1:8-1:12", "3:1-3:5", "4:1-4:5", "5:1-5:5", "8:3-8:7", "12:1-12:5", "15:14-15:18", "17:8-17:12", "18:8-18:12"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "7:7-7:10",
      "definition_extent": "7:1-10:2",
      "funcs": [1, 2],
      "uses": ["7:7-7:10", "12:7-12:10", "13:6-13:9"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "Type *foo()",
      "declarations": [{
          "spelling": "3:7-3:10",
          "extent": "3:1-3:12",
          "content": "Type* foo()"
        }, {
          "spelling": "4:7-4:10",
          "extent": "4:1-4:12",
          "content": "Type* foo()"
        }],
      "definition_spelling": "5:7-5:10",
      "definition_extent": "5:1-5:15"
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@S@Foo@F@Get#I#",
      "short_name": "Get",
      "detailed_name": "Type *Foo::Get(int)",
      "declarations": [{
          "spelling": "8:9-8:12",
          "extent": "8:3-8:17",
          "content": "Type* Get(int)",
          "param_spellings": ["8:16-8:16"]
        }],
      "definition_spelling": "12:12-12:15",
      "definition_extent": "12:1-12:23",
      "declaring_type": 1
    }, {
      "id": 2,
      "is_operator": false,
      "usr": "c:@S@Foo@F@Empty#",
      "short_name": "Empty",
      "detailed_name": "void Foo::Empty()",
      "declarations": [{
          "spelling": "9:8-9:13",
          "extent": "9:3-9:15",
          "content": "void Empty()"
        }],
      "definition_spelling": "13:11-13:16",
      "definition_extent": "13:1-13:21",
      "declaring_type": 1
    }, {
      "id": 3,
      "is_operator": false,
      "usr": "c:@F@external#",
      "short_name": "external",
      "detailed_name": "const Type &external()",
      "declarations": [{
          "spelling": "15:20-15:28",
          "extent": "15:1-15:30",
          "content": "extern const Type& external()"
        }]
    }, {
      "id": 4,
      "is_operator": false,
      "usr": "c:type_usage_on_return_type.cc@F@bar#",
      "short_name": "bar",
      "detailed_name": "Type *bar()",
      "declarations": [{
          "spelling": "17:14-17:17",
          "extent": "17:1-17:19",
          "content": "static Type* bar()"
        }],
      "definition_spelling": "18:14-18:17",
      "definition_extent": "18:1-18:22"
    }]
}
*/
