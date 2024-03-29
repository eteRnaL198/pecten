from src.customizer.lang_processor.base_transformer import BaseTransformer
from src.customizer.method.method import Method
from src.customizer.pointcut.func_signature import FuncSignature
from src.customizer.primitive_aspect.abstract_aspect import AbstractAspect
from src.customizer.primitive_aspect.concrete_aspect import ConcreteAspect
from src.customizer.primitive_aspect.constructor import Constructor
from src.customizer.primitive_aspect.intermediate_aspect import IntermediateAspect
from src.customizer.primitive_aspect.primitive_aspect import PrimitiveAspect
from src.customizer.primitive_aspect.stringified_aspect import StringifiedAspect
from src.customizer.primitive_aspect.super_constructor import SuperConstructor


class InheritanceTransformer(BaseTransformer):
    ######################## aspect ########################
    def aspect_container(self, tree):
        return tree[0]

    def abstract_aspect(self, tree):
        """
        abstract aspect Foo {
            Aspects
        }
        """
        name = str(tree[0])
        constructor: Constructor = tree[1]
        abstract_methods = [t for t in tree[2:] if type(t) == FuncSignature]
        methods: list[Method] = [t for t in tree[2:] if type(t) == Method]
        aspects: list[StringifiedAspect] = [
            t for t in tree[2:] if type(t) == StringifiedAspect
        ]
        return AbstractAspect(name, constructor, abstract_methods, methods, aspects)

    def intermediate_aspect(self, tree):
        """
        abstract aspect Foo extends Bar {
            Aspects
        }
        """
        name = str(tree[0])
        super_asp_name = str(tree[1])
        super: SuperConstructor = [t for t in tree[2:] if type(t) == SuperConstructor][
            0
        ]
        constructor: Constructor = [t for t in tree[2:] if type(t) == Constructor][0]
        abstract_methods = [t for t in tree[3:] if type(t) == FuncSignature]
        methods: list[Method] = [t for t in tree[3:] if type(t) == Method]
        aspects: list[StringifiedAspect] = [
            t for t in tree[3:] if type(t) == StringifiedAspect
        ]
        return IntermediateAspect(
            name, super_asp_name, super, constructor, abstract_methods, methods, aspects
        )

    def concrete_aspect(self, tree):
        """
        aspect Foo extends Bar {
            Aspects
        }
        """
        name = str(tree[0])
        super_asp_name = str(tree[1])
        super: SuperConstructor = tree[2]
        methods: list[Method] = [t for t in tree[3:] if type(t) == Method]
        aspects: list[StringifiedAspect] = [
            t for t in tree[3:] if type(t) == StringifiedAspect
        ]
        return ConcreteAspect(name, super_asp_name, super, methods, aspects)

    def primitive_aspect(self, tree):
        """
        aspect Foo {
            Aspects
        }
        """
        name = str(tree[0])
        methods: list[Method] = [t for t in tree[1:] if type(t) == Method]
        aspects: list[StringifiedAspect] = [
            t for t in tree[1:] if type(t) == StringifiedAspect
        ]
        return PrimitiveAspect(name, methods, aspects)

    def aspect_name(self, tree):
        return str(tree[0])

    def aspect(self, tree):
        advice_type, pointcut, advice_body, end_bracket = (
            tree[0],
            str(tree[1]),
            "\n".join(tree[2]),
            str(tree[3]),
        )
        return StringifiedAspect(advice_type, pointcut, advice_body, end_bracket)

    def advice_type(self, tree):
        return str(tree[0] + "():")

    ######################## constructor ########################
    def constructor(self, tree):
        return Constructor(tree[1])

    def constructor_args(self, tree):
        """
        return:
            e.g.) ['foo', 'bar']
        """
        return [str(t) for t in tree]

    def super(self, tree):
        return SuperConstructor(tree[1])

    def super_args(self, tree):
        """
        return:
            e.g.) foo.c, "bar2" →[foo.c, "bar2"]
        """
        return tree

    def super_arg(self, tree):
        return "".join([t.value for t in tree])

    ######################## method ########################
    def abstract_method(self, tree) -> FuncSignature:
        return tree[1]
