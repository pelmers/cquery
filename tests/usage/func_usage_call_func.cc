void called() {}
void caller() {
  called();
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@called#",
      "short_name": "called",
      "detailed_name": "void called()",
      "definition_spelling": "1:6-1:12",
      "definition_extent": "1:1-1:17",
      "callers": ["1@3:3-3:9"]
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "detailed_name": "void caller()",
      "definition_spelling": "2:6-2:12",
      "definition_extent": "2:1-4:2",
      "callees": ["0@3:3-3:9"]
    }]
}
*/
