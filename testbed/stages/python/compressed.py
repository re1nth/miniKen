



from v1 import v2, v3
from v4 import v5


@v2
class C1:
    v6: v7
    v8: v7
    v9: v10
    v11: v12
    v13: v12 = 0.0


@v2
class C2:
    v14: v7
    v15: v7
    v16: v7
    v17: v5[C1] = v3(v18=v19)
    v20: v12 = 0.0
    v21: v12 = 0.0
    v22: v7 = "PENDING"


def f1(v23: C2) -> v24:
    if not v23.v15:
        return False
    if not v23.v16:
        return False
    if not v23.v17:
        return False
    return True


def f2(v23: C2) -> v12:
    v25 = 0.0
    for v26 in v23.v17:
        v27 = v26.v11 * v26.v9
        v25 += v27
    return v25


def f3(
    v23: C2,
    v28: v12,
) -> None:
    v29 = (
        v23.v20
        * (v28 / 100.0)
    )
    v23.v21 = (
        v23.v20 - v29
    )


def f4(
    v23: C2,
    v30: v7,
) -> None:
    v23.v14 = v30
    v23.v22 = "CONFIRMED"


def f5(
    v23: C2,
    v31: v7,
    v32: v7,
) -> None:
    v23.v22 = "CANCELLED"
    _ = v31
    _ = v32
