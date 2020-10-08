# Lint as: python3
#
# Copyright 2020 The XLS Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Defines the ImportFn type signature, used to break circular dependency."""

from typing import Callable, Text, Tuple

from xls.dslx.python import cpp_ast as ast
from xls.dslx.python import cpp_type_info as type_info

ModuleInfo = Tuple[ast.Module, type_info.TypeInfo]
ImportTokens = Tuple[Text, ...]
ImportFn = Callable[[ImportTokens], ModuleInfo]
