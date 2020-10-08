// Copyright 2020 The XLS Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import xls.dslx.interpreter.tests.mod_imported
import xls.dslx.interpreter.tests.mod_imported as mi

fn main(x: u3) -> u1 {
  let lhs: u1 = mod_imported::my_lsb(x);
  let rhs: u1 = mi::my_lsb(x);
  let ehs: u1 = mi::my_lsb_uses_const(x);
  lhs || rhs || ehs
}

test main {
  assert_eq(u1:0b1, main(u3:0b001))
}
