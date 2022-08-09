# Lab3: page tables

## 1. Print a page table(easy)

### 1.1 实验目的

本实验通过让我们打印一个页表的所有有效项，从而让我们了解到页表的三级寻址机制。

### 1.2 实验步骤

1. 由实验指导书可知，每一个页表都是有三级目录的，我们在遍历页表的时候，只需要用一个三重循环遍历整个页表并寻找出有效的页表项继续进行该操作即可，并且我们参考`freewalk`的函数实现可以知道，我们可以使用`PTE2PA`来求出下一级页目录的地址。所以我们便可以在`kernel/vm.c`当中实现`vmprint`函数。

   ```c
   void vmprint(pagetable_t pagetable)
   {
     printf("page table %p\n", pagetable);
     for (int i = 0; i < 512; i++) //每个页表有512项
     {
       pte_t pte = pagetable[i];
       if ((pte & PTE_V)) //说明该页有效
       {
         printf("..%d: pte %p pa %p\n", i, pte, PTE2PA(pte));
   
         //寻找二级页表
         pagetable_t second_pg = (pagetable_t)PTE2PA(pte);
         for (int i = 0; i < 512; i++) //每个页表有512项
         {
           pte_t second_pte = second_pg[i];
           if ((second_pte & PTE_V)) //说明该页有效
           {
             printf(".. ..%d: pte %p pa %p\n", i, second_pte, PTE2PA(second_pte));
   
             //寻找三级页表
             pagetable_t third_pg = (pagetable_t)PTE2PA(second_pte);
             for (int i = 0; i < 512; i++) //每个页表有512项
             {
               pte_t third_pte = third_pg[i];
               if ((third_pte & PTE_V)) //说明该页有效
               {
                 printf(".. .. ..%d: pte %p pa %p\n", i, third_pte, PTE2PA(third_pte));
               }
             }
           }
         }
       }
     }
   }
   ```

### 1.3 实验当中遇到的困难

1. 在一开始实验的时候，自己并不知道如何通过当前页表项来算出下一级页表的地址，在仔细阅读了`freewalk`函数之后，才了解到`PTE2PA`这个宏定义，才解决了自己寻找下一级页表困难的问题。

### 1.4 实验心得

1. 自己通过这个实验，对XV6的三级页表有了更深刻的理解，同时也对三级页表当中下一级页表的寻址方式有了更深的理解。

## 2. A kernel page table per process(hard)

### 2.1 实验目的

因为每个用户进程都有一个独立的内核页表，在进程切换的时候我们需要将用户进程的内核页表切换到`STAP`寄存器，并且在没有进程运行的时候，切换到原本在使用的内核页表`kernel_pagetable`。

### 2.2 实验步骤

1. 首先我们需要在`kernel/proc.h`当中的`struct proc`结构体的下半部分当中添加私有成员变量`kernel_pagetable`，代表进程的内核页表。

   ```c
     struct inode *cwd;           // Current directory
     char name[16];               // Process name (debugging)
     pagetable_t kernel_pgtbl;     //每个进程的内核页表
   ```

2. 然后我们需要实现一个自己的`ukvmmap`函数，它与`kvmmap`函数的逻辑基本上一致，只是多了一个`pagetable_t pagetable`参数，可以让我们指定映射哪个页面，在`ukvmmap`函数的基础上，我们增加一个`proc_kvminit`函数，逻辑与`kvminit`基本一致，但是映射空间需要修改为每个进程自己的内核页表。

   ```c
   //映射指定页表项的ukvmmap函数
   void ukvmmap(pagetable_t kernel_pgtbl,uint64 va, uint64 pa, uint64 sz, int perm)
   {
     if (mappages(kernel_pgtbl, va, sz, pa, perm) != 0)
       panic("ukvmmap");
   }
   
   //仿照kvminit实现的ukvminit函数
   pagetable_t ukvminit()
   {
     pagetable_t kernel_pgtbl = (pagetable_t)kalloc();
     memset(kernel_pgtbl, 0, PGSIZE);
   
     // uart registers
     ukvmmap(kernel_pgtbl,UART0, UART0, PGSIZE, PTE_R | PTE_W);
   
     // virtio mmio disk interface
     ukvmmap(kernel_pgtbl,VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);
   
     // CLINT
     //ukvmmap(kernel_pgtbl,CLINT, CLINT, 0x10000, PTE_R | PTE_W);
   
     // PLIC
     ukvmmap(kernel_pgtbl,PLIC, PLIC, 0x400000, PTE_R | PTE_W);
   
     // map kernel text executable and read-only.
     ukvmmap(kernel_pgtbl,KERNBASE, KERNBASE, (uint64)etext - KERNBASE, PTE_R | PTE_X);
   
     // map kernel data and the physical RAM we'll make use of.
     ukvmmap(kernel_pgtbl,(uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext, PTE_R | PTE_W);
   
     // map the trampoline for trap entry/exit to
     // the highest virtual address in the kernel.
     ukvmmap(kernel_pgtbl,TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
   
     return kernel_pgtbl;
   }
   ```

3. 接着我们修改`kernel/proc.c`当中的`procinit`函数的代码，为每一个内核页表初始化内核栈，并且同时注释掉原本初始化内核栈的代码，将内核栈的空间申请和映射放在创建进程时的函数`allocproc`函数当中,并且在`allocproc`函数当中添加调用`proc_kvminit`的代码段，以便在初始化进程空间的时候初始化用户内核页表。

   ```c
   void procinit(void)
   {
     struct proc *p;
     
     initlock(&pid_lock, "nextpid");
     for(p = proc; p < &proc[NPROC]; p++) {
         initlock(&p->lock, "proc");
     }
     kvminithart();
   }
   
   //allocproc函数核心添加代码
   
     p->pagetable = proc_pagetable(p);
     if(p->pagetable == 0)
     {
       freeproc(p);
       release(&p->lock);
       return 0;
     }
   
     //初始化内核页表空间
     p->kernel_pgtbl=ukvminit();
     if(p->kernel_pgtbl==0)
     {
       freeproc(p);
       release(&p->lock);
       return 0;
     }
   
     //初始化内核栈
     char *pa = kalloc();
     if(pa == 0)
         panic("kalloc");
     uint64 va = KSTACK((int) (p - proc));
     ukvmmap(p->kernel_pgtbl,va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
     p->kstack = va;
   ```

4. 之后我们需要对`kernel/proc.c`当中的`scheduler`函数进行修改，保证在切换进程的时候，可以将对应进程的用户内核页表的地址载入`STAP`寄存器当中，并且在任务执行之后，切换回`kernel_pagetable`，所以我们仿照`kernel/vm.c`当中的`kvminithart`函数对其进行修改。

   ```c
   //scheduler函数的核心添加代码
   
   	//将内核页表替换到STAP寄存器当中
       w_satp(MAKE_SATP(p->kernel_pgtbl));
       //清除快表缓存
       sfence_vma();
       
       p->state = RUNNING;
       c->proc = p;
       swtch(&c->context, &p->context);
   
       //将STAP寄存器的值设定为全局内核页表地址
       kvminithart();
   ```

5. 下一步我们考虑在销毁进程的时候释放对应的内核页表，并且在释放内核页表前需要先释放进程对应的内核栈空间，所以我们对位于`kernel/proc.c`当中的`freeproc`函数进行修改，并且通过改进的`proc_freepagetable`函数来对每个进程的内核页表进行释放。

   ```c
   //freeproc函数的核心修改添加代码
   
     if(p->trapframe)
       kfree((void*)p->trapframe);
     p->trapframe = 0;
   
     // 删除内核栈
     if(p->kstack)
     {
       // 通过页表地址， kstack虚拟地址 找到最后一级的页表项
       pte_t* pte=walk(p->kernel_pgtbl,p->kstack,0);
       if(pte==0)
       {
         panic("free kstack");
       }
       // 删除页表项对应的物理地址
       kfree((void*)PTE2PA(*pte));
     }
     p->kstack=0;
   
     if(p->pagetable)
       proc_freepagetable(p->pagetable, p->sz);
   
     // 删除kernel pagetable
     if(p->kernel_pgtbl)
     {
       proc_freewalk(p->kernel_pgtbl);
     }
     p->kernel_pgtbl=0;
   
   //改进后的proc_freepagetable函数
   void proc_freepagetable(pagetable_t pagetable, uint64 sz)
   {
     uvmunmap(pagetable, TRAMPOLINE, 1, 0);
     uvmunmap(pagetable, TRAPFRAME, 1, 0);
     uvmfree(pagetable, sz);
   }
   ```

6. 最后我们修改位于`kernel/vm.c`当中的`kvmpa`函数，将`walk`函数使用的全局内核页表地址替换成进程自己的内核页表地址，并在`kernel/defs.h`当中添加`walk`函数的声明，以及在`kernel/vm.c`当中添加`spinlock.h`和`proc.h`两个头文件。

   ```c
   //kvmpa函数的核心修改代码
   
   pte = walk(myproc()->kernel_pgtbl, va, 0);
   ```

   ```c
   //kernel/defs.h当中添加的函数声明
   pte_t *         walk(pagetable_t pagetable, uint64 va, int alloc);
   ```

### 2.3 实验当中遇到的困难和解决方法

1. 在一开始进行实验的时候，当修改`allocproc`函数时，需要对内核栈进行分配和映射，自己在一开始实现的时候，并没有看到该函数最后一行当中的`p->context.sp = p->kstack + PGSIZE`语句，该语句设置了内核栈顶指针，在这之前必须保证内核栈已经分配并且映射完成，但是自己之前并没有在该句之前将内核栈映射完成，会导致XV6的系统无法正确进入。

   解决方法：在仔细阅读源码之后，发现了上述语句会进行内核栈顶的指针的设置，因此需要修在最后一个语句之前将内核栈分配和映射完成。

### 2.4 实验心得

1. 感觉通过本次实验，自己对内核页表的理解加深了许多，并且对如何做实验也有了更深刻的理解，自己慢慢的摸索到了根据实验指导书的提示，对看对应的代码和手册当中的对应部分，然后一点点的实现实验，自己的书写代码能力和阅读代码能力得到了很大提升。

## 3. Simplify  copyin/copyinstr(hard)

### 3.1 实验目的

本实验是在上一个实验的基础上，在用户进程内核页表当中添加用户页表映射的副本，这样当该进程陷入内核时，就可以通过内核正确翻译虚拟地址，从而访问用户进程当中的数据，这样做之后，可以利用`MMU`寻址功能进行寻址，效率更高而且可以享受快表加速。

### 3.2 实验过程

1. 首先我们需要实现用户页表的同步到用户进程内核页表当中，则需要实现映射和缩减两个操作，所以在`kernel/vm.c`添加`kvmcopymappings`和`kvmdealloc`两个函数来实现该操作。

   ```c
   //kvmcopymappings函数核心代码
     // PGROUNDUP: 对齐页边界，防止 remap
     for(i = PGROUNDUP(start); i < start + sz; i += PGSIZE)
     {
       if((pte = walk(src, i, 0)) == 0) // 找到虚拟地址的最后一级页表项
         panic("kvmcopymappings: pte should exist");
       if((*pte & PTE_V) == 0)	// 判断页表项是否有效
         panic("kvmcopymappings: page not present");
       pa = PTE2PA(*pte);	 // 将页表项转换为物理地址页起始位置
       // `& ~PTE_U` 表示将该页的权限设置为非用户页
       // 必须设置该权限，RISC-V 中内核是无法直接访问用户页的。
       flags = PTE_FLAGS(*pte) & ~PTE_U;
       // 将pa这一页的PTEs映射到dst上同样的虚拟地址
       if(mappages(dst, i, PGSIZE, pa, flags) != 0)
       {
         // 清除已经映射的部分，但不释放内存
         uvmunmap(dst, 0, i / PGSIZE, 0);
         return -1;
       }
     }
   
   //kvmdealloc函数核心代码
     if(newsz >= oldsz)
       return oldsz;
   
     if(PGROUNDUP(newsz) < PGROUNDUP(oldsz))
     {
       // 如果存在多余的页需要释放
       int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
       uvmunmap(pagetable, PGROUNDUP(newsz), npages, 0);
     }
   ```

2. 之后我们根据提示，在用户内核页表当中的映射范围是[0,PLIC]，但是我们通过阅读XV6书籍可知，全局内核页表的定义当中存在一个`CLINT`核心本地中断，它仅在内核启动时使用，所以用户进程的内核页表当中不需要再存在`CLINT`,所以我们将`proc_kvminit`当中的`CLINT`映射部分注释掉，防止再映射用户页表时出现`remap`的情况。

   ```c
   //proc_kvminit函数的核心修改部分
   
   // virtio mmio disk interface
     ukvmmap(kernel_pgtbl,VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);
   
     // CLINT
     //ukvmmap(kernel_pgtbl,CLINT, CLINT, 0x10000, PTE_R | PTE_W);
   
     // PLIC
     ukvmmap(kernel_pgtbl,PLIC, PLIC, 0x400000, PTE_R | PTE_W);
   ```

3. 然后我们根据实验指导书可知，我们需要对`fork()`，`sbrk()`，`exec()`三个函数进行修改，首先修改`fork`函数，把父进程的用户页表映射拷贝一份到新进程的内核页表当中。

   ```c
   //fork函数的核心添加部分
   
   if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0||kvmcopymappings(np->pagetable, np->kernel_pgtbl, 0, p->sz) < 0)
     {
       freeproc(np);
       release(&np->lock);
       return -1;
     }
   ```

4. 然后我们修改`sbrk`函数，我们通过阅读`sys_sbrk`函数可以发现，执行内存相关的函数为`growproc`函数，所以我们对其进行修改，并且需要对用户页表的扩大和缩小都进行同步处理。

   ```c
   //growproc函数的核心修改代码
   
     struct proc *p = myproc();
   
     sz = p->sz;
     if(n > 0)
     {
       uint64 newsz;
       if((newsz = uvmalloc(p->pagetable, sz, sz + n)) == 0) 
       {
         return -1;
       }
       // 内核页表中的映射同步扩大
       if(kvmcopymappings(p->pagetable, p->kernel_pgtbl, sz, n) != 0) 
       {
         uvmdealloc(p->pagetable, newsz, sz);
         return -1;
       }
       sz = newsz;
     }
     else if(n < 0)
     {
       uvmdealloc(p->pagetable, sz, sz + n);
       // 内核页表中的映射同步缩小
       sz = kvmdealloc(p->kernel_pgtbl, sz, sz + n);
     }
   ```

5. 然后我们修改`exec`函数，在映射之前先检测程序大小是否超过`PLIC`防止出现`remap`的情况，然后在映射之前清楚在[0,PLIC]当中的原有内容，然后在将要执行的程序映射到[0,PLIC]当中。

   ```c
   //exec函数核心修改代码
   
   // 添加检测，防止程序大小超过 PLIC
       if(sz1 >= PLIC) 
       {
         goto bad;
       }
       
   // 清除内核页表中对程序内存的旧映射，然后重新建立映射。
     uvmunmap(p->kernel_pgtbl, 0, PGROUNDUP(oldsz)/PGSIZE, 0);
     kvmcopymappings(pagetable, p->kernel_pgtbl, 0, sz); 
   ```

6. 然后根据提示，我们在`userinit`的内核页表当中包含第一个进程的用户页表

   ```c
   //userinit函数的核心修改代码
   
     uvminit(p->pagetable, initcode, sizeof(initcode));
     p->sz = PGSIZE;
     kvmcopymappings(p->pagetable, p->kernel_pgtbl, 0, p->sz); // 同步程序内存映射到进程内核页表中
   ```

7. 最后我们对`copyin`和`copyinstr`函数进行重写，用`copyin_new`和`copyinstr_new`来进行替换，并在`kernel/defs.h`当中添加对`copyin_new`和`copyinstr_new`两个函数的声明。

   ```c
   //kernel/defs.h文件添加内容
   int             copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len);
   int             copyinstr_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max);
   
   ```

   ```c
   //copyin函数修改代码
   int copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
   {
     return copyin_new(pagetable,dst,srcva,len);
   }
   
   //copyinstr函数修改代码
   int copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
   {
     return copyinstr_new(pagetable,dst,srcva,max);
   }
   ```

### 3.3 实验当中遇到的困难和解决方法

1. 在一开始实验的时候，我并没有认识到在[0,PLIC]当中还有一个`CLINT`本地中断，在`proc_kvminit`当中并没有取消对`CLINT`的映射，结果出现了重映射的错误，自己查找了很长时间，也没有找到问题，最后在网上搜索到有关资料之后，仔细看了一遍XV6的实验指导书，才发现了这个小知识点。

   解决方法：通过阅读XV6实验指导书，我了解到了这个小知识点，然后通过取消对`CLINT`的映射，从而解决了这个问题。

### 3.4 实验心得

1. 通过本次实验，我对页表赋值、页表分配、以及页表映射的理解又加深了一些，并且通过[O,PLIC]当中存在一个`CLINT`只在内核页表初始化的时候使用，而用户内核页表当中不需要使用的问题的处理，让我意识到了阅读手册的重要性，我们出现问题的时候，可以第一时间阅读官方手册，一般都可以解决问题。