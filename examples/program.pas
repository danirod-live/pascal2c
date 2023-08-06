program helloworld;

const
  IDENT = 12345;

var
  a, b : integer;
  res : integer;

begin;
  writeln(IDENT);
  a := 4;
  b := 5;
  res := a + b;
  res := a - b;
  res := a * b;
  res := a / b;
  res := a div b;
  res := a mod b;
  writeln('hola ''mundo''');
  writeln('hola ''mundo'' como '#5a1'stamos');
  writeln('hola'#13#10'mundo'#10);
  writeln('hola mundo');
  writeln(res);
end.

