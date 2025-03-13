// Copyright 2025 Darcy Best
// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// A full Moriarty program using ExampleGraph.

#include <iostream>
#include <ostream>
#include <span>

#include "docs/examples/example_graph.h"
#include "docs/examples/mexample_graph.h"
#include "src/context.h"
#include "src/moriarty.h"
#include "src/test_case.h"
#include "src/variables/minteger.h"

using ::moriarty::Between;
using ::moriarty::Exactly;
using ::moriarty::MInteger;
using ::moriarty::Moriarty;
using ::moriarty_examples::ExampleGraph;
using ::moriarty_examples::MExampleGraph;

// Exporter that prints the Graph to std::cout.
void PrintGraph(moriarty::ExportContext ctx,
                std::span<const moriarty::ConcreteTestCase>) {
  ExampleGraph G = ctx.GetValue<MExampleGraph>("G");

  // Simply print to screen
  std::cout << G.num_nodes << " " << G.edges.size() << '\n';
  for (const auto& [u, v] : G.edges) {
    std::cout << u << " " << v << '\n';
  }
}

int main() {
  Moriarty M =
      Moriarty()
          .SetName("Example Graph Codelab")
          .SetSeed("b34ibhfberogh4tjbsfg843jf1s")
          .AddVariable("N", MInteger(Between(5, 10)))
          .AddVariable("G", MExampleGraph()
                                .WithNumNodes(MInteger(Exactly("N")))
                                .WithNumEdges(MInteger(Between("N", "2 * N"))));

  // Random generation of test cases
  M.GenerateTestCases(
      [](moriarty::GenerateContext ctx) -> std::vector<moriarty::TestCase> {
        return {moriarty::TestCase()};
      });
  M.ExportTestCases(PrintGraph);
}
