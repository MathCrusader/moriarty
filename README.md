# Moriarty

<p align="center">
  <img width="250px" src="docs/logo.png" alt="Moriarty Logo">
</p>
<p align="center">
  <em>Create robust data generators and data validators with little-to-no<br>
  boilerplate code based on just a description of the data.</em>
</p>

## What is Moriarty?

Moriarty is a tool for describing, generating, and verifying data through our
expressive syntax.

```c++
// Example generator
int main() {
  Problem p(
    Title("The Famous Problem"),
    Seed("longrandomuuidhere"),
    Variables(
      Var("N", MInteger(Between(1, 100))),
      Var("A", MArray<MInteger>(Length("N"), Elements<MInteger>(Between(31, 35)))),
      Var("S", MStringMString(Alphabet("abc"), Length("2 * N - 1")))
      Var("X", MInteger())
    ),
    InputFormat(SimpleIO().AddLine("N", "S").AddLine("A")),
    OutputFormat(SimpleIO().AddLine("X"))
  );

  Generate(p)
    .Using("CornerCaseGenerator", CornerCaseGenerator)
    .Using("SmallExamples", SmallCases, {.num_calls = 10})
    .Using("PureRandom", Random, {.num_calls = 20})
    .WriteInputUsing({.ostream = std::cout})
    .WriteOutputUsing({.ostream = std::cerr})
    .Run();
}
```

Example generated input from example (where N = 4):

```
4 abcdefg
33 35 33 32
```

## The Basics

Moriarty is centred around the concept of an `MVariable`. An `MVariable` is a
collection of constraints on a variable. For example, an `MInteger` constrains
an integer value (e.g., "between 1 and 10" or "is a prime number"). Variables
can interact with one another. For example, the length of an `MString` can be
`3 * N`, where `N` is an `MInteger`.

## Available MVariables

| MVariable       | Underlying Type               | Example Constraints                                                |
| --------------- | ----------------------------- | ------------------------------------------------------------------ |
| `MInteger`      | `int64_t`                     | `Between(1, "N")`, `Exactly(5)`, `Mod(2, 0)`                       |
| `MReal`         | `double`                      | `Between(0, Real("5.28"))`, `AtLeast(Real("0.0001"))`, `AtMost(1)` |
| `MString`       | `std::string`                 | `Length(10)`, `Alphabet("abc")`, `DistinctCharacters()`            |
| `MArray<T>`     | `std::vector< ... >`          | `Length("N")`, `Elements<T>(...)`, `DistinctElements()`            |
| `MTuple<...>`   | `std::tuple< ... >`           | `Element<0, MInteger>(Between(1, 10))`                             |
| `MGraph<E, N>`  | `Graph<EdgeLabel, NodeLabel>` | `NumNodes(100)`, `NumEdges(200)`, `Connected()`, `Simple()`        |
