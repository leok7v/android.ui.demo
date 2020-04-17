#include "rt.h" // based on https://github.com/datenwolf/linmath.h
// massaged to C99 and more consistent code style
// removed rogue inlining - depend on gcc, clang and llvm global inlining optimization instead
// made into single file hreader style library

// linmath.h -- A small library for linear math as required for computer graphics
// provides the most frequently used types required for programming computer graphics:
//
// vec2 -- 2 element vector of floats
// vec3 -- 3 element vector of floats
// vec4 -- 4 element vector of floats (4th component used for homogenous computations)
// mat4x4 -- 4 by 4 elements matrix, computations are done in column major order
// quat -- quaternion
//
// The types are *deliberately* named like the types in GLSL. In fact they are meant to
// be used for the client side computations and passing to same typed GLSL uniforms.

BEGIN_C

#ifdef LINMATH_IMPLEMENTATION
#define linmat_implements(code) code
#else
#define linmat_implements(code);
#endif

#define linmath_curly_init(...) { __VA_ARGS__ } // because cannot use {,} inside linmat_implements()

#define linmath_vec_definition(n)                                                \
                                                                                 \
typedef float vec##n[n];                                                         \
void vec##n##_add(vec##n r, vec##n const a, vec##n const b) linmat_implements({  \
    for (int i = 0; i < n; i++) { r[i] = a[i] + b[i]; }                          \
})                                                                               \
                                                                                 \
void vec##n##_sub(vec##n r, vec##n const a, vec##n const b) linmat_implements({  \
    for (int i = 0; i < n; i++) { r[i] = a[i] - b[i]; }                          \
})                                                                               \
                                                                                 \
void vec##n##_scale(vec##n r, vec##n const v, float const s) linmat_implements({ \
    for (int i = 0; i < n; i++) { r[i] = v[i] * s; }                             \
})                                                                               \
                                                                                 \
float vec##n##_mul_inner(vec##n const a, vec##n const b) linmat_implements({     \
    float p = 0;                                                                 \
    for (int i = 0; i < n; i++)                                                  \
        p += b[i] * a[i];                                                        \
    return p;                                                                    \
})                                                                               \
                                                                                 \
float vec##n##_len(vec##n const v) linmat_implements({                           \
    return sqrtf(vec##n##_mul_inner(v,v));                                       \
})                                                                               \
                                                                                 \
void vec##n##_norm(vec##n r, vec##n const v) linmat_implements({                 \
    float k = 1 / vec##n##_len(v);                                               \
    vec##n##_scale(r, v, k);                                                     \
})                                                                               \
                                                                                 \
void vec##n##_min(vec##n r, vec##n const a, vec##n const b) linmat_implements({  \
    for (int i = 0; i < n; i++) { r[i] = a[i]<b[i] ? a[i] : b[i]; }              \
})                                                                               \
                                                                                 \
void vec##n##_max(vec##n r, vec##n const a, vec##n const b) linmat_implements({  \
    for (int i = 0; i < n; i++) { r[i] = a[i]>b[i] ? a[i] : b[i]; }              \
})

linmath_vec_definition(2)
linmath_vec_definition(3)
linmath_vec_definition(4)

void vec3_mul_cross(vec3 r, vec3 const a, vec3 const b) linmat_implements({
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
})

void vec3_reflect(vec3 r, vec3 const v, vec3 const n) linmat_implements({
    float p  = 2 * vec3_mul_inner(v, n);
    for (int i = 0; i < 3; i++) { r[i] = v[i] - p*n[i]; }
})

void vec4_mul_cross(vec4 r, vec4 a, vec4 b) linmat_implements({
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
    r[3] = 1;
})

void vec4_reflect(vec4 r, vec4 v, vec4 n) linmat_implements({
    float p  = 2 * vec4_mul_inner(v, n);
    for (int i = 0; i < 4; i++) { r[i] = v[i] - p*n[i]; }
})

typedef vec4 mat4x4[4];

void mat4x4_identity(mat4x4 M) linmat_implements({
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) { M[i][j] = i==j ? 1 : 0; }
    }
})

void mat4x4_dup(mat4x4 M, mat4x4 N) linmat_implements({
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) { M[i][j] = N[i][j]; }
    }
})

void mat4x4_row(vec4 r, mat4x4 M, int i) linmat_implements({
    for (int k = 0; k < 4; k++) { r[k] = M[k][i]; }
})

void mat4x4_col(vec4 r, mat4x4 M, int i) linmat_implements({
    for (int k = 0; k < 4; k++) { r[k] = M[i][k]; }
})

void mat4x4_transpose(mat4x4 M, mat4x4 N) linmat_implements({
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) { M[i][j] = N[j][i]; }
    }
})

void mat4x4_add(mat4x4 M, mat4x4 a, mat4x4 b) linmat_implements({
    for (int i = 0; i < 4; i++) { vec4_add(M[i], a[i], b[i]); }
})

void mat4x4_sub(mat4x4 M, mat4x4 a, mat4x4 b) linmat_implements({
    for (int i = 0; i < 4; i++) { vec4_sub(M[i], a[i], b[i]); }
})

void mat4x4_scale(mat4x4 M, mat4x4 a, float k) linmat_implements({
    for (int i = 0; i < 4; i++) { vec4_scale(M[i], a[i], k); }
})

void mat4x4_scale_aniso(mat4x4 M, mat4x4 a, float x, float y, float z) linmat_implements({
    vec4_scale(M[0], a[0], x);
    vec4_scale(M[1], a[1], y);
    vec4_scale(M[2], a[2], z);
    for (int i = 0; i < 4; i++) { M[3][i] = a[3][i]; }
})

void mat4x4_mul(mat4x4 M, mat4x4 a, mat4x4 b) linmat_implements({
    mat4x4 temp;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            temp[c][r] = 0;
            for (int k=0; k < 4; k++) { temp[c][r] += a[k][r] * b[c][k]; }
        }
    }
    mat4x4_dup(M, temp);
})

void mat4x4_mul_vec4(vec4 r, mat4x4 M, vec4 v) linmat_implements({
    for (int j = 0; j < 4; j++) {
        r[j] = 0;
        for (int i = 0; i < 4; i++) { r[j] += M[i][j] * v[i]; }
    }
})

void mat4x4_translate(mat4x4 T, float x, float y, float z) linmat_implements({
    mat4x4_identity(T);
    T[3][0] = x;
    T[3][1] = y;
    T[3][2] = z;
})

void mat4x4_translate_in_place(mat4x4 M, float x, float y, float z) linmat_implements({
    vec4 t = linmath_curly_init(x, y, z, 0);
    vec4 r;
    for (int i = 0; i < 4; i++) {
        mat4x4_row(r, M, i);
        M[3][i] += vec4_mul_inner(r, t);
    }
})

void mat4x4_from_vec3_mul_outer(mat4x4 M, vec3 a, vec3 b) linmat_implements({
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            M[i][j] = i<3 && j<3 ? a[i] * b[j] : 0;
        }
    }
})

void mat4x4_rotate(mat4x4 R, mat4x4 M, float x, float y, float z, float angle) linmat_implements({
    float s = sinf(angle);
    float c = cosf(angle);
    vec3 u = linmath_curly_init(x, y, z);
    if (vec3_len(u) > 1e-4) {
        vec3_norm(u, u);
        mat4x4 T;
        mat4x4_from_vec3_mul_outer(T, u, u);
        mat4x4 S = linmath_curly_init(
            {    0,  u[2], -u[1], 0},
            {-u[2],     0,  u[0], 0},
            { u[1], -u[0],     0, 0},
            {    0,     0,     0, 0}
        );
        mat4x4_scale(S, S, s);
        mat4x4 C;
        mat4x4_identity(C);
        mat4x4_sub(C, C, T);
        mat4x4_scale(C, C, c);
        mat4x4_add(T, T, C);
        mat4x4_add(T, T, S);
        T[3][3] = 1;
        mat4x4_mul(R, M, T);
    } else {
        mat4x4_dup(R, M);
    }
})

void mat4x4_rotate_X(mat4x4 Q, mat4x4 M, float angle) linmat_implements({
    float s = sinf(angle);
    float c = cosf(angle);
    mat4x4 R = linmath_curly_init(
        {1,  0, 0, 0},
        {0,  c, s, 0},
        {0, -s, c, 0},
        {0,  0, 0, 1}
    );
    mat4x4_mul(Q, M, R);
})

void mat4x4_rotate_Y(mat4x4 Q, mat4x4 M, float angle) linmat_implements({
    float s = sinf(angle);
    float c = cosf(angle);
    mat4x4 R = linmath_curly_init(
        { c, 0, s, 0},
        { 0, 1, 0, 0},
        {-s, 0, c, 0},
        { 0, 0, 0, 1}
    );
    mat4x4_mul(Q, M, R);
})

void mat4x4_rotate_Z(mat4x4 Q, mat4x4 M, float angle) linmat_implements({
    float s = sinf(angle);
    float c = cosf(angle);
    mat4x4 R = linmath_curly_init(
        { c, s, 0, 0},
        {-s, c, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 0, 1}
    );
    mat4x4_mul(Q, M, R);
})

void mat4x4_invert(mat4x4 T, mat4x4 M) linmat_implements({
    float s[6];
    float c[6];
    s[0] = M[0][0] * M[1][1] - M[1][0] * M[0][1];
    s[1] = M[0][0] * M[1][2] - M[1][0] * M[0][2];
    s[2] = M[0][0] * M[1][3] - M[1][0] * M[0][3];
    s[3] = M[0][1] * M[1][2] - M[1][1] * M[0][2];
    s[4] = M[0][1] * M[1][3] - M[1][1] * M[0][3];
    s[5] = M[0][2] * M[1][3] - M[1][2] * M[0][3];

    c[0] = M[2][0] * M[3][1] - M[3][0] * M[2][1];
    c[1] = M[2][0] * M[3][2] - M[3][0] * M[2][2];
    c[2] = M[2][0] * M[3][3] - M[3][0] * M[2][3];
    c[3] = M[2][1] * M[3][2] - M[3][1] * M[2][2];
    c[4] = M[2][1] * M[3][3] - M[3][1] * M[2][3];
    c[5] = M[2][2] * M[3][3] - M[3][2] * M[2][3];

    // Assumes it is invertible
    float idet = 1 / ( s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] - s[4] * c[1] + s[5] * c[0]);

    T[0][0] = ( M[1][1] * c[5] - M[1][2] * c[4] + M[1][3] * c[3]) * idet;
    T[0][1] = (-M[0][1] * c[5] + M[0][2] * c[4] - M[0][3] * c[3]) * idet;
    T[0][2] = ( M[3][1] * s[5] - M[3][2] * s[4] + M[3][3] * s[3]) * idet;
    T[0][3] = (-M[2][1] * s[5] + M[2][2] * s[4] - M[2][3] * s[3]) * idet;

    T[1][0] = (-M[1][0] * c[5] + M[1][2] * c[2] - M[1][3] * c[1]) * idet;
    T[1][1] = ( M[0][0] * c[5] - M[0][2] * c[2] + M[0][3] * c[1]) * idet;
    T[1][2] = (-M[3][0] * s[5] + M[3][2] * s[2] - M[3][3] * s[1]) * idet;
    T[1][3] = ( M[2][0] * s[5] - M[2][2] * s[2] + M[2][3] * s[1]) * idet;

    T[2][0] = ( M[1][0] * c[4] - M[1][1] * c[2] + M[1][3] * c[0]) * idet;
    T[2][1] = (-M[0][0] * c[4] + M[0][1] * c[2] - M[0][3] * c[0]) * idet;
    T[2][2] = ( M[3][0] * s[4] - M[3][1] * s[2] + M[3][3] * s[0]) * idet;
    T[2][3] = (-M[2][0] * s[4] + M[2][1] * s[2] - M[2][3] * s[0]) * idet;

    T[3][0] = (-M[1][0] * c[3] + M[1][1] * c[1] - M[1][2] * c[0]) * idet;
    T[3][1] = ( M[0][0] * c[3] - M[0][1] * c[1] + M[0][2] * c[0]) * idet;
    T[3][2] = (-M[3][0] * s[3] + M[3][1] * s[1] - M[3][2] * s[0]) * idet;
    T[3][3] = ( M[2][0] * s[3] - M[2][1] * s[1] + M[2][2] * s[0]) * idet;
})

void mat4x4_orthonormalize(mat4x4 R, mat4x4 M) linmat_implements({
    mat4x4_dup(R, M);
    float s = 1;
    vec3 h;
    vec3_norm(R[2], R[2]);
    s = vec3_mul_inner(R[1], R[2]);
    vec3_scale(h, R[2], s);
    vec3_sub(R[1], R[1], h);
    vec3_norm(R[1], R[1]);
    s = vec3_mul_inner(R[0], R[2]);
    vec3_scale(h, R[2], s);
    vec3_sub(R[0], R[0], h);
    s = vec3_mul_inner(R[0], R[1]);
    vec3_scale(h, R[1], s);
    vec3_sub(R[0], R[0], h);
    vec3_norm(R[0], R[0]);
})

void mat4x4_frustum(mat4x4 M, float l, float r, float b, float t, float n, float f) linmat_implements({
    M[0][0] = 2 * n / (r - l);
    M[0][1] = M[0][2] = M[0][3] = 0;

    M[1][1] = 2 * n / (t - b);
    M[1][0] = M[1][2] = M[1][3] = 0;

    M[2][0] =  (r + l)/(r - l);
    M[2][1] =  (t + b)/(t - b);
    M[2][2] = -(f + n)/(f - n);
    M[2][3] = -1;

    M[3][2] = -2*(f*n)/(f-n);
    M[3][0] = M[3][1] = M[3][3] = 0;
})

void mat4x4_ortho(mat4x4 M, float l, float r, float b, float t, float n, float f) linmat_implements({
    M[0][0] = 2/(r-l);
    M[0][1] = M[0][2] = M[0][3] = 0;

    M[1][1] = 2/(t-b);
    M[1][0] = M[1][2] = M[1][3] = 0;

    M[2][2] = -2/(f-n);
    M[2][0] = M[2][1] = M[2][3] = 0;

    M[3][0] = -(r+l)/(r-l);
    M[3][1] = -(t+b)/(t-b);
    M[3][2] = -(f+n)/(f-n);
    M[3][3] = 1;
})

void mat4x4_perspective(mat4x4 m, float y_fov, float aspect, float n, float f) linmat_implements({
    // NOTE: Degrees are an unhandy unit to work with.
    // linmath.h uses radians for everything!
    float const a = 1 / tan(y_fov / 2);
    m[0][0] = a / aspect;
    m[0][1] = 0;
    m[0][2] = 0;
    m[0][3] = 0;

    m[1][0] = 0;
    m[1][1] = a;
    m[1][2] = 0;
    m[1][3] = 0;

    m[2][0] = 0;
    m[2][1] = 0;
    m[2][2] = -((f + n) / (f - n));
    m[2][3] = -1;

    m[3][0] = 0;
    m[3][1] = 0;
    m[3][2] = -((2 * f * n) / (f - n));
    m[3][3] = 0;
})

void mat4x4_look_at(mat4x4 m, vec3 eye, vec3 center, vec3 up) linmat_implements({
    // Adapted from Android's OpenGL Matrix.java.
    // See the OpenGL GLUT documentation for gluLookAt for a description
    // of the algorithm. We implement it in a straightforward way:
    // TODO: The negations can be spared by swapping the order of
    //       operands in the following cross products in the right way.
    vec3 f;
    vec3_sub(f, center, eye);
    vec3_norm(f, f);
    vec3 s;
    vec3_mul_cross(s, f, up);
    vec3_norm(s, s);
    vec3 t;
    vec3_mul_cross(t, s, f);
    m[0][0] =  s[0];
    m[0][1] =  t[0];
    m[0][2] = -f[0];
    m[0][3] =  0;

    m[1][0] =  s[1];
    m[1][1] =  t[1];
    m[1][2] = -f[1];
    m[1][3] =  0;

    m[2][0] =  s[2];
    m[2][1] =  t[2];
    m[2][2] = -f[2];
    m[2][3] =  0;

    m[3][0] =  0;
    m[3][1] =  0;
    m[3][2] =  0;
    m[3][3] =  1;
    mat4x4_translate_in_place(m, -eye[0], -eye[1], -eye[2]);
})

typedef float quat[4];

void quat_identity(quat q) linmat_implements({
    q[0] = q[1] = q[2] = 0;
    q[3] = 1;
})

void quat_add(quat r, quat a, quat b) linmat_implements({
    for (int i = 0; i < 4; i++) { r[i] = a[i] + b[i]; }
})

void quat_sub(quat r, quat a, quat b) linmat_implements({
    for (int i = 0; i < 4; i++) { r[i] = a[i] - b[i]; }
})

void quat_mul(quat r, quat p, quat q) linmat_implements({
    vec3 w;
    vec3_mul_cross(r, p, q);
    vec3_scale(w, p, q[3]);
    vec3_add(r, r, w);
    vec3_scale(w, q, p[3]);
    vec3_add(r, r, w);
    r[3] = p[3] * q[3] - vec3_mul_inner(p, q);
})

void quat_scale(quat r, quat v, float s) linmat_implements({
    for (int i = 0; i < 4; i++) { r[i] = v[i] * s; }
})

float quat_inner_product(quat a, quat b) linmat_implements({
    float p = 0;
    for (int i = 0; i < 4; i++) { p += b[i] * a[i]; }
    return p;
})

void quat_conj(quat r, quat q) linmat_implements({
    for (int i = 0; i < 3; i++) { r[i] = -q[i]; }
    r[3] = q[3];
})

void quat_rotate(quat r, float angle, vec3 axis) linmat_implements({
    vec3 v;
    vec3_scale(v, axis, sinf(angle / 2));
    for (int i = 0; i < 3; i++) { r[i] = v[i]; }
    r[3] = cosf(angle / 2);
})

#define quat_norm vec4_norm

void quat_mul_vec3(vec3 r, quat q, vec3 v) linmat_implements({
    // Method by Fabian 'ryg' Giessen (of Farbrausch)
    // t = 2 * cross(q.xyz, v)
    // v' = v + q.w * t + cross(q.xyz, t)
    vec3 q_xyz = linmath_curly_init(q[0], q[1], q[2]);
    vec3 u = linmath_curly_init(q[0], q[1], q[2]);
    vec3 t;
    vec3_mul_cross(t, q_xyz, v);
    vec3_scale(t, t, 2);
    vec3_mul_cross(u, q_xyz, t);
    vec3_scale(t, t, q[3]);
    vec3_add(r, v, t);
    vec3_add(r, r, u);
})

void mat4x4_from_quat(mat4x4 M, quat q) linmat_implements({
    float a = q[3];
    float b = q[0];
    float c = q[1];
    float d = q[2];
    float a2 = a * a;
    float b2 = b * b;
    float c2 = c * c;
    float d2 = d * d;
    M[0][0] = a2 + b2 - c2 - d2;
    M[0][1] = 2 * (b * c + a * d);
    M[0][2] = 2 * (b * d - a * c);
    M[0][3] = 0;

    M[1][0] = 2 * (b * c - a * d);
    M[1][1] = a2 - b2 + c2 - d2;
    M[1][2] = 2 * (c * d + a * b);
    M[1][3] = 0;

    M[2][0] = 2 * (b * d + a * c);
    M[2][1] = 2 * (c * d - a * b);
    M[2][2] = a2 - b2 - c2 + d2;
    M[2][3] = 0;

    M[3][0] = M[3][1] = M[3][2] = 0;
    M[3][3] = 1;
})

void mat4x4o_mul_quat(mat4x4 R, mat4x4 M, quat q) linmat_implements({
    //  XXX: The way this is written only works for othogonal matrices.
    // TODO: Take care of non-orthogonal case.
    quat_mul_vec3(R[0], q, M[0]);
    quat_mul_vec3(R[1], q, M[1]);
    quat_mul_vec3(R[2], q, M[2]);
    R[3][0] = R[3][1] = R[3][2] = 0;
    R[3][3] = 1;
})

void quat_from_mat4x4(quat q, mat4x4 M) linmat_implements({
    float r = 0;
    int perm[] = linmath_curly_init(0, 1, 2, 0, 1);
    int *p = perm;
    for (int i = 0; i < 3; i++) {
        float m = M[i][i];
        if (m < r) {
            continue;
        }
        m = r;
        p = &perm[i];
    }
    r = sqrtf(1 + M[p[0]][p[0]] - M[p[1]][p[1]] - M[p[2]][p[2]]);
    if (r < 1e-6) {
        q[0] = 1;
        q[1] = q[2] = q[3] = 0;
    } else {
        q[0] = r / 2;
        q[1] = (M[p[0]][p[1]] - M[p[1]][p[0]]) / (2 * r);
        q[2] = (M[p[2]][p[0]] - M[p[0]][p[2]]) / (2 * r);
        q[3] = (M[p[2]][p[1]] - M[p[1]][p[2]]) / (2 * r);
    }
})

void mat4x4_arcball(mat4x4 R, mat4x4 M, vec2 _a, vec2 _b, float s) linmat_implements({
    vec2 a; memcpy(a, _a, sizeof(a));
    vec2 b; memcpy(b, _b, sizeof(b));
    float z_a = 0;
    float z_b = 0;
    if (vec2_len(a) < 1) {
        z_a = sqrtf(1 - vec2_mul_inner(a, a));
    } else {
        vec2_norm(a, a);
    }
    if (vec2_len(b) < 1) {
        z_b = sqrtf(1 - vec2_mul_inner(b, b));
    } else {
        vec2_norm(b, b);
    }
    vec3 a_ = linmath_curly_init(a[0], a[1], z_a);
    vec3 b_ = linmath_curly_init(b[0], b[1], z_b);
    vec3 c_;
    vec3_mul_cross(c_, a_, b_);
    float const angle = acos(vec3_mul_inner(a_, b_)) * s;
    mat4x4_rotate(R, M, c_[0], c_[1], c_[2], angle);
})

END_C
