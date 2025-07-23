echo "\n----- FreeRTOS Utility Debug Script Loaded ---\n"
printf "- $config_use_posix_errno_enabled : %d \n", $config_use_posix_errno_enabled
printf "- $cfg_max_priority : %d \n", $cfg_max_priority
printf "- $platform_mem_baseaddr (memory base adrress on hardware platform): 0x%x\n", $platform_mem_baseaddr
printf "- $platform_mem_maxsize : 0x%x \n", $platform_mem_maxsize

define freertos_heap4_snapshot
    set $expect_baseaddr_in = $platform_mem_baseaddr
    set $expect_tot_nbytes  = $platform_mem_maxsize
    set $init_chk_passed = 0
    echo "\n--- FreeRTOS Heap (heap_4.c) Information ---\n"
    # Try to access pxEnd to see if it's a valid symbol.
    # If it's not initialized, it might be NULL or point to garbage.
    # We'll just proceed and let subsequent errors indicate an issue.
    print   xStart
    printf "internal block list, end pointer:%p \n", pxEnd

    # Check: Are the heap pointers within the provided memory range?
    # This check only applies if a memory range was provided via arguments.
    if $expect_baseaddr_in != 0 && $expect_tot_nbytes != 0
        set $region_start = $expect_baseaddr_in
        set $region_end = $expect_baseaddr_in + $expect_tot_nbytes
        set $start_next_free_block_addr = (unsigned int)xStart.pxNextFreeBlock
        set $pxEnd_addr = (unsigned int)pxEnd
        # Pointers must be within [start, end) range
        set $is_start_ptr_valid = ($start_next_free_block_addr >= $region_start && $start_next_free_block_addr < $region_end)
        set $is_end_ptr_valid = ($pxEnd_addr >= $region_start && $pxEnd_addr < $region_end)
        #print  $is_start_ptr_valid
        #print  $is_end_ptr_valid
        set $init_chk_passed = $is_start_ptr_valid && $is_end_ptr_valid
        if !$init_chk_passed
            echo "FreeRTOS heap pointers appear to be outside the specified memory range:\n"
            printf "Expected Range: 0x%x - 0x%x (exclusive end)\n", $region_start, $region_end
            printf "xStart.pxNextFreeBlock: 0x%x\n", $start_next_free_block_addr
            printf "pxEnd: 0x%x\n", $pxEnd_addr
            echo "Ensure the provided base address and total size are correct, or that the heap is initialized within this range.\n"
        end
    end
    # --- End Heap Initialization Check ---

    # Print global heap variables
    if $init_chk_passed
        printf "Free Bytes Remaining: %u bytes\n", xFreeBytesRemaining
        printf "Minimum Free Bytes Remaining: %u bytes\n", xMinimumEverFreeBytesRemaining
        set $config_total_heap_size = sizeof(ucHeap)
        if $config_total_heap_size != 0
            printf "Total Heap Size (configTOTAL_HEAP_SIZE): %u bytes\n", $config_total_heap_size
        else
            echo "Warning: Could not determine 'configTOTAL_HEAP_SIZE' from 'ucHeap' symbol.\n"
            echo "  Please set it manually if needed: 'set $config_total_heap_size = 0x4e00'\n"
        end
        # Calculate approximate allocated bytes if total heap size is known
        if $config_total_heap_size != 0
            set $allocated_bytes = $config_total_heap_size - xFreeBytesRemaining
            printf "Approx. Allocated Bytes: %u bytes\n", $allocated_bytes
        end
        printf "xBlockAllocatedBit (used to mark allocated blocks): 0x%x\n", xBlockAllocatedBit
    end

    if $init_chk_passed
        echo "\n--- Free List Blocks ---\n"
        # Initialize GDB convenience variables for iteration and counting
        set $uxFreeBlocks = 0
        set $uxTotalFreeSize = 0
        # Get the first block in the free list.
        # Crucially, we cast it to (BlockLink_t *) so GDB knows the structure layout
        # and can correctly access members like 'xBlockSize' and 'pxNextFreeBlock'.
        set $pxCurrentBlock = (BlockLink_t *)xStart.pxNextFreeBlock
        # Loop through the free list until the end marker (pxEnd) is reached
        while $pxCurrentBlock != pxEnd
            set $uxFreeBlocks = $uxFreeBlocks + 1
            set $block_size = $pxCurrentBlock->xBlockSize
            # Get the address of the current block
            set $block_address = $pxCurrentBlock
            set $uxTotalFreeSize = $uxTotalFreeSize + $block_size
            printf "  Block %u: Address=0x%x, Size=%u bytes\n", $uxFreeBlocks, $block_address, $block_size
            # Move to the next block in the list
            set $pxCurrentBlock = (BlockLink_t *)$pxCurrentBlock->pxNextFreeBlock
        end
        printf "\nTotal Free Blocks in List: %u\n", $uxFreeBlocks
        printf "Total Free Bytes (sum of free list blocks): %u bytes\n", $uxTotalFreeSize
        # Consistency check: Compare the global free bytes count with the sum from the list traversal
        if xFreeBytesRemaining != $uxTotalFreeSize
            echo "WARNING: 'xFreeBytesRemaining' does NOT match the sum of free list blocks.\n"
            printf "  xFreeBytesRemaining: %u, Sum of free list blocks: %u\n", xFreeBytesRemaining, $uxTotalFreeSize
            echo   "  This discrepancy could indicate heap corruption!\n"
        end
    end
    echo "--------------------------------------------\n"    
end
# end of freertos_heap4_snapshot

# Helper function to get task state string from eTaskState enum value
define get_task_state_string
    set $state_enum = $arg0
    if $state_enum == 0x00U
        printf "Running"
    end
    if $state_enum == 0x01U
        printf "Ready"
    end
    if $state_enum == 0x02U
        printf "Blocked"
    end
    if $state_enum == 0x03U
        printf "Suspended"
    end
    if $state_enum == 0x04U
        printf "Deleted"
    end
    if $state_enum >= 0x05U
        printf "Invalid"
    end
end

# Helper function to calculate stack high water mark (HWM)
# This GDB implementation mimics prvTaskCheckFreeStackSpace for downward-growing stacks.
# $arg0: TCB_t* pxTCB - Pointer to the Task Control Block
define calculate_stack_hwm_gdb
    # Pointer to the Task Control Block
    set $pxTCB = (TCB_t *)$arg0
    # Defined in tasks.c
    set $tskSTACK_FILL_BYTE = 0xa5
    set $ulCount = 0
    # For downward-growing stacks (e.g., ARM Cortex-M), pxStack points to the lowest address
    # of the allocated stack memory. We count fill bytes from this base upwards.
    set $pucStackCurrent = (unsigned char *)$pxTCB->pxStack
    # pxTopOfStack points to the current lowest occupied address on the stack.
    set $pucStackLimit = (unsigned char *)$pxTCB->pxTopOfStack

    # Iterate from the stack base upwards towards the current stack pointer
    # counting the number of tskSTACK_FILL_BYTE values.
    while $pucStackCurrent < $pucStackLimit
        if *$pucStackCurrent == $tskSTACK_FILL_BYTE
            set $ulCount = $ulCount + 1
            set $pucStackCurrent = $pucStackCurrent + 1
        else
            # Found a non-fill byte, meaning this part of the stack has been used.
            # break from while Loop
            set $pucStackCurrent = $pucStackLimit
        end
    end
    # Convert byte count to StackType_t units (words)
    set $stack_type_size = sizeof(StackType_t)
    set $hwm = $ulCount / $stack_type_size
    printf "%u", $hwm
end

# Helper function to print detailed information for a single TCB
# $arg0: TCB_t* pxTCB - Pointer to the Task Control Block
# $arg1: eTaskState state_enum - The state of the task (e.g., eReady, eBlocked)
# $arg2: unsigned int base_addr - Base address of the valid memory region
# $arg3: unsigned int total_bytes - Total size of the valid memory region in bytes
define print_tcb_details
    set $pxTCB = (TCB_t *)$arg0
    set $state_enum = $arg1
    set $base_addr = $arg2
    set $total_bytes = $arg3

    set $tcb_addr = (unsigned int)$pxTCB
    set $region_end = $base_addr + $total_bytes

    if $tcb_addr < $base_addr || $tcb_addr >= $region_end
        printf "[print_tcb_details] Error: TCB address 0x%x is outside the valid memory range [0x%x, 0x%x).\n", $tcb_addr, $base_addr, $region_end
    else
        printf "  Name: %s\n", $pxTCB->pcTaskName
        printf "  Handle (TCB Addr): 0x%x\n", $pxTCB
        printf "  State: "
        get_task_state_string $state_enum
        printf "\n"
        printf "  Priority: %u\n", $pxTCB->uxPriority
        printf "  Stack Base: 0x%x\n", $pxTCB->pxStack
        printf "  Stack Top (Current SP): 0x%x\n", $pxTCB->pxTopOfStack
        printf "  Stack High Water Mark (words): "
        calculate_stack_hwm_gdb $pxTCB
        printf "\n"
        printf "  TCB Number: %u\n", $pxTCB->uxTCBNumber
        printf "  Run Time Counter: %u\n", $pxTCB->ulRunTimeCounter
        printf "  Statically Allocated: %u (0=Dynamic, 1=Static Stack, 2=Static Stack+TCB)\n", $pxTCB->ucStaticallyAllocated
        printf "  Notification State: %u (0=NOT_WAITING, 1=WAITING, 2=RECEIVED)\n", $pxTCB->ucNotifyState
        printf "  Notification Value: %u\n", $pxTCB->ulNotifiedValue
        if $config_use_posix_errno_enabled == 1
            printf "  POSIX Errno: %d\n", $pxTCB->iTaskErrno
        else
            printf "  POSIX Errno: Not compiled in (configUSE_POSIX_ERRNO == 0)\n"
        end
    end
    printf "-----------------------------------\n"
end

# Helper function to iterate through a FreeRTOS list and print task details
# $arg0: List_t* pxList - Pointer to the FreeRTOS list
# $arg1: eTaskState state_enum - The state to report for tasks in this list
# $arg2: unsigned int base_addr - Base address of the valid memory region
# $arg3: unsigned int total_bytes - Total size of the valid memory region in bytes
define print_tasks_in_list
    # Pointer to the FreeRTOS list
    set $pxList = $arg0
    # The state to report for tasks in this list
    set $state_enum = $arg1
    # Base address of the valid memory region
    set $base_addr = $arg2
    # Total size of the valid memory region in bytes
    set $total_bytes = $arg3
    set $region_end = $base_addr + $total_bytes
    if $pxList < $base_addr || $pxList >= $region_end
        printf "[print_tasks_in_list] Error: task list address 0x%x is outside the valid memory range [0x%x, 0x%x).\n", $pxList, $base_addr, $region_end
    else
        set $num_tasks_in_list = $pxList->uxNumberOfItems
        if $num_tasks_in_list > 0
            # Start from the first item in the list (after xListEnd)
            set $pxListItem = $pxList->xListEnd.pxNext
            # Loop until we return to xListEnd (circular list)
            while $pxListItem != &$pxList->xListEnd
                # Get the TCB pointer from the ListItem_t's pvOwner member
                set $pxTCB = (TCB_t *)$pxListItem->pvOwner
                print_tcb_details $pxTCB $state_enum $base_addr $total_bytes
                # Move to the next item in the list
                set $pxListItem = $pxListItem->pxNext
            end
        end
    end
end

# Main function to snapshot all FreeRTOS tasks
# $arg0: UBaseType_t max_priorities - Maximum number of priorities (configMAX_PRIORITIES)
# $arg1: unsigned int base_addr - Base address of the system memory (e.g., start of RAM)
# $arg2: unsigned int total_bytes - Total size of the system memory in bytes
define freertos_ready_tasks_snapshot
    echo "\n--- FreeRTOS Ready Task Snapshot ---\n"
    set $max_priorities = $cfg_max_priority
    set $base_addr = $platform_mem_baseaddr
    set $total_bytes = $platform_mem_maxsize

    echo "--- Running Task ---\n"
    if pxCurrentTCB != 0
        print_tcb_details pxCurrentTCB 0x00U $base_addr $total_bytes
    else
        # No task currently running
        echo "No task currently running (scheduler not started or pxCurrentTCB is NULL).\n"
    end

    echo "\n--- Ready Tasks ---\n"
    set $i = $max_priorities - 1
    while $i >= 0
        printf "Priority %u Ready List:\n", $i
        print_tasks_in_list &pxReadyTasksLists[$i] 0x01U $base_addr $total_bytes
        set $i = $i - 1
    end
    echo "--- End of FreeRTOS Ready Task Snapshot ---\n"
end

define freertos_blocked_tasks_snapshot
    set $base_addr = $platform_mem_baseaddr
    set $total_bytes = $platform_mem_maxsize
    echo "\n--- Blocked Tasks ---\n"
    # Current delayed list
    echo "Delayed Task List (Current):\n"
    print_tasks_in_list pxDelayedTaskList 0x02U $base_addr $total_bytes
    # Overflow delayed list
    echo "Delayed Task List (Overflow):\n"
    print_tasks_in_list pxOverflowDelayedTaskList 0x02U $base_addr $total_bytes
    echo "--- End of FreeRTOS Blocked Task Snapshot ---\n"
end

define freertos_suspended_tasks_snapshot
    set $base_addr = $platform_mem_baseaddr
    set $total_bytes = $platform_mem_maxsize
    echo "\n--- Suspended Tasks ---\n"
    # We assume xSuspendedTaskList exists based on tasks.c content
    print_tasks_in_list &xSuspendedTaskList 0x03U $base_addr $total_bytes
    # Tasks Waiting Termination (Deleted Tasks - if INCLUDE_vTaskDelete is enabled)
    echo "\n--- Tasks Waiting Termination (Deleted) ---\n"
    # We assume xTasksWaitingTermination exists based on tasks.c content
    print_tasks_in_list &xTasksWaitingTermination 0x04U $base_addr $total_bytes
    echo "--- End of FreeRTOS Suspended Task Snapshot ---\n"
end
