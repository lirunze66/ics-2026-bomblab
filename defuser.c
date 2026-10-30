#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};

    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t lo = (uint32_t)challenge;

    uint32_t step0 = (hi ^ UINT32_C(0xb1dd)) + UINT32_C(4) * lo;
    uint32_t step1 = (UINT32_C(5) * hi + mid) ^ UINT32_C(0xd395);
    uint32_t step2 = (lo + UINT32_C(0x6492)) ^ step1;

    values[0] = step2 ^ step0;
    size_t count = 1;

    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};

    values[0] = UINT32_C(3) *
                (uint32_t)((challenge >> 37) & UINT64_C(0x7f));

    for (uint32_t k = 1; k < 6; ++k) {
        uint32_t recurrence = UINT32_C(3) * values[k - 1] - k;
        uint32_t challenge_part =
            (uint32_t)((challenge >> (8 + 3 * k)) & UINT64_C(0xf)) +
            UINT32_C(11);

        values[k] = recurrence ^ challenge_part;
    }

    size_t count = 6;
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t selector = (uint32_t)(challenge & UINT64_C(7));
    uint32_t v = (uint32_t)(challenge >> 32) ^ (uint32_t)(challenge >> 3);
    uint32_t result;

    values[0] = selector;

    switch (selector) {
        case 0:
            result = (v ^ UINT32_C(0xa92a)) + (v >> 1);
            break;
        case 1:
            result = UINT32_C(5) * v + UINT32_C(0x2e644);
            break;
        case 2:
            result = UINT32_C(5) * v + UINT32_C(0x2556);
            break;
        case 3:
            result = (UINT32_C(4) * v) ^ UINT32_C(0xd000);
            break;
        case 4:
            result = UINT32_C(5) * v + UINT32_C(0x923f);
            break;
        case 5:
            result = (v ^ UINT32_C(0x3aa6)) + (v >> 1);
            break;
        case 6:
            result = (UINT32_C(4) * v) ^ UINT32_C(0x9e8e);
            break;
        case 7:
            result = UINT32_C(5) * v + UINT32_C(0x2b16a);
            break;
        default:
            return 1;
    }

    values[1] = result;
    return write_answer(out, cap, values, 2);
}

static uint32_t p4_walk_c(uint32_t target,
                          uint32_t low,
                          uint32_t high,
                          uint32_t salt) {
    uint32_t mid = low + ((high - low) >> 1);

    if (target == mid) {
        return (mid + UINT32_C(0xeb)) ^ salt;
    }

    uint32_t branch = mid + salt;

    if (target < mid) {
        uint32_t result = p4_walk_c(target, low, mid - 1, salt);
        return (UINT32_C(3) * result ^ branch) ^ UINT32_C(0xd);
    } else {
        uint32_t result = p4_walk_c(target, mid + 1, high, salt);
        return (UINT32_C(3) * result ^ branch) ^ UINT32_C(0x13);
    }
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};

    uint32_t target = (uint32_t)(challenge & UINT64_C(0x7f));
    uint32_t salt = (uint32_t)(challenge >> 7) ^
                    (uint32_t)(challenge >> 32);

    values[0] = p4_walk_c(target, 0, UINT32_C(0x7f), salt);

    return write_answer(out, cap, values, 1);
}

static uint32_t p5_solve(uint64_t challenge, uint32_t values[2]) {
    static const uint8_t heads[8] = {5, 9, 8, 0, 2, 1, 7, 6};
    static const uint8_t next_node[12] = {
        2, 7, 1, 11, 0xff, 9, 10, 6, 0, 8, 3, 4
    };
    static const uint32_t field8[12] = {
        0x0b85, 0x072c, 0x0933, 0x042c, 0x0155, 0x0333,
        0x0883, 0x0685, 0x000d, 0x027d, 0x050f, 0x0a80
    };
    static const uint32_t field12[12] = {
        0xcd26, 0x2e17, 0x388d, 0x1e32, 0xd238, 0xd25f,
        0x085d, 0xf503, 0x5209, 0x782a, 0x7f39, 0x6fbd
    };

    uint32_t node = heads[(uint32_t)challenge & 7];
    uint32_t steps = ((uint32_t)(challenge >> 3)) & 3;
    uint32_t acc = (uint32_t)(challenge >> 6) ^
                   (uint32_t)(challenge >> 32) ^
                   UINT32_C(0x2f02);

    for (uint32_t i = 0; i <= steps; ++i) {
        acc = (acc + field12[node]) * UINT32_C(5);
        acc ^= field8[node];

        if (i < steps) {
            node = next_node[node];
        }
    }

    values[0] = acc;
    values[1] = field8[node];
    return 0;
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    p5_solve(challenge, values);
    return write_answer(out, cap, values, 2);
}

static uint32_t p6_apply(uint32_t index, uint32_t x) {
    switch (index) {
        case 0:  /* p6_op_5: 32-bit rotate left 3, then xor */
        case 6:
            return ((x << 3) | (x >> 29)) ^ UINT32_C(0x61a6);

        case 1:  /* p6_op_2 */
            return (x ^ (x >> 2)) + UINT32_C(0x4429);

        case 2:  /* p6_op_0: 32-bit rotate right 5 */
        case 5:
            return (x >> 5) | (x << 27);

        case 3:  /* p6_op_4 */
            return (x ^ UINT32_C(0x4410)) + (x << 5);

        case 4:  /* p6_op_1 */
            return (UINT32_C(8) * x + UINT32_C(0x7e68)) ^
                   (UINT32_C(7) * x);

        case 7:  /* p6_op_3 */
            return (x + UINT32_C(0x2f0a)) ^ (x >> 5);

        default:
            return 0;
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};

    uint32_t first_arg = (uint32_t)(challenge >> 6) ^
                         (uint32_t)(challenge >> 32);
    uint32_t first = p6_apply((uint32_t)(challenge & UINT64_C(7)),
                              first_arg);

    uint32_t second_arg = first ^ UINT32_C(0xd7b4);
    uint32_t second = p6_apply(
        (uint32_t)((challenge >> 3) & UINT64_C(7)),
        second_arg
    );

    values[0] = second + UINT32_C(0x100e);
    return write_answer(out, cap, values, 1);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t c = (uint32_t)context & UINT32_C(3);
    uint32_t a = (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;
    uint32_t b = (uint32_t)(context >> 32) ^ (uint32_t)context;
    uint32_t t = (uint32_t)(context >> 16);
    uint32_t u = a ^ b;

    uint32_t mixed = (u << 1) | (u >> 31);
    mixed = (mixed + UINT32_C(0x197)) ^ UINT32_C(0x643f);

    uint32_t second = mixed ^ (t ^ (c * UINT32_C(0x197)));
    uint32_t third = second ^
                     ((a >> 1) + b + c + UINT32_C(0x643f));

    int size = snprintf(
        out,
        cap,
        "AAAAAAAA" "AAAAAAAA" "AAAAAAAA" "AAAAAAAA"
        "OPEN%" PRIu32 " %" PRIu32 " %" PRIu32,
        c,
        second,
        third
    );

    return size < 0 || (size_t)size >= cap;
}
