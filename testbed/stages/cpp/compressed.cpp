



#include <string>
#include <vector>
#include <stdexcept>


struct C1 {
    v1::v2 v3;
    v1::v2 v4;
    int         v5;
    float       v6;
    float       v7;
};


struct C2 {
    v1::v2                v8;
    v1::v2                v9;
    v1::v2                v10;
    v1::v11<C1> v12;
    float                      v13;
    float                      v14;
    v1::v2                v15;
};



bool f1(C2 v16) {
    if (v16.v9.v17()) return false;
    if (v16.v10.v17()) return false;
    if (v16.v12.v17())          return false;
    return true;
}


float f2(C2 v16) {
    float v18 = 0.0f;
    for (auto& v19 : v16.v12) {
        float v20 = v19.v6
                               * v19.v5;
        v18 += v20;
    }
    return v18;
}



void f3(C2& v16,
                                     float v21) {
    float v22 = v16.v13
                                 * (v21 / 100.0f);
    v16.v14 =
        v16.v13 - v22;
}



void f4(C2& v16,
                                     v1::v2 v23) {
    v16.v8   = v23;
    v16.v15 = "CONFIRMED";
}



void f5(C2& v16,
                                   v1::v2 v24,
                                   v1::v2 v25) {
    v16.v15 = "CANCELLED";
    (void)v24;
    (void)v25;
}
