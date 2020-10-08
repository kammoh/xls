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

// options: {"input_is_dslx": true, "convert_to_ir": true, "optimize_ir": true, "codegen": true, "codegen_args": ["--generator=pipeline", "--pipeline_stages=3"], "simulate": false, "simulator": null}
// args: bits[33]:0x8000; bits[22]:0x800; bits[59]:0x400
// args: bits[33]:0x1d95e7e90; bits[22]:0x2177ec; bits[59]:0x800000000000
// args: bits[33]:0xffffffff; bits[22]:0xaaac0; bits[59]:0x555555555555555
// args: bits[33]:0x10; bits[22]:0x400; bits[59]:0x4000000
// args: bits[33]:0x2000000; bits[22]:0x33ef3d; bits[59]:0x8000000000
// args: bits[33]:0x13e092cf6; bits[22]:0x1; bits[59]:0x40000
// args: bits[33]:0x8456d6bc; bits[22]:0x155555; bits[59]:0xb03006cb81d1ff
// args: bits[33]:0x1000; bits[22]:0x10000; bits[59]:0x2
// args: bits[33]:0x9cf915a6; bits[22]:0x1; bits[59]:0x10000000
// args: bits[33]:0x400000; bits[22]:0x40; bits[59]:0x8000000
// args: bits[33]:0x80000; bits[22]:0x1000; bits[59]:0x40000000000
// args: bits[33]:0x4; bits[22]:0x8000; bits[59]:0x8000000000
// args: bits[33]:0x155555555; bits[22]:0x40000; bits[59]:0x20000000000000
// args: bits[33]:0x1000; bits[22]:0x10; bits[59]:0x4
// args: bits[33]:0x1965e432a; bits[22]:0x40000; bits[59]:0x20000000000
// args: bits[33]:0x4000; bits[22]:0x4; bits[59]:0x8
// args: bits[33]:0xaaaaaaaa; bits[22]:0x0; bits[59]:0x400000000000
// args: bits[33]:0x2; bits[22]:0x2; bits[59]:0x200000000000000
// args: bits[33]:0x200; bits[22]:0x4000; bits[59]:0x8
// args: bits[33]:0x400000; bits[22]:0x10000; bits[59]:0x4000000000000
// args: bits[33]:0x80000000; bits[22]:0x80000; bits[59]:0x2000000000
// args: bits[33]:0x10000000; bits[22]:0x3c3833; bits[59]:0x284818ec89a0564
// args: bits[33]:0x20000000; bits[22]:0x80; bits[59]:0x200000000000
// args: bits[33]:0x400000; bits[22]:0x10000; bits[59]:0x40000
// args: bits[33]:0x25568f21; bits[22]:0x2aaaaa; bits[59]:0x1000000000
// args: bits[33]:0x20000; bits[22]:0x8; bits[59]:0x400000
// args: bits[33]:0x1; bits[22]:0x40; bits[59]:0x0
// args: bits[33]:0x0; bits[22]:0x40000; bits[59]:0x1000
// args: bits[33]:0x800000; bits[22]:0x2; bits[59]:0x1fa5a9faec43bd
// args: bits[33]:0x20; bits[22]:0x400; bits[59]:0x4000000000
// args: bits[33]:0x20000000; bits[22]:0x1000; bits[59]:0x2000000000
// args: bits[33]:0x65e9900c; bits[22]:0x8; bits[59]:0x4000000
// args: bits[33]:0x1; bits[22]:0x200000; bits[59]:0x8000
// args: bits[33]:0x4; bits[22]:0x800; bits[59]:0x1000000000
// args: bits[33]:0x40000000; bits[22]:0x4000; bits[59]:0x2000000000000
// args: bits[33]:0x100; bits[22]:0x80; bits[59]:0x8000000000
// args: bits[33]:0x8000000; bits[22]:0x1000; bits[59]:0x1
// args: bits[33]:0x80000000; bits[22]:0x40; bits[59]:0x4000
// args: bits[33]:0x20; bits[22]:0x4000; bits[59]:0x2000000000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x40000; bits[59]:0x800000000000
// args: bits[33]:0x8000; bits[22]:0x4; bits[59]:0x400000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x1; bits[59]:0x100
// args: bits[33]:0x1007a636e; bits[22]:0x12fd97; bits[59]:0x100000000000000
// args: bits[33]:0x0; bits[22]:0x3fffff; bits[59]:0x10000000000000
// args: bits[33]:0x10000000; bits[22]:0x400; bits[59]:0x40
// args: bits[33]:0x4000; bits[22]:0x155555; bits[59]:0x40000000000000
// args: bits[33]:0x20000000; bits[22]:0x20; bits[59]:0x800000000000
// args: bits[33]:0x2af5a0eb; bits[22]:0x80; bits[59]:0x400
// args: bits[33]:0x200; bits[22]:0x200; bits[59]:0x80000
// args: bits[33]:0x400000; bits[22]:0x155555; bits[59]:0x61e022dafca9c40
// args: bits[33]:0x200; bits[22]:0x2000; bits[59]:0x200000000000
// args: bits[33]:0x200; bits[22]:0x40000; bits[59]:0x6c758b87806576f
// args: bits[33]:0x10000000; bits[22]:0x155555; bits[59]:0x1000000000000
// args: bits[33]:0x100; bits[22]:0x1; bits[59]:0x20000000000
// args: bits[33]:0x80000000; bits[22]:0x40000; bits[59]:0x2aaaaaaaaaaaaaa
// args: bits[33]:0x0; bits[22]:0x2f551d; bits[59]:0x10000000000000
// args: bits[33]:0x10; bits[22]:0x40000; bits[59]:0x2
// args: bits[33]:0x100000000; bits[22]:0x100000; bits[59]:0x2000
// args: bits[33]:0x40; bits[22]:0x400; bits[59]:0x80
// args: bits[33]:0xffffffff; bits[22]:0x1fffff; bits[59]:0x1
// args: bits[33]:0x1230910f9; bits[22]:0x100; bits[59]:0x4000000000
// args: bits[33]:0x100000; bits[22]:0x40; bits[59]:0x1000
// args: bits[33]:0x2000000; bits[22]:0x1000; bits[59]:0x1aac6b1cf0ae16e
// args: bits[33]:0x10000; bits[22]:0x2aaaaa; bits[59]:0x10000000000000
// args: bits[33]:0x1a6714e92; bits[22]:0x8; bits[59]:0x400000000000
// args: bits[33]:0x1; bits[22]:0x2129c8; bits[59]:0x8000000000000
// args: bits[33]:0xffffffff; bits[22]:0x2; bits[59]:0x80000000000
// args: bits[33]:0x40000; bits[22]:0x1fffff; bits[59]:0x8000
// args: bits[33]:0x1ffffffff; bits[22]:0x200000; bits[59]:0x40000
// args: bits[33]:0x10000; bits[22]:0x1000; bits[59]:0x800000000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x2; bits[59]:0x691f4ef46881f35
// args: bits[33]:0x40; bits[22]:0x2000; bits[59]:0x800000
// args: bits[33]:0x4; bits[22]:0x400; bits[59]:0x800000000000
// args: bits[33]:0x200; bits[22]:0x800; bits[59]:0x20000000000
// args: bits[33]:0x800000; bits[22]:0x2000; bits[59]:0x1000
// args: bits[33]:0x100000; bits[22]:0x20000; bits[59]:0x80000
// args: bits[33]:0x2; bits[22]:0x80; bits[59]:0x4000000
// args: bits[33]:0x2000; bits[22]:0x8; bits[59]:0x400000000000000
// args: bits[33]:0x20; bits[22]:0x2; bits[59]:0x162b007d69b5600
// args: bits[33]:0x20000; bits[22]:0x200000; bits[59]:0x4000000000000
// args: bits[33]:0x80000; bits[22]:0x149fb0; bits[59]:0x3ffffffffffffff
// args: bits[33]:0x2000; bits[22]:0x40; bits[59]:0x10000000
// args: bits[33]:0x4000000; bits[22]:0x80000; bits[59]:0x3f3396b9ab371c
// args: bits[33]:0x10; bits[22]:0x8000; bits[59]:0x479b7e5945006a
// args: bits[33]:0x958f11ac; bits[22]:0x3cc12a; bits[59]:0x8000
// args: bits[33]:0x1000000; bits[22]:0x3fffff; bits[59]:0x40000
// args: bits[33]:0x20; bits[22]:0x80; bits[59]:0x472493a248291db
// args: bits[33]:0x100000; bits[22]:0x2000; bits[59]:0x1000000000
// args: bits[33]:0x2; bits[22]:0x10000; bits[59]:0x3a9f98a99d4b19e
// args: bits[33]:0x100; bits[22]:0x200; bits[59]:0x10
// args: bits[33]:0x80000000; bits[22]:0x1; bits[59]:0x10
// args: bits[33]:0xaaaaaaaa; bits[22]:0x10; bits[59]:0x2000000
// args: bits[33]:0x1; bits[22]:0x0; bits[59]:0x8000000000
// args: bits[33]:0xdcd1faf1; bits[22]:0x2; bits[59]:0x1000
// args: bits[33]:0x2; bits[22]:0x40000; bits[59]:0x80000000000000
// args: bits[33]:0x10; bits[22]:0x8000; bits[59]:0x80000000
// args: bits[33]:0x200000; bits[22]:0x800; bits[59]:0x800000000000
// args: bits[33]:0x80000000; bits[22]:0x251411; bits[59]:0x20000000
// args: bits[33]:0x10000; bits[22]:0xc253b; bits[59]:0x3ffffffffffffff
// args: bits[33]:0x8000000; bits[22]:0x1fffff; bits[59]:0x8000000000
// args: bits[33]:0x1ffffffff; bits[22]:0x20000; bits[59]:0x10000000000000
// args: bits[33]:0x10000000; bits[22]:0x40000; bits[59]:0x40
// args: bits[33]:0x400; bits[22]:0x80000; bits[59]:0x80000000
// args: bits[33]:0x1f8f64179; bits[22]:0x800; bits[59]:0x2000000000
// args: bits[33]:0x4000; bits[22]:0x1fffff; bits[59]:0x8000
// args: bits[33]:0x80000000; bits[22]:0x4000; bits[59]:0x7c74a7ddaba5c76
// args: bits[33]:0x1799044a1; bits[22]:0x40; bits[59]:0x10000000000
// args: bits[33]:0x155555555; bits[22]:0x2aaaaa; bits[59]:0x7ffffffffffffff
// args: bits[33]:0x4; bits[22]:0x2; bits[59]:0x400000000000000
// args: bits[33]:0xc756dadb; bits[22]:0x1000; bits[59]:0x1de32887e171afe
// args: bits[33]:0xffffffff; bits[22]:0x100000; bits[59]:0x8000000000000
// args: bits[33]:0x80; bits[22]:0x1000; bits[59]:0x20
// args: bits[33]:0x20000000; bits[22]:0x4000; bits[59]:0x100000000000
// args: bits[33]:0x2; bits[22]:0x40; bits[59]:0x40000000000000
// args: bits[33]:0x2; bits[22]:0x1; bits[59]:0x7ce137795888394
// args: bits[33]:0x400000; bits[22]:0xb112c; bits[59]:0x800000
// args: bits[33]:0x10; bits[22]:0x2; bits[59]:0x800000000000
// args: bits[33]:0x800; bits[22]:0x3fffff; bits[59]:0x20000000000
// args: bits[33]:0x1000000; bits[22]:0x80; bits[59]:0x8000000
// args: bits[33]:0x10000000; bits[22]:0x10000; bits[59]:0x400000
// args: bits[33]:0x100000000; bits[22]:0x100; bits[59]:0x7ffffffffffffff
// args: bits[33]:0xffffffff; bits[22]:0x3fffff; bits[59]:0x100000000
// args: bits[33]:0x8000; bits[22]:0x1000; bits[59]:0x80000000000
// args: bits[33]:0x4; bits[22]:0x80000; bits[59]:0x20000
// args: bits[33]:0x2; bits[22]:0x8; bits[59]:0x4000000000000
// args: bits[33]:0x10; bits[22]:0x20000; bits[59]:0x400
// args: bits[33]:0x80000; bits[22]:0x2; bits[59]:0x1000000
// args: bits[33]:0x4; bits[22]:0x400; bits[59]:0x40000000000000
// args: bits[33]:0x10000; bits[22]:0x10000; bits[59]:0x80
// args: bits[33]:0xc6975e0a; bits[22]:0x200000; bits[59]:0x10000
// args: bits[33]:0x40000; bits[22]:0x200000; bits[59]:0x3ffffffffffffff
// args: bits[33]:0x1ffffffff; bits[22]:0x2; bits[59]:0x4f018ef1a29d101
// args: bits[33]:0x800; bits[22]:0x10000; bits[59]:0x8000000
// args: bits[33]:0x1; bits[22]:0x34b865; bits[59]:0x4c767848b9d10c3
// args: bits[33]:0x800000; bits[22]:0x10000; bits[59]:0x100000
// args: bits[33]:0x80000; bits[22]:0x100; bits[59]:0x800000000000
// args: bits[33]:0x2000; bits[22]:0x4000; bits[59]:0x100000000
// args: bits[33]:0x0; bits[22]:0x80000; bits[59]:0x800000
// args: bits[33]:0x0; bits[22]:0x8000; bits[59]:0x400000000
// args: bits[33]:0x155555555; bits[22]:0x4; bits[59]:0x80
// args: bits[33]:0x40; bits[22]:0x100000; bits[59]:0x20000
// args: bits[33]:0x200; bits[22]:0x400; bits[59]:0x20000000000000
// args: bits[33]:0x20000; bits[22]:0x80; bits[59]:0x20
// args: bits[33]:0x1; bits[22]:0x200000; bits[59]:0x10
// args: bits[33]:0x200; bits[22]:0x2000; bits[59]:0x200
// args: bits[33]:0x12183c46b; bits[22]:0x2aaaaa; bits[59]:0x8000
// args: bits[33]:0x200000; bits[22]:0x800; bits[59]:0x555555555555555
// args: bits[33]:0x155555555; bits[22]:0x20; bits[59]:0x80
// args: bits[33]:0x1fe7d3750; bits[22]:0x155555; bits[59]:0x100000000000000
// args: bits[33]:0x4; bits[22]:0x200000; bits[59]:0x2aaaaaaaaaaaaaa
// args: bits[33]:0x20000; bits[22]:0x2aaaaa; bits[59]:0x8
// args: bits[33]:0x20; bits[22]:0x2bb13f; bits[59]:0x400000000
// args: bits[33]:0x7bcf32e9; bits[22]:0x20; bits[59]:0x40000000
// args: bits[33]:0x155555555; bits[22]:0x80; bits[59]:0x400000000000
// args: bits[33]:0x20000000; bits[22]:0x8000; bits[59]:0x7ffffffffffffff
// args: bits[33]:0x1000000; bits[22]:0x400; bits[59]:0x8000000000
// args: bits[33]:0x400; bits[22]:0x4; bits[59]:0x200000000000
// args: bits[33]:0xffffffff; bits[22]:0x8000; bits[59]:0x0
// args: bits[33]:0xebb93341; bits[22]:0x193f17; bits[59]:0x40
// args: bits[33]:0x400; bits[22]:0x200; bits[59]:0x200000
// args: bits[33]:0x8000000; bits[22]:0x155555; bits[59]:0x100000000000000
// args: bits[33]:0x94871a0d; bits[22]:0x1000; bits[59]:0x10000000
// args: bits[33]:0x100000; bits[22]:0x8; bits[59]:0x1
// args: bits[33]:0x10; bits[22]:0x2000; bits[59]:0x80
// args: bits[33]:0x400; bits[22]:0x100; bits[59]:0x2
// args: bits[33]:0x1000000; bits[22]:0x20; bits[59]:0x2000000000
// args: bits[33]:0x1ffffffff; bits[22]:0x1fffff; bits[59]:0x25bcafb2767619f
// args: bits[33]:0x400000; bits[22]:0x80000; bits[59]:0x4000000
// args: bits[33]:0x4000000; bits[22]:0x8; bits[59]:0x20000
// args: bits[33]:0x1ffffffff; bits[22]:0x2816fb; bits[59]:0x10
// args: bits[33]:0x1000000; bits[22]:0x40; bits[59]:0x4769f974df4b514
// args: bits[33]:0x4992a60c; bits[22]:0x100; bits[59]:0x14a2c3c04143b67
// args: bits[33]:0x20000; bits[22]:0x40; bits[59]:0x400000000000000
// args: bits[33]:0x80000; bits[22]:0x3fffff; bits[59]:0x8000000
// args: bits[33]:0x8000; bits[22]:0x10000; bits[59]:0x2000000000
// args: bits[33]:0x1000; bits[22]:0x11a5ff; bits[59]:0x200000
// args: bits[33]:0x20000000; bits[22]:0x100; bits[59]:0x400000000000000
// args: bits[33]:0x2000; bits[22]:0x3fffff; bits[59]:0x800000000
// args: bits[33]:0x80000000; bits[22]:0x8000; bits[59]:0x400000000
// args: bits[33]:0x1080c20e9; bits[22]:0x100; bits[59]:0x20000000
// args: bits[33]:0x100000000; bits[22]:0x2; bits[59]:0x2
// args: bits[33]:0x155555555; bits[22]:0x20000; bits[59]:0x10
// args: bits[33]:0x10000000; bits[22]:0x2000; bits[59]:0x20000000000
// args: bits[33]:0x8000000; bits[22]:0x1000; bits[59]:0x10000000000000
// args: bits[33]:0x100000; bits[22]:0x1; bits[59]:0x8
// args: bits[33]:0x20; bits[22]:0x20000; bits[59]:0x20
// args: bits[33]:0x10000000; bits[22]:0x16b511; bits[59]:0x4ab79dc18f483d
// args: bits[33]:0x4000; bits[22]:0x800; bits[59]:0x80000000000000
// args: bits[33]:0x10000000; bits[22]:0x400; bits[59]:0x40000
// args: bits[33]:0x2000000; bits[22]:0x200; bits[59]:0x10000000000000
// args: bits[33]:0x4000000; bits[22]:0x303a70; bits[59]:0x2
// args: bits[33]:0x1; bits[22]:0x1fffff; bits[59]:0x2000
// args: bits[33]:0x80; bits[22]:0x194bfa; bits[59]:0x3a49de08a602c40
// args: bits[33]:0x0; bits[22]:0x400; bits[59]:0x400000000000000
// args: bits[33]:0xb9314a5d; bits[22]:0x20000; bits[59]:0x2aaaaaaaaaaaaaa
// args: bits[33]:0x8000; bits[22]:0x4000; bits[59]:0x48004664f41bdbf
// args: bits[33]:0x8; bits[22]:0x10; bits[59]:0x10000000000
// args: bits[33]:0x0; bits[22]:0x1000; bits[59]:0x20000000000000
// args: bits[33]:0x80000000; bits[22]:0x8000; bits[59]:0x80000
// args: bits[33]:0x8000; bits[22]:0x40; bits[59]:0x800000000000
// args: bits[33]:0x40000; bits[22]:0x1; bits[59]:0x200000
// args: bits[33]:0xffffffff; bits[22]:0x400; bits[59]:0x1000
// args: bits[33]:0x2000; bits[22]:0x100000; bits[59]:0x4000000000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x1; bits[59]:0x20000000
// args: bits[33]:0x3e0a3a5d; bits[22]:0x4000; bits[59]:0x20000000
// args: bits[33]:0x400; bits[22]:0x155555; bits[59]:0x800000000
// args: bits[33]:0x40000000; bits[22]:0x40; bits[59]:0x8000000000
// args: bits[33]:0xffffffff; bits[22]:0x155555; bits[59]:0x1000000000
// args: bits[33]:0x10; bits[22]:0x80000; bits[59]:0x800000000
// args: bits[33]:0x6423f410; bits[22]:0x2; bits[59]:0x40000000
// args: bits[33]:0x100; bits[22]:0x8; bits[59]:0x4000000000
// args: bits[33]:0x155555555; bits[22]:0x4000; bits[59]:0x4000000000000
// args: bits[33]:0x1000000; bits[22]:0x100000; bits[59]:0x3ffffffffffffff
// args: bits[33]:0x0; bits[22]:0x8; bits[59]:0x100000000000
// args: bits[33]:0x100; bits[22]:0x1b184; bits[59]:0x10000
// args: bits[33]:0x2; bits[22]:0x2000; bits[59]:0x1000000000
// args: bits[33]:0x800; bits[22]:0x1000; bits[59]:0x1
// args: bits[33]:0x20000000; bits[22]:0x40; bits[59]:0x80000
// args: bits[33]:0x100; bits[22]:0x40000; bits[59]:0x10
// args: bits[33]:0x40000; bits[22]:0x80; bits[59]:0x7e9a0d776d71ef8
// args: bits[33]:0x400000; bits[22]:0x155555; bits[59]:0x400000000000
// args: bits[33]:0x40000; bits[22]:0x20000; bits[59]:0x1000
// args: bits[33]:0x40000; bits[22]:0x100; bits[59]:0x16f29097c227774
// args: bits[33]:0x1810a2547; bits[22]:0x20000; bits[59]:0x400000000000
// args: bits[33]:0x20000; bits[22]:0x266603; bits[59]:0x20000
// args: bits[33]:0x4; bits[22]:0x200; bits[59]:0x1000
// args: bits[33]:0x0; bits[22]:0x20; bits[59]:0x40000000
// args: bits[33]:0x20000000; bits[22]:0x155555; bits[59]:0x27a0be38b5d9155
// args: bits[33]:0x4000000; bits[22]:0x8; bits[59]:0x400000000
// args: bits[33]:0x80000000; bits[22]:0x0; bits[59]:0x100000
// args: bits[33]:0x1ffffffff; bits[22]:0x100000; bits[59]:0x200000000000
// args: bits[33]:0x800000; bits[22]:0x3fffff; bits[59]:0x1
// args: bits[33]:0x40; bits[22]:0x2000; bits[59]:0x54012d27e27ea78
// args: bits[33]:0x8000; bits[22]:0x0; bits[59]:0x20000000
// args: bits[33]:0x8; bits[22]:0x1; bits[59]:0x2000000
// args: bits[33]:0x10; bits[22]:0x155555; bits[59]:0x8
// args: bits[33]:0xffffffff; bits[22]:0x100; bits[59]:0x400000000000
// args: bits[33]:0x200000; bits[22]:0x0; bits[59]:0x800000
// args: bits[33]:0x4; bits[22]:0x3fffff; bits[59]:0x80000000000000
// args: bits[33]:0x800; bits[22]:0x2; bits[59]:0x4000000
// args: bits[33]:0x1ffffffff; bits[22]:0x10; bits[59]:0x1000
// args: bits[33]:0x2000000; bits[22]:0x1; bits[59]:0x400
// args: bits[33]:0x80; bits[22]:0x10000; bits[59]:0x40000000000000
// args: bits[33]:0x1ffffffff; bits[22]:0x2aaaaa; bits[59]:0x8000000000
// args: bits[33]:0x4000; bits[22]:0x10; bits[59]:0x2000
// args: bits[33]:0x4; bits[22]:0x200000; bits[59]:0x20
// args: bits[33]:0x4000000; bits[22]:0x1; bits[59]:0x8
// args: bits[33]:0x400000; bits[22]:0x20000; bits[59]:0x4
// args: bits[33]:0x4; bits[22]:0x400; bits[59]:0x400000000000
// args: bits[33]:0x200000; bits[22]:0x8000; bits[59]:0x8
// args: bits[33]:0x200000; bits[22]:0x2aaaaa; bits[59]:0x40000000000
// args: bits[33]:0x10000; bits[22]:0x800; bits[59]:0x2000
// args: bits[33]:0x1ffffffff; bits[22]:0x8000; bits[59]:0x555555555555555
// args: bits[33]:0x10000000; bits[22]:0x10; bits[59]:0x555555555555555
// args: bits[33]:0x20000000; bits[22]:0x155555; bits[59]:0x0
// args: bits[33]:0x4000000; bits[22]:0x8000; bits[59]:0x2000000000000
// args: bits[33]:0x200000; bits[22]:0x1000; bits[59]:0x8000000000
// args: bits[33]:0x10000000; bits[22]:0x80; bits[59]:0x8000000000000
// args: bits[33]:0x1000000; bits[22]:0x2000; bits[59]:0x8
// args: bits[33]:0x80000; bits[22]:0x155555; bits[59]:0x10000000000
// args: bits[33]:0x2000000; bits[22]:0x2; bits[59]:0x200000000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x1000; bits[59]:0x8000
// args: bits[33]:0x9e9bb1e4; bits[22]:0x0; bits[59]:0x400000000000000
// args: bits[33]:0x2; bits[22]:0x1000; bits[59]:0x20000000000
// args: bits[33]:0x40; bits[22]:0x100000; bits[59]:0x23bf54c13db7261
// args: bits[33]:0x20; bits[22]:0x100; bits[59]:0x2
// args: bits[33]:0x1000; bits[22]:0x100; bits[59]:0x80
// args: bits[33]:0x400000; bits[22]:0x1000; bits[59]:0x4000000000
// args: bits[33]:0x4000000; bits[22]:0x40; bits[59]:0x4000
// args: bits[33]:0x155555555; bits[22]:0x100000; bits[59]:0x400
// args: bits[33]:0x19e09e62e; bits[22]:0x4000; bits[59]:0x37a1e465ecb8bfa
// args: bits[33]:0x20000; bits[22]:0x8000; bits[59]:0x10000000000
// args: bits[33]:0x80; bits[22]:0x200; bits[59]:0x20000000000000
// args: bits[33]:0x100; bits[22]:0x2000; bits[59]:0x1da3542325ce50
// args: bits[33]:0x80000000; bits[22]:0x1; bits[59]:0x40000000
// args: bits[33]:0x10; bits[22]:0x3fffff; bits[59]:0x40
// args: bits[33]:0x155555555; bits[22]:0x800; bits[59]:0x100000000000000
// args: bits[33]:0x100; bits[22]:0x2aaaaa; bits[59]:0x3b9accda719cc0c
// args: bits[33]:0x2000; bits[22]:0x10; bits[59]:0x555555555555555
// args: bits[33]:0x11c43db1f; bits[22]:0x40000; bits[59]:0x800000000000
// args: bits[33]:0x1000000; bits[22]:0x4; bits[59]:0x20000000000
// args: bits[33]:0x800000; bits[22]:0x20000; bits[59]:0x80000000000000
// args: bits[33]:0x10000000; bits[22]:0x800; bits[59]:0x40000000000
// args: bits[33]:0x4; bits[22]:0x2aaaaa; bits[59]:0x2000000000000
// args: bits[33]:0xffffffff; bits[22]:0x100; bits[59]:0x200000000000
// args: bits[33]:0x66a393f5; bits[22]:0x1; bits[59]:0x400000000
// args: bits[33]:0xeaa33010; bits[22]:0x100; bits[59]:0x72edad12d84ce7d
// args: bits[33]:0x1ac9f9678; bits[22]:0x2f8b4c; bits[59]:0x485597c3b5754dd
// args: bits[33]:0x100000; bits[22]:0x185e4b; bits[59]:0x3cd97703decb630
// args: bits[33]:0x40000; bits[22]:0x8; bits[59]:0x3fc4d7f329703b6
// args: bits[33]:0xffffffff; bits[22]:0x2aaaaa; bits[59]:0x80000000000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x80; bits[59]:0x100000
// args: bits[33]:0x458908c3; bits[22]:0xba5fd; bits[59]:0x400000000
// args: bits[33]:0x400000; bits[22]:0x800; bits[59]:0x4
// args: bits[33]:0x4000; bits[22]:0x1; bits[59]:0x2d896ed07d79ef5
// args: bits[33]:0x4000; bits[22]:0x4; bits[59]:0x20000000000
// args: bits[33]:0x1000; bits[22]:0x40; bits[59]:0x3ffffffffffffff
// args: bits[33]:0x4000000; bits[22]:0x40; bits[59]:0x8000000
// args: bits[33]:0x20000000; bits[22]:0x80000; bits[59]:0x4000000000
// args: bits[33]:0x100000000; bits[22]:0x2aaaaa; bits[59]:0x1000
// args: bits[33]:0x4000000; bits[22]:0x40000; bits[59]:0x10000000000
// args: bits[33]:0x170da4bd6; bits[22]:0x80000; bits[59]:0x2000000000000
// args: bits[33]:0x800; bits[22]:0x10; bits[59]:0x20000000000
// args: bits[33]:0x8; bits[22]:0x40000; bits[59]:0x20000000000
// args: bits[33]:0x10000000; bits[22]:0x1; bits[59]:0x4000
// args: bits[33]:0x100; bits[22]:0x20000; bits[59]:0x2000
// args: bits[33]:0x200; bits[22]:0x4; bits[59]:0x400000000
// args: bits[33]:0x1000000; bits[22]:0x1fffff; bits[59]:0x555555555555555
// args: bits[33]:0x800; bits[22]:0x20; bits[59]:0x2000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x8; bits[59]:0x20
// args: bits[33]:0x1000; bits[22]:0x4000; bits[59]:0x100000
// args: bits[33]:0x40000000; bits[22]:0x3fffff; bits[59]:0x4000
// args: bits[33]:0x1ffffffff; bits[22]:0x2; bits[59]:0x100000000000
// args: bits[33]:0x800; bits[22]:0x4; bits[59]:0x4000000000
// args: bits[33]:0x20000; bits[22]:0x163ffd; bits[59]:0x2000000000
// args: bits[33]:0x2000000; bits[22]:0x2aaaaa; bits[59]:0x800000000000
// args: bits[33]:0x80; bits[22]:0x20; bits[59]:0x1000000
// args: bits[33]:0x200; bits[22]:0x8; bits[59]:0x100000000000000
// args: bits[33]:0x200; bits[22]:0x155555; bits[59]:0x2000000
// args: bits[33]:0x40; bits[22]:0x40000; bits[59]:0x455a04581c4434d
// args: bits[33]:0x4000; bits[22]:0x40; bits[59]:0x5d94d88f37e36ed
// args: bits[33]:0x80; bits[22]:0x80000; bits[59]:0x400000000000
// args: bits[33]:0x10000000; bits[22]:0x3fffff; bits[59]:0x4000000000
// args: bits[33]:0x1e76cd61d; bits[22]:0x1000; bits[59]:0x65775b0970467
// args: bits[33]:0x40000000; bits[22]:0x4000; bits[59]:0x10
// args: bits[33]:0x8000; bits[22]:0x8; bits[59]:0x200000000
// args: bits[33]:0x2000; bits[22]:0x800; bits[59]:0x100000000000000
// args: bits[33]:0x80; bits[22]:0x10; bits[59]:0x20
// args: bits[33]:0x10000; bits[22]:0x8; bits[59]:0x1000000
// args: bits[33]:0x1b173cfae; bits[22]:0x1000; bits[59]:0x80000000000
// args: bits[33]:0x1000000; bits[22]:0x2000; bits[59]:0x20
// args: bits[33]:0xffffffff; bits[22]:0x400; bits[59]:0x7d511d02bb59456
// args: bits[33]:0x20000; bits[22]:0x200; bits[59]:0x2000000000000
// args: bits[33]:0x1000; bits[22]:0x200000; bits[59]:0x247053890341a7e
// args: bits[33]:0x20000; bits[22]:0x40; bits[59]:0x20000000000
// args: bits[33]:0x20000000; bits[22]:0x10000; bits[59]:0x400000000000000
// args: bits[33]:0x40000000; bits[22]:0x2; bits[59]:0x201dc80886a044
// args: bits[33]:0x100000; bits[22]:0x20; bits[59]:0x40
// args: bits[33]:0x10000; bits[22]:0x761d9; bits[59]:0x200
// args: bits[33]:0x800; bits[22]:0x155555; bits[59]:0x800000
// args: bits[33]:0x40000; bits[22]:0x4; bits[59]:0x2000000000
// args: bits[33]:0x4000000; bits[22]:0x400; bits[59]:0x10000000
// args: bits[33]:0x80000000; bits[22]:0x10; bits[59]:0x1000000000
// args: bits[33]:0x0; bits[22]:0x800; bits[59]:0x1000000000
// args: bits[33]:0x400000; bits[22]:0x80; bits[59]:0x555555555555555
// args: bits[33]:0x0; bits[22]:0x24be43; bits[59]:0x200
// args: bits[33]:0x10000; bits[22]:0x2; bits[59]:0x64c380b72b66e58
// args: bits[33]:0x8; bits[22]:0x80000; bits[59]:0x10000000000000
// args: bits[33]:0x2000; bits[22]:0x10000; bits[59]:0x4000000000
// args: bits[33]:0x80000000; bits[22]:0x100000; bits[59]:0x40000000000
// args: bits[33]:0x4000; bits[22]:0x1fffff; bits[59]:0x3b7b73845dda062
// args: bits[33]:0x2000; bits[22]:0x10; bits[59]:0x29331eac1186e4f
// args: bits[33]:0x100; bits[22]:0x8000; bits[59]:0x2000000000
// args: bits[33]:0x800; bits[22]:0x200; bits[59]:0x10000000000
// args: bits[33]:0x400; bits[22]:0x112764; bits[59]:0x1000
// args: bits[33]:0xe2bac435; bits[22]:0x2aaaaa; bits[59]:0x80000000
// args: bits[33]:0x100; bits[22]:0x80; bits[59]:0x800
// args: bits[33]:0xec2343d4; bits[22]:0x200; bits[59]:0x4000000000
// args: bits[33]:0x80; bits[22]:0x200000; bits[59]:0x400000
// args: bits[33]:0x20000; bits[22]:0x20; bits[59]:0x7ffffffffffffff
// args: bits[33]:0x80000000; bits[22]:0x2; bits[59]:0x4
// args: bits[33]:0x10000000; bits[22]:0x1fffff; bits[59]:0x2000000
// args: bits[33]:0x8000000; bits[22]:0x35ef57; bits[59]:0x400000
// args: bits[33]:0x10; bits[22]:0x0; bits[59]:0x200000000000000
// args: bits[33]:0x20; bits[22]:0x200; bits[59]:0x8000000
// args: bits[33]:0x20000; bits[22]:0x200; bits[59]:0x8000000000000
// args: bits[33]:0x2; bits[22]:0x4000; bits[59]:0x10000
// args: bits[33]:0xffffffff; bits[22]:0x1; bits[59]:0x200000000
// args: bits[33]:0x8000000; bits[22]:0x100000; bits[59]:0x8
// args: bits[33]:0x10000; bits[22]:0x10000; bits[59]:0x40000000000
// args: bits[33]:0x2000000; bits[22]:0x40; bits[59]:0x0
// args: bits[33]:0x11a4cf18c; bits[22]:0x1; bits[59]:0x800000000000
// args: bits[33]:0x10000; bits[22]:0x200000; bits[59]:0x3ffffffffffffff
// args: bits[33]:0x1ae69b5c3; bits[22]:0x100; bits[59]:0x100000
// args: bits[33]:0x4; bits[22]:0x8; bits[59]:0x400
// args: bits[33]:0x80; bits[22]:0x17f5db; bits[59]:0x8000000000
// args: bits[33]:0xc43a28e2; bits[22]:0x1; bits[59]:0x80000
// args: bits[33]:0x40; bits[22]:0x100; bits[59]:0x31d3451883793fd
// args: bits[33]:0x1ffffffff; bits[22]:0x20000; bits[59]:0x80000
// args: bits[33]:0x8000000; bits[22]:0x8; bits[59]:0x2000000000000
// args: bits[33]:0xffffffff; bits[22]:0x1000; bits[59]:0x800000
// args: bits[33]:0x100; bits[22]:0x10; bits[59]:0x3bcd0c7436adeae
// args: bits[33]:0x1000000; bits[22]:0x1; bits[59]:0x100000000000
// args: bits[33]:0x2000; bits[22]:0xd261d; bits[59]:0x12e818813ba64ca
// args: bits[33]:0x0; bits[22]:0x1; bits[59]:0x4000
// args: bits[33]:0x1; bits[22]:0x200; bits[59]:0x30dc259cc6ef3ae
// args: bits[33]:0x200; bits[22]:0x10; bits[59]:0x200
// args: bits[33]:0x2000000; bits[22]:0x2; bits[59]:0x10000000000000
// args: bits[33]:0x4; bits[22]:0x10000; bits[59]:0x20000000000000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x8000; bits[59]:0x40000000000
// args: bits[33]:0x40000; bits[22]:0x8; bits[59]:0x40000000
// args: bits[33]:0x1cc12f4cd; bits[22]:0x3fffff; bits[59]:0x20000000000
// args: bits[33]:0x4000000; bits[22]:0x40000; bits[59]:0x555555555555555
// args: bits[33]:0x12a8a288a; bits[22]:0x20; bits[59]:0x653ad3655d3b87b
// args: bits[33]:0x80000; bits[22]:0x10000; bits[59]:0x20
// args: bits[33]:0xffffffff; bits[22]:0x8; bits[59]:0x0
// args: bits[33]:0x100000000; bits[22]:0x100; bits[59]:0x100000
// args: bits[33]:0x20000; bits[22]:0x0; bits[59]:0x4
// args: bits[33]:0x20000000; bits[22]:0x8000; bits[59]:0x800000000
// args: bits[33]:0x100000; bits[22]:0x200; bits[59]:0x40000000000
// args: bits[33]:0x4000000; bits[22]:0x40000; bits[59]:0x10000000
// args: bits[33]:0x40000; bits[22]:0x4000; bits[59]:0x400000000
// args: bits[33]:0xd7494840; bits[22]:0x2e1b61; bits[59]:0x2ff531ca555f0df
// args: bits[33]:0x1ffffffff; bits[22]:0x1fffff; bits[59]:0x4000
// args: bits[33]:0x80; bits[22]:0x30514e; bits[59]:0x2
// args: bits[33]:0x4000000; bits[22]:0x1; bits[59]:0x80000000000
// args: bits[33]:0x10000000; bits[22]:0x3fffff; bits[59]:0x8000000
// args: bits[33]:0x80000; bits[22]:0x155555; bits[59]:0x4bb9b90fed681d4
// args: bits[33]:0x2000000; bits[22]:0x0; bits[59]:0x80
// args: bits[33]:0x200000; bits[22]:0x800; bits[59]:0x10000000000
// args: bits[33]:0x800000; bits[22]:0x3fffff; bits[59]:0x200000000000
// args: bits[33]:0x40000; bits[22]:0x1000; bits[59]:0x80000000000
// args: bits[33]:0x193573a52; bits[22]:0x100; bits[59]:0x10000000000
// args: bits[33]:0x20; bits[22]:0x1fffff; bits[59]:0x80000
// args: bits[33]:0x20; bits[22]:0x40; bits[59]:0x1000000000
// args: bits[33]:0x988ff055; bits[22]:0x0; bits[59]:0x4000000
// args: bits[33]:0x10000; bits[22]:0x400; bits[59]:0x400000000000
// args: bits[33]:0x80000; bits[22]:0x2eefe6; bits[59]:0x400
// args: bits[33]:0x80000000; bits[22]:0x155555; bits[59]:0x4000000
// args: bits[33]:0x8000000; bits[22]:0x0; bits[59]:0x400000000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x40000; bits[59]:0x2
// args: bits[33]:0x4; bits[22]:0x20; bits[59]:0x400000000000000
// args: bits[33]:0x73b9105; bits[22]:0x40; bits[59]:0x1000000000
// args: bits[33]:0x1000; bits[22]:0x2c733d; bits[59]:0x40000000000
// args: bits[33]:0x40000000; bits[22]:0x20; bits[59]:0x7ffffffffffffff
// args: bits[33]:0x20; bits[22]:0x20000; bits[59]:0x4000
// args: bits[33]:0x1; bits[22]:0x80; bits[59]:0x4000000000000
// args: bits[33]:0x10000000; bits[22]:0x40; bits[59]:0x20000000000000
// args: bits[33]:0x100000000; bits[22]:0x3fffff; bits[59]:0x100
// args: bits[33]:0x10000; bits[22]:0x800; bits[59]:0x800
// args: bits[33]:0x9826148; bits[22]:0x109764; bits[59]:0x2000000000
// args: bits[33]:0x1000; bits[22]:0x80000; bits[59]:0x555555555555555
// args: bits[33]:0x80000000; bits[22]:0x1; bits[59]:0x688ff737e523a3d
// args: bits[33]:0x1f272efd6; bits[22]:0x40000; bits[59]:0x4000000000
// args: bits[33]:0x0; bits[22]:0x4000; bits[59]:0x2000000000
// args: bits[33]:0x8000000; bits[22]:0x2; bits[59]:0x3ffffffffffffff
// args: bits[33]:0x100000000; bits[22]:0x20; bits[59]:0x400000000000000
// args: bits[33]:0x80000000; bits[22]:0x80000; bits[59]:0x2
// args: bits[33]:0x40000; bits[22]:0x2; bits[59]:0x1
// args: bits[33]:0xffffffff; bits[22]:0x3fffff; bits[59]:0x800000000000
// args: bits[33]:0x1000000; bits[22]:0x200; bits[59]:0x40000
// args: bits[33]:0x800; bits[22]:0xe6164; bits[59]:0x800000000000
// args: bits[33]:0x8000; bits[22]:0x20; bits[59]:0x1
// args: bits[33]:0x40000; bits[22]:0x20; bits[59]:0x200000000000
// args: bits[33]:0x400; bits[22]:0x20000; bits[59]:0x5e480fbd20effa0
// args: bits[33]:0x1; bits[22]:0x1bdfe9; bits[59]:0x400000000
// args: bits[33]:0xf2c86dd9; bits[22]:0x20000; bits[59]:0x761c79c2b68d050
// args: bits[33]:0x20000; bits[22]:0x100; bits[59]:0x8000000000
// args: bits[33]:0x200000; bits[22]:0x200; bits[59]:0x7ffffffffffffff
// args: bits[33]:0x20; bits[22]:0x20000; bits[59]:0x3ffffffffffffff
// args: bits[33]:0x100000; bits[22]:0x80000; bits[59]:0x100000000000
// args: bits[33]:0x10000; bits[22]:0x8000; bits[59]:0x2000000000
// args: bits[33]:0x800; bits[22]:0x10; bits[59]:0x200000000000
// args: bits[33]:0x10; bits[22]:0x2; bits[59]:0x100
// args: bits[33]:0x40; bits[22]:0x3fffff; bits[59]:0x4000000000
// args: bits[33]:0x800; bits[22]:0x100000; bits[59]:0x8000000000000
// args: bits[33]:0x100000000; bits[22]:0x200000; bits[59]:0x8
// args: bits[33]:0xa10a1184; bits[22]:0x1fffff; bits[59]:0x800000
// args: bits[33]:0x10000000; bits[22]:0x30fc1c; bits[59]:0x129c27264938a87
// args: bits[33]:0x8000; bits[22]:0x155555; bits[59]:0x40000000000000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x2000; bits[59]:0x40000000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x20000; bits[59]:0x10000000
// args: bits[33]:0x1000; bits[22]:0x200000; bits[59]:0x200
// args: bits[33]:0x1000; bits[22]:0x40; bits[59]:0x40000000
// args: bits[33]:0xaaaaaaaa; bits[22]:0x10000; bits[59]:0x80000000000
// args: bits[33]:0x10000000; bits[22]:0x155555; bits[59]:0x40000000000000
// args: bits[33]:0x8000000; bits[22]:0x2aaaaa; bits[59]:0x1000000
// args: bits[33]:0x200; bits[22]:0x4; bits[59]:0x20000000000000
// args: bits[33]:0x100000; bits[22]:0x1; bits[59]:0x8
// args: bits[33]:0x34b08fb5; bits[22]:0x400; bits[59]:0x800000000
// args: bits[33]:0x200; bits[22]:0x10000; bits[59]:0x2000000
// args: bits[33]:0x8000000; bits[22]:0x32a9ba; bits[59]:0x555555555555555
// args: bits[33]:0x8000; bits[22]:0x2aaaaa; bits[59]:0x1000000
// args: bits[33]:0x8000; bits[22]:0x80; bits[59]:0x10
// args: bits[33]:0x53717926; bits[22]:0x10000; bits[59]:0x20
// args: bits[33]:0x200000; bits[22]:0x4000; bits[59]:0x800000
// args: bits[33]:0x8; bits[22]:0x100000; bits[59]:0x53c33c0e1e36a7c
// args: bits[33]:0x8; bits[22]:0x20; bits[59]:0x80000000
// args: bits[33]:0x40000000; bits[22]:0x400; bits[59]:0x0
// args: bits[33]:0xffffffff; bits[22]:0x40; bits[59]:0x1e9890f0ea48ef2
// args: bits[33]:0x0; bits[22]:0x80000; bits[59]:0x555555555555555
// args: bits[33]:0x40; bits[22]:0x278b8f; bits[59]:0x80000
// args: bits[33]:0x10000; bits[22]:0x10; bits[59]:0x200000000
// args: bits[33]:0x9866615a; bits[22]:0x100000; bits[59]:0x40000000000000
// args: bits[33]:0x80000; bits[22]:0x1000; bits[59]:0x1eaefbe369824cd
// args: bits[33]:0x1960d06eb; bits[22]:0x2000; bits[59]:0x200000
// args: bits[33]:0x400; bits[22]:0x2000; bits[59]:0x4000000000000
// args: bits[33]:0x800; bits[22]:0x100000; bits[59]:0x521a02190aa27fa
// args: bits[33]:0x400; bits[22]:0x4000; bits[59]:0x100000000000
// args: bits[33]:0x20000000; bits[22]:0x155555; bits[59]:0x8000000000000
// args: bits[33]:0x80000000; bits[22]:0x100; bits[59]:0x20000000000
// args: bits[33]:0x100000000; bits[22]:0x800; bits[59]:0x400000000000000
// args: bits[33]:0x200; bits[22]:0x155555; bits[59]:0x8000000
// args: bits[33]:0x100000000; bits[22]:0x10000; bits[59]:0x100000000
// args: bits[33]:0x2; bits[22]:0x3fffff; bits[59]:0x20000000000
// args: bits[33]:0x10000000; bits[22]:0x400; bits[59]:0x80
// args: bits[33]:0xaaaaaaaa; bits[22]:0x4000; bits[59]:0x80
// args: bits[33]:0x10; bits[22]:0x1fffff; bits[59]:0x100000000
// args: bits[33]:0x155555555; bits[22]:0x100000; bits[59]:0x80000000
// args: bits[33]:0x155555555; bits[22]:0x1; bits[59]:0x8000000
// args: bits[33]:0x0; bits[22]:0x80000; bits[59]:0x20
// args: bits[33]:0x100000000; bits[22]:0x2aaaaa; bits[59]:0x800000000000
// args: bits[33]:0x4000; bits[22]:0x4; bits[59]:0x555555555555555
// args: bits[33]:0x0; bits[22]:0x1; bits[59]:0x555555555555555
// args: bits[33]:0x800; bits[22]:0x343d99; bits[59]:0x200000000
// args: bits[33]:0x80; bits[22]:0x10; bits[59]:0x200000000000
// args: bits[33]:0x200; bits[22]:0x10; bits[59]:0x2000000000
// args: bits[33]:0x100; bits[22]:0x100000; bits[59]:0x10000000
// args: bits[33]:0x1ffffffff; bits[22]:0x2000; bits[59]:0x5b2a1c78e6c21a9
// args: bits[33]:0x8000; bits[22]:0x155555; bits[59]:0x5a20fa9bedd47a8
// args: bits[33]:0x8000; bits[22]:0x10000; bits[59]:0x0
// args: bits[33]:0x40; bits[22]:0x155555; bits[59]:0x4109d2afd3abd74
fn main(x5994102: u33, x5994103: u22, x5994104: u59) -> (u1, u22, u59, u22, u60, u33, u60, u32, u33, (u33, u33, u33, u59), u33, u43) {
    let x5994105: u33 = clz(x5994102);
    let x5994106: (u33, u33, u33, u59) = (x5994105, x5994102, x5994102, x5994104);
    let x5994107: u22 = (x5994103) & (x5994103);
    let x5994108: u22 = -(x5994107);
    let x5994109: u59 = (x5994104) & ((x5994108 as u59));
    let x5994110: u33 = one_hot_sel((u1:0x0), [x5994105]);
    let x5994111: u33 = x5994110;
    let x5994112: u34 = one_hot(x5994111, (u1:1));
    let x5994113: u60 = (u60:0xfffffffffffffff);
    let x5994114: u33 = -(x5994111);
    let x5994115: u33 = !(x5994111);
    let x5994116: u44 = (x5994108) ++ (x5994107);
    let x5994117: u22 = one_hot_sel((u2:0x3), [x5994108, x5994103]);
    let x5994118: u34 = one_hot_sel((u4:0x1), [x5994112, x5994112, x5994112, x5994112]);
    let x5994119: u4 = (u4:0x0);
    let x5994120: u33 = ((x5994103 as u33)) & (x5994115);
    let x5994121: u33 = x5994105;
    let x5994122: u1 = ((x5994120) != ((u33:0x0))) || ((x5994111) != ((u33:0x0)));
    let x5994123: u43 = (u43:0x3ffffffffff);
    let x5994124: u22 = (x5994117) ^ ((x5994119 as u22));
    let x5994125: u32 = (u32:0x400);
    let x5994126: u59 = x5994109;
    let x5994127: u33 = (x5994121) + ((x5994119 as u33));
    let x5994128: u33 = one_hot_sel(x5994122, [x5994105]);
    let x5994129: u33 = -(x5994121);
    let x5994130: u32 = one_hot_sel(x5994122, [x5994125]);
    let x5994131: (u22, u59, u22) = (x5994107, x5994109, x5994117);
    let x5994132: u33 = one_hot_sel(x5994119, [x5994128, x5994102, x5994128, x5994128]);
    (x5994122, x5994107, x5994109, x5994108, x5994113, x5994127, x5994113, x5994130, x5994111, x5994106, x5994105, x5994123)
}


