


class C1 {
    constructor(v1, v2,
                v3, v4,
                v5 = 0.0) {
        this.v1 = v1;
        this.v2 = v2;
        this.v3 = v3;
        this.v4 = v4;
        this.v5 = v5;
    }
}


class C2 {
    constructor(v6, v7) {
        this.v8  = '';
        this.v6  = v6;
        this.v7  = v7;
        this.v9  = [];
        this.v10 = 0.0;
        this.v11 = 0.0;
        this.v12 = 'PENDING';
    }
}


function f1(v13) {
    if (!v13.v6) return false;
    if (!v13.v7) return false;
    if (!v13.v9 || v13.v9.v14 === 0) return false;
    return true;
}


function f2(v13) {
    let v15 = 0.0;
    for (const v16 of v13.v9) {
        const v17 = v16.v4 * v16.v3;
        v15 += v17;
    }
    return v15;
}


function f3(v13, v18) {
    const v19 = v13.v10
                * (v18 / 100.0);
    v13.v11 =
        v13.v10 - v19;
}


function f4(v13, v20) {
    v13.v8  = v20;
    v13.v12 = 'CONFIRMED';
}


function f5(v13, v21,
            v22) {
    v13.v12 = 'CANCELLED';
    void v21;
    void v22;
}

v23.v24 = {
    C1,
    C2,
    f1,
    f2,
    f3,
    f4,
    f5,
};
