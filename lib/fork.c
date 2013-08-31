// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
// PTE_COW is software defined
#define PTE_COW		0x800


extern void _pgfault_upcall(void);

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
  if ( !(uvpd[PDX(addr)] & PTE_P ) ) 
    panic("pgfault : page dir PTE_P not set.\n");
  if (((err & FEC_WR) != FEC_WR) || !(uvpt[PGNUM(addr)] & PTE_COW) ) 
    panic("pgfault : pagefault %08x not FEC_WR or PTE_COW.\n",err);

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
  // allocate  PFTEMP -> new page
  r = sys_page_alloc(0, PFTEMP, PTE_U | PTE_P | PTE_W);
  if (r < 0)
    panic("pgfault : sys_page_alloc error : %e.\n",r);

  // copy old page  = new page
  addr = ROUNDDOWN(addr, PGSIZE);
  memmove(PFTEMP, addr, PGSIZE);

  // make addr -> new page
  r = sys_page_map(0, PFTEMP, 0, addr, PTE_U | PTE_P | PTE_W);
  if (r < 0)
    panic("pgfault : sys_page_map error : %e.\n",r);

  // delete map of PFTEMP -> new page
  r = sys_page_unmap(0, PFTEMP);
  if (r < 0)
    panic("pgfault : sys_page_unmap error : %e.\n",r);

  return ;
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
  void * va = (void *) (pn << PGSHIFT);

  // check page dir PTE_P exist
  if (!(uvpd[PDX(pn << PGSHIFT)] & PTE_P )) 
    panic("duppage : page dir PTE_P is not set.\n");

  // check page is PTE_W or PTE_COW
  if (!(uvpt[pn] & ( PTE_W | PTE_COW )))
    panic("duppage : page is not PTE_W or PTE_COW.\n");

  // map child's page as PTE_COW
  r = sys_page_map(0, va, envid, va, PTE_U | PTE_COW | PTE_P);
  if (r < 0)
    panic("duppage : sys_page_map error : %e.\n",r);
 
  // remap parent's page as PTE_COW, make PTE_W invalid.
  r = sys_page_map(0, va, 0, va, PTE_U | PTE_COW | PTE_P);
  if (r < 0) 
    panic("dupage : sys_page_map erro : %e.\n", r);

	return 0;
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
  envid_t envid;
  uintptr_t va;
  int r;

  // set pagefault handler
  set_pgfault_handler(pgfault);

  // allocate child env
  envid = sys_exofork();
  if (envid < 0) 
    panic("fork : sys_exofork error, %e.\n", envid);

  // Executing at child 
  if (envid == 0) {
    thisenv = &envs[ENVX(sys_getenvid())];
    return 0;
  }

  // Child's env initialization : 
  // 1. element in struct Env itself.
  // 2. child env's page table initizlization( address space )
  // 
  // For 1. some part of struct Env is initialized in sys_exefork, 
  // remaining exception stack and pgfault_upcall to initialize.
  // For 2. create envid 's address space

  // 2.1. Duppage [UTEXT, USTACKTOP] of PTE_W | PTE_COW | PTE_P
  // first see if pdt & PTE_P or not
  for (va = UTEXT ; va < USTACKTOP; va += PGSIZE){
    if ((uvpd[PDX(va)] & PTE_P) && (uvpt[PGNUM(va)] & PTE_P) && 
        (uvpt[PGNUM(va)] & PTE_U) && (uvpt[PGNUM(va)] & (PTE_W | PTE_COW)))
      duppage(envid, PGNUM(va));
    
    // For pages that are not PTE_W or PTE_COW, just ignore it, some of 
    // that page are protection consideration.
  }

  // 1.2. Create exception stack, parent's exception stack cannot 
  // be duppaged ! because at this time it's page fault are using it, 
  // and it should be writable.
  r = sys_page_alloc(envid, (void*)(UXSTACKTOP-PGSIZE), PTE_U | PTE_P | PTE_W);
  if (r < 0)
    panic("[%08x] fork : sys_page_alloc error : %e.\n", thisenv->env_id, r);

  // 1.1 Set child's page fault handler -- initialize 
  // child_env->env_pgfault_upcall
  r = sys_env_set_pgfault_upcall(envid, (void*)_pgfault_upcall);
  if (r < 0)
    panic("[%08x] fork : sys_env_set_pgfault_upcall error : %e.\n", 
      thisenv->env_id, r);

  // Child is ready to run, make it RUNNABLE
  r = sys_env_set_status(envid, ENV_RUNNABLE);
  if (r < 0)
    panic("[%08x] fork : sys_env_set_status error : %e",thisenv->env_id, r);

  return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
