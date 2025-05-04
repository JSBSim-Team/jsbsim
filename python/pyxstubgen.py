import sys
from typing import List, Union, Tuple

from lark import Lark
from lark.indenter import PythonIndenter
from lark.lexer import Token
from lark.tree import Tree
from lark.visitors import Interpreter

parser = Lark.open(
    "cython.lark", parser="lalr", postlex=PythonIndenter(), start="file_input"
)

with open("jsbsim.pyx.in", "r", encoding="utf-8") as f:
    pyx_tree = parser.parse(f.read())


def rule_name(tree: Tree) -> str:
    item_type: Token = tree.data
    assert isinstance(item_type, Token)
    assert item_type.type == "RULE"
    assert item_type.value == "name"
    assert len(tree.children) == 1
    item_name: Token = tree.children[0]
    assert isinstance(item_name, Token)
    assert item_name.type == "python__NAME"
    return item_name.value


def get_constant(tree: Tree) -> str:
    if tree.children != []:
        raise TypeError(f"Not a constant: {repr(tree)}")
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

    def cyclassdef(self, tree: Tree) -> None:
        return self.python__classdef(tree)

    def python__var(self, tree: Tree) -> str:
        assert len(tree.children) == 1
        var: Tree = tree.children[0]
        assert isinstance(var, Tree)
        return rule_name(var)

    def python__getattr(self, tree: Tree) -> List[str]:
        assert len(tree.children) >= 2
        attrs: List[str] = [self.visit(tree.children[0])]
        for child in tree.children[1:]:
            assert isinstance(child, Tree)
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
                print(f"class {class_name}: ...")
                assert item.value == "suite"
                self.indent += 1
                # Look for the class methods
                self.visit(tree.children[2])
                self.indent -= 1
                print("")

    def python__typedparam(self, tree: Tree) -> Tuple[str, str]:
        assert len(tree.children) == 2
        assert isinstance(tree.children[0], Tree)
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

    def python__decorator(self, tree:Tree) -> str:
        assert len(tree.children) == 2
        assert tree.children[1] is None
        deco_name:Tree = tree.children[0]
        assert isinstance(deco_name, Tree)
        deco_type:Token = deco_name.data
        assert isinstance(deco_type, Token)
        assert deco_type.type == "RULE"
        assert deco_type.value == "dotted_name"
        names:List[str] = []
        for child in deco_name.children:
            names.append(rule_name(child))
        print(f"{self.TAB_SPACES*self.indent}@{'.'.join(names)}")

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
        print(f"{self.TAB_SPACES*self.indent}def {func_name}: ...")
        if self.indent == 0:
            print("")


GenerateStub().visit(pyx_tree)
