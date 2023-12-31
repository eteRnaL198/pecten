from typing import List

from src.customizer.method.method import Method
from src.customizer.pointcut.func_signature import FuncSignature
from src.customizer.primitive_aspect.constructor import Constructor
from src.customizer.primitive_aspect.primitive_aspect import PrimitiveAspect
from src.customizer.primitive_aspect.stringified_aspect import StringifiedAspect


class AbstractAspect(PrimitiveAspect):
    def __init__(
        self,
        name,
        constructor: Constructor,
        abstract_methods: List[FuncSignature],
        methods: List[Method],
        aspects: List[StringifiedAspect],
    ):
        """
        Args:
            name (str): アスペクト名
            constructor (Constructor): コンストラクタ
            abstract_methods (list[AbstractMethod]): 抽象メソッドのリスト
        """
        super().__init__(name, methods, aspects)
        self.constructor = constructor
        self.abstract_methods: List[FuncSignature] = abstract_methods

    def bind_token_params(self, other_constructor: Constructor):
        if len(self.constructor.args) != len(other_constructor.args):
            raise Exception(
                "Number of arguments provided does not match the number of constructor arguments. given: {}, expected: {}".format(
                    len(other_constructor.args), len(self.constructor.args)
                )
            )
        for i in range(len(other_constructor.args)):
            for method in self.methods:
                method.replace(
                    self.constructor.get_called_format(i), other_constructor.args[i]
                )
            for aspect in self.aspects:
                aspect.replace(
                    self.constructor.get_called_format(i), other_constructor.args[i]
                )
        for m in self.methods:
            print(m)

    def override_methods(self, other_methods: List[Method]):
        for method in other_methods:
            if method in self.methods:
                self.methods.remove(method)
                self.methods.append(method)
            if method.signature in self.abstract_methods:
                self.abstract_methods.remove(method.signature)
                self.methods.append(method)
        if len(self.abstract_methods) != 0:
            raise Exception(
                "The following abstract methods have not been overridden: {}".format(
                    self.abstract_methods
                ),
            )
