// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

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
        if(err&FEC_WR&&uvpt[PGNUM(addr)]&PTE_COW){
          
        }else{
            panic("Not a valid pgfault for copy-on-write page\n");
        }
	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if((r=sys_page_alloc(sys_getenvid(),PFTEMP,PTE_P|PTE_U|PTE_W))<0)
	    panic("pgft:%e\n",r);
        memcpy((void*)PFTEMP,(void*)ROUNDDOWN((uintptr_t)addr,PGSIZE),PGSIZE);
        if((r=sys_page_map(sys_getenvid(),(void*)PFTEMP,sys_getenvid(),(void*)ROUNDDOWN((uintptr_t)addr,PGSIZE),PTE_P|PTE_U|PTE_W))<0)
            panic("pgft:%e\n",r);
        if((r=sys_page_unmap(sys_getenvid(),(void*)PFTEMP))<0)
            panic("pgft:%e\n",r);
	//panic("pgfault not implemented");
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
	//panic("duppage not implemented");
	r = uvpt[pn];
	if(r&PTE_W||r&PTE_COW){
	    r=sys_page_map(0,(void*)(pn*PGSIZE),envid,(void*)(pn*PGSIZE),PTE_P|PTE_U|PTE_COW);
	    if(r){
	        return r;
	    }
	    r=sys_page_map(0,(void*)(pn*PGSIZE),0,(void*)(pn*PGSIZE),PTE_P|PTE_U|PTE_COW);
	}else{
	    r=sys_page_map(0,(void*)(pn*PGSIZE),envid,(void*)(pn*PGSIZE),PTE_P|PTE_U);
	}
	return r;
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
	//panic("fork not implemented");

	int res=0;
	set_pgfault_handler(pgfault);
	envid_t child = sys_exofork();
	if(child==0)
	    thisenv = &envs[ENVX(sys_getenvid())];
	if(child<0) return child;
	uintptr_t va = 0;
	for(va = 0;va<USTACKTOP;va+=PGSIZE){
	    if(uvpd[PDX(va)]&PTE_P && uvpt[PGNUM(va)]&PTE_P&&uvpt[PGNUM(va)]&(PTE_W|PTE_COW)){
	        res=duppage(child,PGNUM(va));
	        if(res) return res;
	    }
	}
	sys_page_alloc(child,(void*)(UXSTACKTOP-PGSIZE),PTE_P|PTE_U|PTE_W);
	/*
	if(!child)
	    thisenv = &envs[ENVX(sys_getenvid())];
	*/
	extern void _pgfault_upcall(void);
	sys_env_set_pgfault_upcall(child,_pgfault_upcall);
	res = sys_env_set_status(child,ENV_RUNNABLE);
	if(res<0) return res;
	return child;

}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
