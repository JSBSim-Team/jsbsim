#! /usr/bin/python
#
# pyxstubgen.py
#
# Automatic generation of the JSBSim stub file `_jsbsim.pyi` from Cython's pyx file.
#
# Copyright (c) 2025 Bertrand Coconnier
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>
#

import argparse
import io
from typing import List, Optional, Tuple, Union

from lark import Lark
from lark.indenter import PythonIndenter
from lark.lexer import Token
from lark.tree import Tree
from lark.visitors import Interpreter

arg_parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
arg_parser.add_argument(
    "--pyxfile", metavar="<filename>", required=True, help="specifies the *.pyx file"
)
arg_parser.add_argument(
    "--output",
    metavar="<filename>",
    required=True,
    help="specifies the generated stub file",
)
args = arg_parser.parse_args()

# BEWARE! The class `Token` is inheriting from `str` so always check that an item is an
# instance of `Token` *BEFORE* checking that it is an instance of `str`.
# The code includes lots of assert's to fail on any syntax that it has not been designed
# to handle. Would that happen, the code would need to be modified to handle the new
# case.

grammar_parser = Lark.open(
    "cython.lark",
    rel_to=__file__,
    parser="lalr",
    postlex=PythonIndenter(),
    start="file_input",
)

with open(args.pyxfile, "r", encoding="utf-8") as f:
    pyx_tree = grammar_parser.parse(f.read())


def rule_name(tree: Tree) -> str:
    assert isinstance(tree, Tree)
    item_type: Token = tree.data
    assert isinstance(item_type, Token)
    assert item_type.type == "RULE"
    assert item_type.value == "name"
    assert len(tree.children) == 1
    item_name: Token = tree.children[0]
    assert isinstance(item_name, Token)
    assert item_name.type == "python__NAME"
    return item_name.value


def dotted_name(tree: Tree) -> str:
    assert isinstance(tree, Tree)
    item_type: Token = tree.data
    assert isinstance(item_type, Token)
    assert item_type.type == "RULE"
    assert item_type.value == "dotted_name"
    names: List[str] = []
    for child in tree.children:
        names.append(rule_name(child))
    return ".".join(names)


def get_constant(tree: Tree) -> str:
    assert isinstance(tree, Tree)
    assert tree.children == []
    if tree.data == "python__const_false":
        return "False"
    elif tree.data == "python__const_true":
        return "True"
    elif tree.data == "python__const_none":
        return "None"
    else:
        raise TypeError(f"Unknown constant value: {repr(tree)}")


class GenerateStub(Interpreter):
    TAB_SPACES: str = "    "
    indent: int = 0
    has_members = False

    def __init__(self, output: io.TextIOBase):
        self.output = output

    def python__dotted_as_name(self, tree: Tree) -> str:
        assert len(tree.children) == 2
        name = dotted_name(tree.children[0])
        as_name_item: Optional[Tree] = tree.children[1]
        assert as_name_item is None
        return name

    def python__dotted_as_names(self, tree: Tree) -> str:
        import_names: List[str] = []
        for child in tree.children:
            import_names.append(self.visit(child))
        return ", ".join(import_names)

    def python__import_as_name(self, tree: Tree) -> str:
        assert len(tree.children) == 2
        name = f"{rule_name(tree.children[0])}"
        as_name_item: Optional[Tree] = tree.children[1]
        if as_name_item is not None:
            name += f" as {rule_name(as_name_item)}"
        return name

    def python__import_from(self, tree: Tree):
        assert len(tree.children) == 2
        name = dotted_name(tree.children[0])
        import_names_item: Tree = tree.children[1]
        assert isinstance(import_names_item, Tree)
        import_names_type: Token = import_names_item.data
        assert isinstance(import_names_type, Token)
        assert import_names_type.type == "RULE"
        assert import_names_type.value == "import_as_names"
        import_names: List[str] = []
        for child in import_names_item.children:
            import_names.append(self.visit(child))
        self.output.write(f"from {name} import {', '.join(import_names)}\n")

    def python__import_name(self, tree: Tree) -> None:
        assert len(tree.children) == 1
        self.output.write(f"import {self.visit(tree.children[0])}\n")

    def cyclassdef(self, tree: Tree) -> None:
        self.python__classdef(tree)

    def python__var(self, tree: Tree) -> str:
        assert len(tree.children) == 1
        return rule_name(tree.children[0])

    def python__getattr(self, tree: Tree) -> List[str]:
        assert len(tree.children) >= 2
        attrs: List[str] = [self.visit(tree.children[0])]
        for child in tree.children[1:]:
            attrs.append(rule_name(child))
        return attrs

    def python__getitem(self, tree: Tree) -> str:
        assert len(tree.children) == 2
        name = self.visit(tree.children[0])
        argument: Tree = tree.children[1]
        assert isinstance(argument, Tree)
        if argument.data == "python__var":
            return f"{name}[{self.visit(argument)}]"
        elif argument.data == "python__getattr":
            return f"{name}[{'.'.join(self.visit(argument))}]"
        else:
            raise TypeError(f"Unknown argument type: {tree}")

    def get_varname(self, tree: Tree) -> str:
        if tree.data in ("python__var", "python__getitem"):
            return self.visit(tree)
        elif tree.data == "python__getattr":
            return ".".join(self.visit(tree))
        else:
            return get_constant(tree)

    def python__classdef(self, tree: Tree) -> None:
        class_name: str = ""
        for child in tree.children:
            if child is None:  # This class does not inherit
                continue

            assert isinstance(child, Tree)
            item: Token = child.data
            assert isinstance(item, Token)

            if item.value == "name":  # Class name
                class_name = rule_name(child)
            elif (
                item.value == "arguments"
            ):  # Classes that this class is inheriting from
                arguments: List[str] = []
                for argument in child.children:
                    assert isinstance(argument, Tree)
                    arguments.append(self.get_varname(argument))
                if arguments:
                    class_name += f"({', '.join(arguments)})"
            else:  # Class body
                self.output.write(f"\nclass {class_name}:")
                assert item.value == "suite"
                self.has_members = False
                self.indent += 1
                self.visit(tree.children[2])
                self.indent -= 1

                if not self.has_members:
                    self.output.write("...")
                self.output.write("\n")

    def python__typedparam(self, tree: Tree) -> Tuple[str, str]:
        assert len(tree.children) == 2
        param_name = rule_name(tree.children[0])
        assert isinstance(tree.children[1], Tree)
        param_type = self.visit(tree.children[1])
        return param_name, param_type

    def python__number(self, tree: Tree) -> str:
        assert len(tree.children) == 1
        assert isinstance(tree.children[0], Token)
        return tree.children[0].value

    def python__string(self, tree: Tree) -> str:
        assert len(tree.children) == 1
        assert isinstance(tree.children[0], Token)
        assert tree.children[0].type in ("python__STRING", "python__LONG_STRING")
        return tree.children[0].value

    def python__decorator(self, tree: Tree) -> str:
        assert len(tree.children) == 2
        assert tree.children[1] is None
        self.output.write(
            f"\n{self.TAB_SPACES*self.indent}@{dotted_name(tree.children[0])}"
        )

    def funcdef(self, tree: Tree) -> None:
        func_name: str = ""
        for i, child in enumerate(tree.children):
            if child is None:
                if i == 1:
                    func_name += "()"  # This function has no parameter
                continue

            assert isinstance(child, Tree)
            child_type: Union[str, Token] = child.data
            if isinstance(child_type, Token):
                if child_type.value == "name":  # Get the function
                    func_name = rule_name(child)
                    if func_name in ("__cinit__", "__dealloc__"):
                        return
                elif child_type.value == "cparameters":  # Get the function parameters
                    parameters: List[str] = []
                    for cparameter in child.children:
                        if cparameter is None:
                            continue

                        assert isinstance(cparameter, Tree)
                        cparam_type: Token = cparameter.data
                        assert isinstance(cparam_type, Token)
                        if cparam_type.value == "cparameter":
                            assert len(cparameter.children) == 1
                            parameter = cparameter.children[0]
                            assert isinstance(parameter, Tree)
                            if isinstance(parameter.data, Token):
                                if parameter.data == "name":
                                    parameters.append(rule_name(parameter))
                                elif parameter.data == "paramvalue":
                                    assert len(parameter.children) == 2
                                    pname, ptype = self.visit(parameter.children[0])
                                    value = parameter.children[1]
                                    assert isinstance(value, Tree)
                                    if value.data in (
                                        "python__number",
                                        "python__string",
                                    ):
                                        pvalue = self.visit(value)
                                    else:
                                        pvalue = get_constant(value)
                                    parameters.append(f"{pname}: {ptype} = {pvalue}")
                                else:
                                    raise TypeError(
                                        f"Unknown parameter type in {func_name}: {repr(parameter)}"
                                    )
                            elif (
                                isinstance(parameter.data, str)
                                and parameter.data == "python__typedparam"
                            ):
                                pname, ptype = self.visit(parameter)
                                parameters.append(f"{pname}: {ptype}")
                            else:
                                raise TypeError(
                                    f"Uknown parameter type in {func_name}: {repr(parameter)}"
                                )
                        else:
                            raise TypeError(
                                f"Unknown parameter type in {func_name}: {repr(cparameter)}"
                            )
                    func_name += f"({', '.join(parameters)})"
                else:
                    assert child_type.value == "suite"
            elif isinstance(child_type, str):
                func_name += f" -> {self.get_varname(child)}"
            else:
                raise TypeError(f"Unknown parameter in {func_name}: {repr(child)}")

        self.has_members = True
        self.output.write(f"\n{self.TAB_SPACES * self.indent}def {func_name}: ...")
        if self.indent == 0:
            self.output.write("\n")


with open(args.output, "w", encoding="utf-8") as f:
    GenerateStub(f).visit(pyx_tree)
