# Lab: Threads

## Uthread

### When create

- set state to `RUNNABLE`
- set `context.ra` to `func`, when running for the first time, will set PC to addr of the 1st instuction of `func`
- set `context.sp` to `stack+STACKSIZE` (the highest addr of stack)

### When switch

- just copy the code from `swtch.S`, save **callee-saved** regs only, because caller-saved register will be automatically saved&restored by C function call(compiler did it).

## Thread-safe Hash Table

### Invariants

- After calling put(`key`,`value`), there must be one and only one entry of `key`,`value` in table[key % NBUCKET]
- Before we insert a new entry or update existing entry in `put()`, we need to search through the whole `table[i]` to find if there is an existing entry, which the invariant doesn't hold.

### Implementation

- We have to acquire the lock of table[i] when accessing it. in `get()` and `put()`
