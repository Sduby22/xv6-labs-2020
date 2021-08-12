# Lab COW: copy on write fork()

## What should we do?

- When fork(), map the same pages to parent & child page table
- Make sure to make the mapping pte read only, because if not, the parent and the child would affect each other, which violates the process isolation.
- If parent or child tries to write on a COW page(which is ro), it will cause a page fault
- in page fault, we should:
	- check if the faulting va is a COW page
		- we can use the reserved 2 bits in the pte.
	- copy the page, map it to the pagetable with PTE_W flag.
	- What should be done with the old ro page?
		- should be freed, if no pagetable points to it
		- maintain a refcount for every page, if refcount = 0, 		   	 frees it. 
			- when unmap a page, refcount--
			- when map a page, refcount++
			- 128M mem, 128M/4k = 32k pages, needs 32k refcount.
			  32k int = 32k * 4B = 128KB
			- uvmunmap(do_free=1) checks refcount opposed to just freeing the page.
	- when in kernel mode, we use walkaddr() which wont trigger
	  page faults, so we must manually handle it in copyout().
	  
## What's the problem?

- In this method, if parent&child both write to memory after 
  fork(), we have to copy the pages twice, map the new pages 
  to parent & child respectively, and maybe we want to free 
  the old ro page if refcount = 0. 
  
  can we find a new method that:
	- when child writes, copy a new page for child, make it 
	  rw, and then make the parent pte that was ro become 
	  rw?
	  - that would not be right when the parent chain
		is greater than 2 and a page's refcount is bigger
		than 2.
	- maybe if refcount = 1, make the pte rw?
	  - No because it will be costly to find that pte.

## Stumbles & Solutions

### Where should we store our refcounts?

- We can make an array in vm.c, the array should have `(PHYSTOP - KERNBASE)/PGSIZE` entries. each entry represents refcount of a physical page
- mapping: f(x) = (pa-KERNBASE)/PGSIZE

### Where do we add/dec refcounts?

We should modify refcounts when we map/unmap pages, which involves:

- mappages(), uvmunmap(), we dont unmap kernel pagetables because kernel lives all over the time.

### Is there a circumstance that refcount of a paticular page will go below 0?

- Probably not. Prove: We only dec the refcount when we successfully unmapped a page, if refcount go below 0, it means that we unmap a page more times than we map it. In this case we cant successfully unmap that page, it contradicts.

### How do we use reserved 2 bits to indicate COW pages?

- To mark a page `#define PTE_COW (1L << 8)` in riscv.h, we use 1 of the 2 RSW bits to indicate COW pages.

- Modify uvmcopy.c (which is called during fork()), add PTE_COW flag, remove PTE_W flag for both parent and child.

	- when error, we should resume the pte flags of the parent: remove PTE_COW flag and add PTE_W flag.
	
	
### The kernel freezes when accessing refcount array.

- Sometimes pa < KERNBASE or pa >= PHYSTOP, which leads to array out-of-bound. We just use if statements to get rid of that.

### Kernel page fault stval=0x21fdc0000

That was a stupid mistake: forget to dereference pte pointer when extracting pa using PTE2PA

### refcount always > 1

we add/dec the refcount in mappages(), but in kvminit, the kernel maps the whole physical memory, so all entries in the refcount array was set 1. Multiple ways to fix this:

- Use a different mappages() version in kvmmap(), which doesn't affect refcount.
- When kalloc(), set refcount = 1, Only add refcount when uvmcopying(), dec refcount when unmapping it.

I prefer the 1st method so I add a kmappages().

## Summary

- Very interesting lab of good challenge!!
- Important to maintain refcount correctly!
- Refcount reminds me of C++ smart pointers!
- Keep in mind that when using kernel pagetable, we have to handle "page faults" using software.

