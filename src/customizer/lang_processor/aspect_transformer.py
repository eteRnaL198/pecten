from src.customizer.aspect.advice import Advice
from src.customizer.aspect.aspect import Aspect
from src.customizer.aspect.basic_aspect import BasicAspect
from src.customizer.lang_processor.base_transformer import BaseTransformer
from src.customizer.method.method import Method
from src.customizer.pointcut.execution import Execution
from src.customizer.pointcut.infile import Infile
from src.customizer.pointcut.set import Set


class AspectTransformer(BaseTransformer):
    ######################## aspect ########################
    def basic_aspect(self, tree):
        """aspect Baz {
            Aspects
        }"""
        name = str(tree[0])
        methods = [t for t in tree[1:] if type(t) == Method]
        aspects = [t for t in tree[1:] if type(t) == Aspect]
        return BasicAspect(name, methods, aspects)

    def aspect_name(self, tree):
        return str(tree[0])

    def aspect(self, tree):
        """
        tree[str, Pointcut, list[str]]
        """
        advice_type, pointcut, advice_body = tree[0], tree[1], "".join(tree[2])
        return Aspect(pointcut, Advice(advice_type, advice_body))

    ######################## advice ########################
    def advice_type(self, tree):
        return tree[0]

    ######################## pointcut ########################
    def pointcut(self, tree):
        """
        Return:
            e.g.) [Call, Execution, etc...]
        """
        primitive_pointcuts = [*tree]
        return primitive_pointcuts

    def primitive_pointcut(self, tree):
        """
        Return:
            e.g.) Call or Execution or etc...
        """
        return tree[0]

    def execution(self, tree):
        return Execution(tree[0])

    def set(self, tree):
        return Set(str(tree[0]))

    def infile(self, tree):
        return Infile(str(tree[0]))
