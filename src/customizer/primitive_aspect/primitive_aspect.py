from typing import List

from src.customizer.method.method import Method
from src.customizer.primitive_aspect.stringified_aspect import StringifiedAspect


class PrimitiveAspect:
    def __init__(
        self,
        name: str,
        methods: List[Method],
        aspects: List[StringifiedAspect],
    ):
        """
        Args:
            name (str): アスペクト名
            methods (list[Method]): メソッドのリスト
            aspects (list[StringifiedAspect]): アスペクトのリスト
        """
        self.name = name
        self.methods: List[Method] = methods
        self.aspects: List[StringifiedAspect] = aspects

    def set_name(self, name: str):
        self.name = name

    def add_aspect(self, aspect: StringifiedAspect):
        self.aspects.append(aspect)

    def stringify(self) -> str:
        return "\n".join(
            [
                "aspect {} {{".format(self.name),
                "\n".join([str(m) for m in self.methods]),
                "\n".join([str(a) for a in self.aspects]),
                "}",
            ]
        )
