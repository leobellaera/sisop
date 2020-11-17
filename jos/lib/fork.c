// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW 0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	pte_t pte = uvpt[PGNUM(addr)];

	if (!(err & FEC_WR) || !(pte & PTE_COW)) {
		panic("pgfault");
	}

	//a va debe estar alineada a PGSIZE
	addr = ROUNDDOWN(addr, PGSIZE);

	if ((r = sys_page_alloc(0, PFTEMP, PTE_P | PTE_W | PTE_U)) < 0)
		panic("pgfault sys_page_alloc %e", r);

	memmove(PFTEMP, addr, PGSIZE);

	//mapeamos la pagina ahora con permisos de lectura y escritura
	if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_U | PTE_W)) < 0)
		panic("pgfault sys_page_map %e", r);

	//desmapeamos la pagina temporaria
	if ((r = sys_page_unmap(0, PFTEMP)) < 0)   
		panic("pgfault sys_page_unmap %e", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	pte_t pte = uvpt[pn];
	void* addr = (void*) (pn * PGSIZE);

	if (pte & PTE_SHARE) {
		if ((r = sys_page_map(0, addr, envid, addr, pte & PTE_SYSCALL)) < 0) {
			panic("duppage sys_page_map: %e", r);
		}

	} else if ((pte & PTE_COW) || (pte & PTE_W)) {
		if ((r = sys_page_map(0, addr, envid, addr, PTE_COW | PTE_P | PTE_U)) < 0)
			panic("duppage sys_page_map: %e", r);
		if ((r = sys_page_map(0, addr, 0, addr, PTE_COW | PTE_P | PTE_U)) < 0)
			panic("duppage sys_page_map: %e", r);

	} else {
		if ((r = sys_page_map(0, addr, envid, addr, PTE_P | PTE_U)) < 0)
               panic("duppage sys_page_map: %e", r);

	}

	return 0;
}

static void
dup_or_share(envid_t dstenv, void *va, int perm) {
	int result;

	if (perm & PTE_W) {
		if ((result = sys_page_alloc(dstenv, va, perm)) < 0)
			panic("dup_or_share sys_page_alloc: %e", result);

		if ((result = sys_page_map(dstenv, va, 0, UTEMP, perm)) < 0)
			panic("dup_or_share sys_page_map: %e", result);

		memmove(UTEMP, va, PGSIZE);

		if ((result = sys_page_unmap(0, UTEMP)) < 0)
			panic("dup_or_share sys_page_unmap: %e", result);
	} else {
		if ((result = sys_page_map(0, va, dstenv, va, perm)) < 0)
			panic("dup_or_share sys_page_map: %e", result);
	}

}

envid_t
fork_v0(void)
{
	envid_t envid = sys_exofork();
	if (envid < 0)
		panic("fork_v0 sys_exofork failed");
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (uintptr_t addr = 0; addr < UTOP; addr+= PGSIZE){
		pde_t pde = uvpd[PDX(addr)];
		if (pde & PTE_P) {
			pde_t pte = uvpt[PGNUM(addr)];
			if (pte & PTE_P) 
				dup_or_share(envid, (void*)addr, pte & PTE_SYSCALL);
		}
	}

	if (sys_env_set_status(envid,ENV_RUNNABLE)) 
		panic("fork_v0 sys_env_set_status");

	return envid;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);

	envid_t envid = sys_exofork();

	if (envid < 0)
		panic("fork sys_exofork: %e", envid);

	//child env
	if (envid == 0) {
		//set this env to child id
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	//parent env
	for (int i = 0 ; i < PDX(UTOP) ; i++) {
		pde_t pde = uvpd[i];
		//if pde is not present, continue
		if ((pde & PTE_P) == 0) continue;

		//copy present pages
		for (int j = 0; j < NPTENTRIES; j++) {

			uintptr_t addr = (uintptr_t) PGADDR(i, j, 0);
			if (addr == UXSTACKTOP - PGSIZE) continue;

			pte_t pte = uvpt[PGNUM(addr)];
			if ((pte & PTE_P))
      			duppage(envid, PGNUM(addr));
      	}
    }

	//create exception stack for child env
	void* addr = (void *)(UXSTACKTOP - PGSIZE);
	int r;
	if ((r = sys_page_alloc(envid, addr, PTE_P | PTE_U | PTE_W)) < 0)
		panic("fork sys_page_alloc: %e", r);

	//set child page fault handler
    if ((r = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall)) < 0)
		panic("fork sys_env_set_pgfault_upcall: %e", r);

    //set child status as runnable
    if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
    	panic("fork sys_env_set_status: %e", r);

    return envid;

}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
