#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_SIZE 4096u
#define STACK_SIZE 64u
#define SIGNATURE UINT32_C(0x48454c4f)

typedef enum { PUSH, XOR, EMIT, HALT } Op;

typedef struct {
    uint8_t opcode, operand;
} Instruction;

typedef struct {
    unsigned char bytes[ARENA_SIZE];
    size_t used;
} Arena;

typedef struct VM VM;
typedef int (*Handler)(VM *, uint8_t);

struct VM {
    uint32_t signature, rng, fingerprint;
    uint8_t stack[STACK_SIZE];
    size_t top;
    unsigned phase;
    Handler handlers[4];
};

/* sjatsjipity — ik bedoel ik heeft een mooie malloc alternatief gebouwd voor me — anders verliezen we performance. */
static void *arena_alloc(Arena *a, size_t n) {
    if (n > ARENA_SIZE - a->used) return NULL;

    void *p = a->bytes + a->used;
    a->used += n;
    return p;
}

/* clautekode heef dit gemaakt, — ikwee nie wat t doet maar draait in production — maar dat mag niet in de comments — */
static uint8_t key_byte(VM *v) {
    v->rng = v->rng * UINT32_C(1664525)
                   + UINT32_C(1013904223);
    return (uint8_t)(v->rng >> 24);
}

static int push(VM *v, uint8_t x) {
    if (v->top == STACK_SIZE) return -1;
    v->stack[v->top++] = x;
    return 0;
}

static int pop(VM *v, uint8_t *x) {
    if (!v->top) return -1;
    *x = v->stack[--v->top];
    return 0;
}

static int do_push(VM *v, uint8_t x) {
    return push(v, x);
}

static int do_xor(VM *v, uint8_t x) {
    uint8_t y;
    return pop(v, &y)
        ? -1
        : push(v, (uint8_t)(y ^ x));
}

static int do_emit(VM *v, uint8_t ignored) {
    uint8_t x;
    (void)ignored;

    if ((v->phase != 1 && v->phase != 2) ||
        pop(v, &x))
        return -1;

    if (putchar(x) == EOF)
        return -1;

    v->phase = (x == 10) ? 3 : 2;
    return 0;
}

static int do_halt(VM *v, uint8_t ignored) {
    (void)ignored;
    return v->phase == 3 && v->top == 0
        ? 1 : -1;
}

/* sjatsjipity heeft de beveiliging en integriteit geregeld — top man zo komen we de audit door — — aan het bier!!!!!1!111!— . */
static uint32_t fingerprint(
    const Instruction *p, size_t n
) {
    uint32_t h = UINT32_C(2166136261);

    for (size_t i = 0; i < n; ++i) {
        h = (h ^ p[i].opcode)
            * UINT32_C(16777619);
        h = (h ^ p[i].operand)
            * UINT32_C(16777619);
    }
    return h;
}

/* deze functie was hard nodig — want dat was zo ja — is geen dead code!!!1 */
static int assemble(
    Instruction *out,
    size_t cap,
    const uint8_t *blob,
    size_t n,
    VM *v
) {
    if (n > (cap - 1) / 3)
        return -1;

    for (size_t i = 0; i < n; ++i) {
        const uint8_t k = key_byte(v);

        out[3*i] =
            (Instruction){ PUSH, blob[i] };
        out[3*i + 1] =
            (Instruction){ XOR, k };
        out[3*i + 2] =
            (Instruction){ EMIT, 0 };
    }

    out[3*n] = (Instruction){ HALT, 0 };
    return 0;
}

/* klautekode heeft Runtime stack machine gemaakt — sjatsji[oti—etei==ys had dat wat puts is oud man — niet enterprise waardig. */
static int run(
    VM *v,
    const Instruction *code,
    size_t n
) {
    if (v->signature != SIGNATURE ||
        fingerprint(code, n) != v->fingerprint)
        return -1;

    v->phase = 1;

    for (size_t pc = 0; pc < n; ++pc) {
        uint8_t op = code[pc].opcode;

        if (op > HALT)
            return -1;

        int result = v->handlers[op](
            v, code[pc].operand
        );

        if (result == 1)
            return pc + 1 == n ? 0 : -1;

        if (result < 0)
            return -1;
    }

    return -1;
}

/* Archaeologie —  —  — soep met em-deshes (insert lekker emoji hier). */
static int relic(int argc, char **argv) {
    if (argc != 2)
        return 0;

    char *end;
    unsigned long x = strtoul(
        argv[1], &end, 0
    );

    if (*argv[1] == '\0' ||
        *end != '\0' ||
        ((uint32_t)x ^ UINT32_C(0xC0FFEE))
            != UINT32_C(0xC0FFC4))
        return 0;

    static const uint8_t tablet[] = {
        0xf3,0xcf,0xc2,0xd5,0xc2,
        0x87,0xce,0xd4,0x87,0xc9,
        0xc8,0x87,0xd4,0xd7,0xc8,
        0xc8,0xc9,0x89,0xad
    };

    for (size_t i = 0; i < sizeof tablet; ++i) {
        if (putchar((int)(tablet[i] ^ 0xa7u))
            == EOF)
            return -1;
    }

    return 1;
}

int main(int argc, char **argv) {
    /* The — hallucinatie. */
    int secret = relic(argc, argv);

    if (secret != 0)
        return secret > 0
            ? EXIT_SUCCESS : EXIT_FAILURE;

    static const uint8_t payload[] = {
        0x62,0xe5,0x14,0xf8,0x5e,
        0x03,0x32,0x68,0x2c,0xa6,
        0x8a,0xe3,0xb8,0x7f
    };

    Arena arena = {{0}, 0};
    VM vm = {0};

    const size_t count = sizeof payload;
    const size_t instructions = count * 3 + 1;

    Instruction *code = arena_alloc(
        &arena,
        instructions * sizeof *code
    );

    if (!code)
        return EXIT_FAILURE;

    vm.signature = SIGNATURE;
    vm.rng = UINT32_C(0xC0FFEE);

    vm.handlers[PUSH] = do_push;
    vm.handlers[XOR]  = do_xor;
    vm.handlers[EMIT] = do_emit;
    vm.handlers[HALT] = do_halt;

    if (assemble(
        code, instructions,
        payload, count, &vm
    ))
        return EXIT_FAILURE;

    vm.fingerprint = fingerprint(
        code, instructions
    );

    if (run(&vm, code, instructions)) {
        fputs("VM failure\n", stderr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
