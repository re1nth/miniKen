


import v1.v2.v3;
import v1.v2.v4;

class C1 {
    v5 v6;
    v5 v7;
    int    v8;
    float  v9;
    float  v10;

    C1(v5 v6, v5 v7,
               int v8, float v9,
               float v10) {
        this.v6         = v6;
        this.v7         = v7;
        this.v8         = v8;
        this.v9         = v9;
        this.v10        = v10;
    }
}

class C2 {
    v5             v11;
    v5             v12;
    v5             v13;
    v3<C1>         v14;
    float          v15;
    float          v16;
    v5             v17;

    C2(v5 v12, v5 v13) {
        this.v12        = v12;
        this.v13        = v13;
        this.v14        = new v4<>();
        this.v17        = "PENDING";
    }
}

boolean f1(C2 v18) {
    if (v18.v12 == null || v18.v12.v19())
        return false;
    if (v18.v13 == null || v18.v13.v19())
        return false;
    if (v18.v14 == null || v18.v14.v19())
        return false;
    return true;
}

float f2(C2 v18) {
    float v20 = 0.0f;
    for (C1 v21 : v18.v14) {
        float v22 = v21.v9 * v21.v8;
        v20 += v22;
    }
    return v20;
}

void f3(C2 v18,
        float v23) {
    float v24 = v18.v15
                * (v23 / 100.0f);
    v18.v16 =
        v18.v15 - v24;
}

void f4(C2 v18,
        v5 v25) {
    v18.v11   = v25;
    v18.v17   = "CONFIRMED";
}

void f5(C2 v18,
        v5 v26,
        v5 v27) {
    v18.v17 = "CANCELLED";
}
